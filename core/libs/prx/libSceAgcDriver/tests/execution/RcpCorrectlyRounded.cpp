#include "prx/libSceAgcDriver/Execution/include/VulkanDevice.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Draw.hpp"
#include "Recompiler.hpp"
#include "VulkanTestDevice.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {

using AgcDriver::Graphics::Require;
using ShaderRecompiler::ShaderStage;

constexpr std::uint32_t Threads = 32;
constexpr std::uint32_t Batches = 256;
alignas(256) std::array<std::uint32_t, Threads> Input{};
alignas(256) std::array<std::uint32_t, Threads> Output{};

alignas(256) constexpr std::array<std::uint32_t, 9> Code{
    0x34020082, 0xe0301000, 0x80000201, 0xbf8c3f70, 0x7e065502, 0xe0701000, 0x80010301, 0xbf8c3f70,
    0xbf810000,
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

std::uint32_t Sample(std::uint32_t index) {
    auto state = (index + 1u) * 0x9e3779b9u;
    state ^= state >> 15u;
    state *= 0x2c1b3c6du;
    state ^= state >> 12u;
    const auto exponent = 2u + (index * 7u) % 250u;
    return (index & 1u) << 31u | exponent << 23u | (state & 0x7fffffu);
}

double Distance(float candidate, float value) {
    return std::abs(static_cast<double>(candidate) * static_cast<double>(value) - 1.0);
}

std::uint32_t Rounded(std::uint32_t bits) {
    const auto value = std::bit_cast<float>(bits);
    const auto estimate = static_cast<float>(1.0 / static_cast<double>(value));
    auto best = estimate;
    for (const auto candidate : {std::nextafter(estimate, -INFINITY), std::nextafter(estimate, INFINITY)}) {
        const auto distance = Distance(candidate, value);
        const auto bestDistance = Distance(best, value);
        if (distance < bestDistance || (distance == bestDistance && (std::bit_cast<std::uint32_t>(candidate) & 1u) == 0u)) best = candidate;
    }
    return std::bit_cast<std::uint32_t>(best);
}

}

int main() {
    try {
        const auto device = OpenVulkanTestDevice();
        if (!device) return VulkanTestSkipped;
        std::vector<std::uint32_t> userData(8, 0u);
        const auto input = BufferDescriptor(Input.data(), static_cast<std::uint32_t>(Input.size() * 4u));
        const auto output = BufferDescriptor(Output.data(), static_cast<std::uint32_t>(Output.size() * 4u));
        std::copy(input.begin(), input.end(), userData.begin());
        std::copy(output.begin(), output.end(), userData.begin() + 4);
        const std::span<const std::uint32_t> code(Code);
        const std::array<ShaderRecompiler::MemoryRegion, 1> memory{{{reinterpret_cast<std::uintptr_t>(code.data()), std::as_bytes(code)}}};
        const ShaderRecompiler::ShaderComputeStageInfo compute{{Threads, 1, 1}, 0u, {false, false, false}, false, 1};
        ShaderRecompiler::RecompileRequest request{
            {ShaderStage::Compute, reinterpret_cast<std::uintptr_t>(code.data()), code, 0, {}},
            {32, 0, userData, compute, std::nullopt, std::nullopt, memory},
            device->Target(),
            {0, 0, 0, 128}
        };
        request.useCache = false;
        request.context.floatMode = ShaderRecompiler::ShaderFloatMode{0xf0u, true, true, false};
        const auto result = ShaderRecompiler::Recompile(request);
        for (std::uint32_t batch = 0; batch < Batches; ++batch) {
            for (std::uint32_t tid = 0; tid < Threads; ++tid) Input[tid] = Sample(batch * Threads + tid);
            Output.fill(0xdeadbeefu);
            device->Dispatch(result, 1, 1, 1, {}, reinterpret_cast<std::uintptr_t>(code.data()));
            device->WaitIdle();
            for (std::uint32_t tid = 0; tid < Threads; ++tid) {
                const auto wanted = Rounded(Input[tid]);
                Require(Output[tid] == wanted, std::string("v_rcp_f32 of ") + Hex(Input[tid]) + " is " + Hex(Output[tid]) + ", expected the correctly rounded " + Hex(wanted));
            }
        }
        std::puts("rcp correctly rounded tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
