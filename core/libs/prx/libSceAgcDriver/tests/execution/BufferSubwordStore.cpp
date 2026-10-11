#include "prx/libSceAgcDriver/Execution/include/VulkanDevice.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Draw.hpp"
#include "Recompiler.hpp"
#include "VulkanTestDevice.hpp"
#include <spirv/unified1/spirv.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <vector>

namespace {

using AgcDriver::Graphics::Require;
using ShaderRecompiler::ShaderStage;

constexpr std::uint32_t Lanes = 32;
constexpr std::uint8_t Fill = 0xa5u;
alignas(256) std::array<std::uint32_t, Lanes * 2> Input{};
alignas(256) std::array<std::uint8_t, 256> Output{};

alignas(256) constexpr std::array<std::uint32_t, 7> ByteCode{
    0x34020083, 0xe0341000, 0x80010201, 0xbf8c3f70, 0xe0601000, 0x80000302, 0xbf810000,
};

alignas(256) constexpr std::array<std::uint32_t, 7> ShortCode{
    0x34020083, 0xe0341000, 0x80010201, 0xbf8c3f70, 0xe0681000, 0x80000302, 0xbf810000,
};

std::array<std::uint32_t, 4> BufferDescriptor(const void* data, std::uint32_t bytes) {
    const auto address = reinterpret_cast<std::uintptr_t>(data);
    return {static_cast<std::uint32_t>(address), static_cast<std::uint32_t>((address >> 32u) & 0xffffu), bytes, 0x31016facu};
}

std::string Hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof(text), "0x%08x", value);
    return text;
}

bool HasNarrowStorage(std::span<const std::uint32_t> words, std::uint32_t bits) {
    for (std::size_t cursor = 5; cursor < words.size();) {
        const auto count = words[cursor] >> 16u;
        Require(count != 0 && cursor + count <= words.size(), "buffer subword store: malformed SPIR-V");
        if ((words[cursor] & 0xffffu) == spv::OpTypeInt && count >= 3 && words[cursor + 2] == bits) return true;
        cursor += count;
    }
    return false;
}

void Run(AgcDriver::VulkanDevice& device, const ShaderRecompiler::SpirvTarget& target, bool shorts, std::uint32_t misalignment, bool bytes, bool halves, const char* path, bool dispatch) {
    for (std::uint32_t lane = 0; lane < Lanes; ++lane) {
        const std::uint32_t position = shorts ? 4u * (lane + 1u) + lane % 3u : lane + 3u;
        Input[lane * 2u] = position - misalignment;
        Input[lane * 2u + 1u] = shorts ? 0x6000u + lane * 0x0101u : 0x40u + lane;
    }
    Output.fill(Fill);
    std::vector<std::uint32_t> userData(8, 0u);
    const auto output = BufferDescriptor(Output.data() + misalignment, 200u);
    const auto input = BufferDescriptor(Input.data(), static_cast<std::uint32_t>(Input.size() * 4u));
    std::copy(output.begin(), output.end(), userData.begin());
    std::copy(input.begin(), input.end(), userData.begin() + 4);
    const std::span<const std::uint32_t> code = shorts ? std::span<const std::uint32_t>(ShortCode) : std::span<const std::uint32_t>(ByteCode);
    const std::array<ShaderRecompiler::MemoryRegion, 1> memory{{{reinterpret_cast<std::uintptr_t>(code.data()), std::as_bytes(code)}}};
    const ShaderRecompiler::ShaderComputeStageInfo compute{{Lanes, 1, 1}, 0u, {false, false, false}, false, 1};
    ShaderRecompiler::RecompileRequest request{
        {ShaderStage::Compute, reinterpret_cast<std::uintptr_t>(code.data()), code, 0, {}},
        {Lanes, 0, userData, compute, std::nullopt, std::nullopt, memory},
        target,
        {0, 0, 0, 128}
    };
    request.useCache = false;
    const auto result = ShaderRecompiler::Recompile(request);
    const std::string name = std::string(shorts ? "buffer_store_short" : "buffer_store_byte") + " (" + path + ", base +" + std::to_string(misalignment) + ")";
    Require(HasNarrowStorage(result.spirv.Words(), 8u) == bytes, name + (bytes ? ": the 8-bit store view was not used" : ": an 8-bit store view was used"));
    Require(HasNarrowStorage(result.spirv.Words(), 16u) == halves, name + (halves ? ": the 16-bit store view was not used" : ": a 16-bit store view was used"));
    if (!dispatch) return;
    device.Dispatch(result, 1, 1, 1, {}, reinterpret_cast<std::uintptr_t>(code.data()));
    device.WaitIdle();
    std::array<std::uint8_t, 256> expected{};
    expected.fill(Fill);
    for (std::uint32_t lane = 0; lane < Lanes; ++lane) {
        const std::uint32_t at = misalignment + Input[lane * 2u];
        expected[at] = static_cast<std::uint8_t>(Input[lane * 2u + 1u]);
        if (shorts) expected[at + 1u] = static_cast<std::uint8_t>(Input[lane * 2u + 1u] >> 8u);
    }
    for (std::uint32_t byte = 0; byte < Output.size(); ++byte) {
        Require(Output[byte] == expected[byte], name + ": byte " + std::to_string(byte) + " is " + Hex(Output[byte]) + ", expected " + Hex(expected[byte]));
    }
}

}

int main() {
    try {
        const auto device = OpenVulkanTestDevice();
        if (!device) return VulkanTestSkipped;
        const auto target = device->Target();
        const auto supports = [&](spv::Capability capability) {
            return std::ranges::find(target.supportedCapabilities, static_cast<std::uint32_t>(capability)) != target.supportedCapabilities.end();
        };
        const auto without = [&](std::initializer_list<spv::Capability> removed) {
            std::vector<std::uint32_t> kept;
            std::ranges::copy_if(target.supportedCapabilities, std::back_inserter(kept), [&](std::uint32_t capability) { return std::ranges::find(removed, static_cast<spv::Capability>(capability)) == removed.end(); });
            return kept;
        };
        const auto withoutHalves = without({spv::CapabilityStorageBuffer16BitAccess});
        const auto withoutNarrow = without({spv::CapabilityStorageBuffer8BitAccess, spv::CapabilityStorageBuffer16BitAccess});
        auto byteTarget = target;
        byteTarget.supportedCapabilities = withoutHalves;
        auto wordTarget = target;
        wordTarget.supportedCapabilities = withoutNarrow;
        const bool bytes = supports(spv::CapabilityStorageBuffer8BitAccess);
        const bool halves = bytes && supports(spv::CapabilityStorageBuffer16BitAccess);
        for (std::uint32_t misalignment = 0; misalignment < 4u; ++misalignment) {
            for (const bool shorts : {false, true}) {
                Run(*device, target, shorts, misalignment, bytes, shorts && halves, "device target", true);
                Run(*device, byteTarget, shorts, misalignment, bytes, false, "8-bit stores", true);
                Run(*device, wordTarget, shorts, misalignment, false, false, "word updates", false);
            }
        }
        std::printf("buffer subword store tests passed (8-bit view: %s, 16-bit view: %s)\n", supports(spv::CapabilityStorageBuffer8BitAccess) ? "yes" : "no", supports(spv::CapabilityStorageBuffer16BitAccess) ? "yes" : "no");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
