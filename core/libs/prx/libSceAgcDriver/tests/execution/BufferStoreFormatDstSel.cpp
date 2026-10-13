#include "prx/libSceAgcDriver/Execution/include/VulkanDevice.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Draw.hpp"
#include "Recompiler.hpp"
#include "VulkanTestDevice.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using AgcDriver::Graphics::Require;
using ShaderRecompiler::ShaderStage;

constexpr std::uint32_t Threads = 64;
constexpr std::uint32_t LaneDwords = 16;
constexpr std::uint32_t Fill = 0xaaaaaaaau;
alignas(256) std::array<std::uint32_t, Threads * LaneDwords> Input{};
alignas(256) std::array<std::uint32_t, Threads * LaneDwords> Output{};
alignas(256) std::array<std::uint32_t, Threads * LaneDwords> Scratch{};

alignas(256) constexpr std::array<std::uint32_t, 39> Code{
    0x34020086, 0xe0381000, 0x80000401, 0xe0341010, 0x80000801, 0xe0381020, 0x80000c01, 0xe0381030,
    0x80001001, 0xbf8c3f70, 0xe01c1000, 0x80020401, 0xe21c1004, 0x80020801, 0xe0101008, 0x80020401,
    0xe01c1010, 0x80031001, 0xe01c1018, 0x80040c01, 0xe010101c, 0x80040c01, 0xe01c1020, 0x80050c01,
    0xbbfd0000, 0xe00c1000, 0x80021401, 0xe00c1010, 0x80031801, 0xe00c1018, 0x80041c01, 0xbf8c3f70,
    0xe0781000, 0x80011401, 0xe0781010, 0x80011801, 0xe0781020, 0x80011c01, 0xbf810000,
};

alignas(256) constexpr std::array<std::uint32_t, 6> StoreCode{
    0x34020086, 0xe01c1000, 0x80020401, 0xe01c1020, 0x80050401, 0xbf810000,
};

alignas(256) constexpr std::array<std::uint32_t, 5> GpuSelectedStoreCode{
    0x34020086, 0x7e160501, 0xe01c1000, 0x80020401, 0xbf810000,
};

constexpr std::uint32_t Unorm8888ZyxwWord3 = 0x31038f2eu;
constexpr std::uint32_t Uint16x4YzxwWord3 = 0x31045f35u;
constexpr std::uint32_t Uint8AlphaWord3 = 0x31005800u;
constexpr std::uint32_t Uint32RedWord3 = 0x31014204u;

constexpr std::array<std::uint32_t, 4> ZyxwChannelSlots{2, 1, 0, 3};
constexpr std::array<std::uint32_t, 4> YzxwChannelSlots{2, 0, 1, 3};

constexpr std::array<std::uint32_t, 8> UnormInputs{0x00000000u, 0x3f800000u, 0x3e800000u, 0x3f000000u, 0x3f400000u, 0x40000000u, 0xbf800000u, 0x3e4ccccdu};
constexpr std::array<std::uint32_t, 8> HalfInputs{0x0000u, 0x3c00u, 0x3400u, 0x3800u, 0x3a00u, 0x4000u, 0xbc00u, 0x3266u};
constexpr std::array<std::uint32_t, 8> UnormBytes{0x00u, 0xffu, 0x40u, 0x80u, 0xbfu, 0xffu, 0x00u, 0x33u};
constexpr std::array<std::uint32_t, 8> UnormLoads{0x00000000u, 0x3f800000u, 0x3e808081u, 0x3f008081u, 0x3f3fbfc0u, 0x3f800000u, 0x00000000u, 0x3e4ccccdu};

std::uint32_t UnormIndex(std::uint32_t lane, std::uint32_t slot) {
    return (lane + slot * 3u + 1u) % 8u;
}

std::uint32_t HalfIndex(std::uint32_t lane, std::uint32_t slot) {
    return (lane + slot * 3u + 6u) % 8u;
}

std::uint32_t Byte(std::uint32_t lane, std::uint32_t slot) {
    return (lane * 4u + slot * 0x41u + 0x11u) & 0xffu;
}

std::uint32_t Short(std::uint32_t lane, std::uint32_t slot) {
    return (lane * 0x0101u + slot * 0x3c51u + 0x1234u) & 0xffffu;
}

std::array<std::uint32_t, 4> BufferDescriptor(const void* data, std::uint32_t bytes, std::uint32_t word3) {
    const auto address = reinterpret_cast<std::uintptr_t>(data);
    return {static_cast<std::uint32_t>(address), static_cast<std::uint32_t>((address >> 32u) & 0xffffu), bytes, word3};
}

std::string Hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof(text), "0x%08x", value);
    return text;
}

std::vector<std::uint32_t> UserData() {
    std::vector<std::uint32_t> userData(24u, 0u);
    const std::array<std::array<std::uint32_t, 4>, 6> descriptors{
        BufferDescriptor(Input.data(), sizeof(Input), 0x31016facu),
        BufferDescriptor(Output.data(), sizeof(Output), 0x31016facu),
        BufferDescriptor(Scratch.data(), sizeof(Scratch), Unorm8888ZyxwWord3),
        BufferDescriptor(Scratch.data(), sizeof(Scratch), Uint16x4YzxwWord3),
        BufferDescriptor(Scratch.data(), sizeof(Scratch), Uint8AlphaWord3),
        BufferDescriptor(Scratch.data(), sizeof(Scratch), Uint32RedWord3),
    };
    for (std::size_t index = 0; index < descriptors.size(); ++index) std::copy(descriptors[index].begin(), descriptors[index].end(), userData.begin() + 4 * index);
    return userData;
}

ShaderRecompiler::RecompileResult Compile(AgcDriver::VulkanDevice& device, std::span<const std::uint32_t> code, std::uint32_t waveSize, std::span<const std::uint32_t> userData) {
    const std::array<ShaderRecompiler::MemoryRegion, 1> memory{{{reinterpret_cast<std::uintptr_t>(code.data()), std::as_bytes(code)}}};
    const ShaderRecompiler::ShaderComputeStageInfo compute{{Threads, 1, 1}, 0u, {false, false, false}, false, 1};
    ShaderRecompiler::RecompileRequest request{
        {ShaderStage::Compute, reinterpret_cast<std::uintptr_t>(code.data()), code, 0, {}},
        {waveSize, 0, userData, compute, std::nullopt, std::nullopt, memory},
        device.Target(),
        {0, 0, 0, 128}
    };
    request.useCache = false;
    return ShaderRecompiler::Recompile(request);
}

void Run(AgcDriver::VulkanDevice& device, std::uint32_t waveSize) {
    Input.fill(0u);
    for (std::uint32_t lane = 0; lane < Threads; ++lane) {
        auto* row = &Input[lane * LaneDwords];
        for (std::uint32_t slot = 0; slot < 4u; ++slot) {
            row[slot] = UnormInputs[UnormIndex(lane, slot)];
            row[4u + slot / 2u] |= HalfInputs[HalfIndex(lane, slot)] << (slot % 2u * 16u);
            row[8u + slot] = Byte(lane, slot);
            row[12u + slot] = Short(lane, slot);
        }
    }
    Output.fill(0xdeadbeefu);
    Scratch.fill(Fill);
    const auto userData = UserData();
    const auto result = Compile(device, Code, waveSize, userData);
    device.Dispatch(result, 1, 1, 1, {}, reinterpret_cast<std::uintptr_t>(Code.data()));
    device.WaitIdle();
}

void Expect(std::uint32_t waveSize, std::uint32_t lane, std::uint32_t actual, std::uint32_t expected, const std::string& name) {
    Require(actual == expected, "buffer store format dst_sel: wave" + std::to_string(waveSize) + " lane " + std::to_string(lane) + " " + name + " is " + Hex(actual) + ", expected " + Hex(expected));
}

void Check(std::uint32_t waveSize) {
    for (std::uint32_t lane = 0; lane < Threads; ++lane) {
        const auto* scratch = &Scratch[lane * LaneDwords];
        const auto* output = &Output[lane * LaneDwords];
        std::uint32_t unorm = 0;
        std::uint32_t half = 0;
        for (std::uint32_t channel = 0; channel < 4u; ++channel) {
            unorm |= UnormBytes[UnormIndex(lane, ZyxwChannelSlots[channel])] << (channel * 8u);
            half |= UnormBytes[HalfIndex(lane, ZyxwChannelSlots[channel])] << (channel * 8u);
        }
        Expect(waveSize, lane, scratch[0], unorm, "buffer_store_format_xyzw 8_8_8_8_UNORM Z,Y,X,W");
        Expect(waveSize, lane, scratch[1], half, "buffer_store_format_d16_xyzw 8_8_8_8_UNORM Z,Y,X,W");
        Expect(waveSize, lane, scratch[2], UnormBytes[UnormIndex(lane, 0u)] << 16u, "buffer_store_format_x 8_8_8_8_UNORM Z,Y,X,W");
        Expect(waveSize, lane, scratch[3], Fill, "scratch dword 3");
        Expect(waveSize, lane, scratch[4], Short(lane, YzxwChannelSlots[0]) | Short(lane, YzxwChannelSlots[1]) << 16u, "buffer_store_format_xyzw 16_16_16_16_UINT Y,Z,X,W low");
        Expect(waveSize, lane, scratch[5], Short(lane, YzxwChannelSlots[2]) | Short(lane, YzxwChannelSlots[3]) << 16u, "buffer_store_format_xyzw 16_16_16_16_UINT Y,Z,X,W high");
        Expect(waveSize, lane, scratch[6], (Fill & ~0xffu) | Byte(lane, 3u), "buffer_store_format_xyzw 8_UINT 0,0,0,X");
        Expect(waveSize, lane, scratch[7], Fill & ~0xffu, "buffer_store_format_x 8_UINT 0,0,0,X");
        Expect(waveSize, lane, scratch[8], Byte(lane, 0u), "buffer_store_format_xyzw 32_UINT X,0,0,1");
        for (std::uint32_t dword = 9; dword < LaneDwords; ++dword) Expect(waveSize, lane, scratch[dword], Fill, "scratch dword " + std::to_string(dword));
        for (std::uint32_t slot = 0; slot < 4u; ++slot) {
            const auto component = std::string(1, "xyzw"[slot]);
            Expect(waveSize, lane, output[slot], UnormLoads[UnormIndex(lane, slot)], "buffer_load_format_xyzw 8_8_8_8_UNORM Z,Y,X,W " + component);
            Expect(waveSize, lane, output[4u + slot], Short(lane, slot), "buffer_load_format_xyzw 16_16_16_16_UINT Y,Z,X,W " + component);
            Expect(waveSize, lane, output[8u + slot], slot == 3u ? Byte(lane, 3u) : 0u, "buffer_load_format_xyzw 8_UINT 0,0,0,X " + component);
        }
    }
}

void CheckThrows(AgcDriver::VulkanDevice& device) {
    struct Case {
        std::size_t word3;
        std::uint32_t value;
        const char* name;
    };
    constexpr std::array<Case, 3> cases{{
        {11, 0x31038fa4u, "X,X,Z,W on 8_8_8_8_UNORM"},
        {11, 0x3103832eu, "Z,Y,X,1 on 8_8_8_8_UNORM"},
        {23, 0x31014404u, "X,0,0 and the reserved selector 2 on 32_UINT"},
    }};
    for (const auto& test : cases) {
        auto userData = UserData();
        userData[test.word3] = test.value;
        bool thrown = false;
        try {
            static_cast<void>(Compile(device, StoreCode, 32u, userData));
        } catch (const std::runtime_error& error) {
            thrown = std::string_view(error.what()).find("does not name each format component once") != std::string_view::npos;
            Require(thrown, std::string("buffer store format dst_sel: ") + test.name + " failed with an unrelated error: " + error.what());
        }
        Require(thrown, std::string("buffer store format dst_sel: a store through DST_SEL ") + test.name + " compiled");
        userData[test.word3 - 1u] = 0u;
        static_cast<void>(Compile(device, StoreCode, 32u, userData));
    }
}

}

int main() {
    try {
        const auto device = OpenVulkanTestDevice();
        if (!device) return VulkanTestSkipped;
        for (const std::uint32_t waveSize : {32u, 64u}) {
            Run(*device, waveSize);
            Check(waveSize);
        }
        CheckThrows(*device);
        static_cast<void>(Compile(*device, GpuSelectedStoreCode, 32u, UserData()));
        std::puts("buffer store format dst_sel tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
