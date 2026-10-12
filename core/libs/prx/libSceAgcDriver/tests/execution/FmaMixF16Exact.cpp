#include "prx/libSceAgcDriver/Execution/include/VulkanDevice.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Draw.hpp"
#include "Recompiler.hpp"
#include "VulkanTestDevice.hpp"
#include <spirv/unified1/spirv.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

using AgcDriver::Graphics::Require;
using ShaderRecompiler::ShaderStage;

constexpr std::uint32_t Threads = 32;
constexpr std::uint32_t Lanes = 4096;
constexpr std::uint32_t Batches = 16;
constexpr std::uint32_t Variants = 4;
alignas(256) std::array<std::uint32_t, Lanes * 4> Input{};
alignas(256) std::array<std::uint32_t, Lanes * 4> Output{};

alignas(256) constexpr std::array<std::uint32_t, 17> Code{
    0x34020084, 0x8f098908, 0x4a020209, 0xe0381000, 0x80000401, 0xbf8c3f70, 0xcc214008, 0x1c1a0b04,
    0xcc210009, 0x141a0b04, 0xcc21400a, 0x0c1a0b04, 0xcc21000b, 0x041a0b04, 0xe0781000, 0x80010801,
    0xbf810000,
};

constexpr std::array<std::array<bool, 3>, Variants> HalfSources{{
    {true, true, true}, {false, true, false}, {true, false, true}, {false, false, false},
}};

constexpr std::array<const char*, Variants> Names{
    "v_fma_mixlo_f16 (f16, f16, f16)", "v_fma_mixlo_f16 (f32, f16, f32)", "v_fma_mixlo_f16 (f16, f32, f16)", "v_fma_mixlo_f16 (f32, f32, f32)",
};

float HalfValue(std::uint32_t half) {
    const std::uint32_t sign = (half & 0x8000u) << 16u;
    const std::uint32_t exponent = (half >> 10u) & 0x1fu;
    const std::uint32_t mantissa = half & 0x3ffu;
    if (exponent == 0u) return std::bit_cast<float>(std::bit_cast<std::uint32_t>(static_cast<float>(mantissa) * 0x1p-24f) | sign);
    return std::bit_cast<float>(sign | ((exponent + 112u) << 23u) | (mantissa << 13u));
}

float Source(std::uint32_t word, bool half) {
    if (half) return HalfValue(word & 0xffffu);
    return std::bit_cast<float>((word & 0x7f800000u) == 0u ? word & 0x80000000u : word);
}

std::uint32_t Expected(float a, float b, float c, bool overflowClamp) {
    const long double product = static_cast<long double>(a) * static_cast<long double>(b);
    long double sum = product + static_cast<long double>(c);
    const long double cPart = sum - product;
    const long double productPart = sum - cPart;
    const long double error = (product - productPart) + (static_cast<long double>(c) - cPart);
    if (error != 0.0L) {
        std::uint64_t significand = 0;
        std::memcpy(&significand, &sum, sizeof(significand));
        if ((significand & 1u) == 0u) sum = std::nextafter(sum, error > 0.0L ? INFINITY : -INFINITY);
    }
    const std::uint32_t sign = std::signbit(sum) ? 0x8000u : 0u;
    const long double magnitude = std::fabs(sum);
    if (magnitude == 0.0L) return sign;
    const int step = std::max(std::ilogb(magnitude) - 10, -24);
    const long double scaled = std::ldexp(magnitude, -step);
    const long double floor = std::floor(scaled);
    const long double fraction = scaled - floor;
    long double rounded = floor;
    if (fraction > 0.5L || (fraction == 0.5L && std::fmod(floor, 2.0L) != 0.0L)) rounded = floor + 1.0L;
    const long double value = std::ldexp(rounded, step);
    if (value >= 65520.0L) return sign | (overflowClamp ? 0x7bffu : 0x7c00u);
    const std::uint32_t bits = std::bit_cast<std::uint32_t>(static_cast<float>(value));
    if (value < 0x1p-14L) return sign | static_cast<std::uint32_t>(std::ldexp(value, 24));
    return sign | ((((bits >> 23u) & 0xffu) - 112u) << 10u) | ((bits >> 13u) & 0x3ffu);
}

std::array<std::uint32_t, 4> BufferDescriptor(const void* data, std::uint32_t bytes) {
    const auto address = reinterpret_cast<std::uintptr_t>(data);
    return {static_cast<std::uint32_t>(address), static_cast<std::uint32_t>((address >> 32u) & 0xffffu), bytes, 0x31016facu};
}

std::string Hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof(text), "0x%08x", value);
    return text;
}

struct Generator {
    std::mt19937_64 random{0x6d69786c6fu};

    std::uint32_t Half() {
        std::uint32_t half = static_cast<std::uint32_t>(random()) & 0xffffu;
        if (((half >> 10u) & 0x1fu) == 0x1fu) half &= 0xbfffu;
        return half;
    }

    std::uint32_t Word() {
        std::uint32_t exponent = 100u + static_cast<std::uint32_t>(random() % 50u);
        if (random() % 8u == 0u) exponent = 1u + static_cast<std::uint32_t>(random() % 253u);
        const std::uint32_t high = (static_cast<std::uint32_t>(random() & 1u) << 31u) | (exponent << 23u) | (static_cast<std::uint32_t>(random()) & 0x7f0000u);
        return high | Half();
    }

    std::uint32_t NearHalf(float value) {
        const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
        const std::uint32_t sign = (bits >> 16u) & 0x8000u;
        const std::uint32_t exponent = (bits >> 23u) & 0xffu;
        if (exponent < 103u || exponent > 142u) return Half();
        if (exponent < 113u) return sign | ((((bits & 0x7fffffu) | 0x800000u) >> (126u - exponent)) & 0x3ffu);
        return sign | ((exponent - 112u) << 10u) | ((bits >> 13u) & 0x3ffu);
    }

    void Fill(std::uint32_t batch) {
        for (std::uint32_t lane = 0; lane < Lanes; ++lane) {
            std::uint32_t* row = &Input[lane * 4u];
            row[0] = Word();
            row[1] = Word();
            row[2] = Word();
            row[3] = 0u;
            const std::uint32_t variant = static_cast<std::uint32_t>(random() % Variants);
            const float a = Source(row[0], HalfSources[variant][0]);
            const float b = Source(row[1], HalfSources[variant][1]);
            switch ((random() + batch) % 4u) {
                case 0: {
                    const float product = a * b;
                    if (!std::isfinite(product)) break;
                    if (HalfSources[variant][2]) row[2] = (row[2] & 0xffff0000u) | NearHalf(-product);
                    else row[2] = std::bit_cast<std::uint32_t>(-product) ^ static_cast<std::uint32_t>(random() & 0x3fu);
                    break;
                }
                case 1: {
                    const std::uint32_t big = 0x7800u + static_cast<std::uint32_t>(random() % 0x3ffu);
                    row[0] = (row[0] & 0xffff0000u) | big;
                    row[1] = (row[1] & 0xffff0000u) | 0x3c00u | static_cast<std::uint32_t>(random() % 0x10u);
                    break;
                }
                case 2:
                    row[0] = (row[0] & 0xffff0000u) | (static_cast<std::uint32_t>(random() % 0x0800u));
                    row[1] = (row[1] & 0xffff0000u) | (0x0400u + static_cast<std::uint32_t>(random() % 0x1000u));
                    break;
                default:
                    break;
            }
            for (std::uint32_t source = 0; source < 3u; ++source) {
                if (((row[source] >> 10u) & 0x1fu) == 0x1fu) row[source] &= ~0x4000u;
            }
        }
    }
};

}

int main() {
    try {
        const auto device = OpenVulkanTestDevice();
        if (!device) return VulkanTestSkipped;
        const auto capabilities = device->Target().supportedCapabilities;
        if (std::find(capabilities.begin(), capabilities.end(), static_cast<std::uint32_t>(spv::CapabilityFloat64)) == capabilities.end()) {
            std::puts("skipped, the exact fallback of v_fma_mixlo_f16 is computed in f64 and the device has no shaderFloat64");
            return VulkanTestSkipped;
        }
        const std::span<const std::uint32_t> code(Code);
        std::vector<std::uint32_t> userData(8, 0u);
        const auto input = BufferDescriptor(Input.data(), static_cast<std::uint32_t>(Input.size() * 4u));
        const auto output = BufferDescriptor(Output.data(), static_cast<std::uint32_t>(Output.size() * 4u));
        std::copy(input.begin(), input.end(), userData.begin());
        std::copy(output.begin(), output.end(), userData.begin() + 4);
        const std::array<ShaderRecompiler::MemoryRegion, 1> memory{{{reinterpret_cast<std::uintptr_t>(code.data()), std::as_bytes(code)}}};
        const ShaderRecompiler::ShaderComputeStageInfo compute{{Threads, 1, 1}, 0u, {true, false, false}, false, 1};
        Generator generator;
        std::uint64_t checked = 0u;
        for (const bool overflowClamp : {false, true}) {
            ShaderRecompiler::RecompileRequest request{
                {ShaderStage::Compute, reinterpret_cast<std::uintptr_t>(code.data()), code, 0, {}},
                {32, 0, userData, compute, std::nullopt, std::nullopt, memory},
                device->Target(),
                {0, 0, 0, 128}
            };
            request.useCache = false;
            request.context.floatMode = ShaderRecompiler::ShaderFloatMode{0xc0u, true, false, overflowClamp};
            const auto result = ShaderRecompiler::Recompile(request);
            for (std::uint32_t batch = 0; batch < Batches; ++batch) {
                generator.Fill(batch);
                Output.fill(0xdeadbeefu);
                device->Dispatch(result, Lanes / Threads, 1, 1, {}, reinterpret_cast<std::uintptr_t>(code.data()));
                device->WaitIdle();
                for (std::uint32_t lane = 0; lane < Lanes; ++lane) {
                    const std::uint32_t* row = &Input[lane * 4u];
                    for (std::uint32_t variant = 0; variant < Variants; ++variant) {
                        const float a = Source(row[0], HalfSources[variant][0]);
                        const float b = Source(row[1], HalfSources[variant][1]);
                        const float c = Source(row[2], HalfSources[variant][2]);
                        const std::uint32_t expected = Expected(a, b, c, overflowClamp);
                        const std::uint32_t actual = Output[lane * 4u + variant] & 0xffffu;
                        Require(actual == expected, std::string(Names[variant]) + (overflowClamp ? " FP16_OVFL" : "") + ": a=" + Hex(row[0]) + " b=" + Hex(row[1]) + " c=" + Hex(row[2]) + " is " + Hex(actual) + ", expected " + Hex(expected));
                        ++checked;
                    }
                }
            }
        }
        std::printf("fma mix f16 exact tests passed (%llu results)\n", static_cast<unsigned long long>(checked));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
