#include "BdaAbi.hpp"
#include "Recompiler.hpp"
#include "execution/FlatStoreFixture.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <spirv/unified1/spirv.hpp>

#if ANYPS5_ENABLE_SPIRV_TOOLS
#include <spirv-tools/libspirv.hpp>
#endif

namespace {

using namespace ShaderRecompiler;

SpirvTarget FlatStoreTarget() {
    // Fixed capability profile; no Vulkan device is created.
    static constexpr std::array<std::uint32_t, 3> capabilities{
        spv::CapabilityInt64,
        spv::CapabilityPhysicalStorageBufferAddresses,
        spv::CapabilityStorageBuffer8BitAccess,
    };
    static constexpr std::array<std::string_view, 4> extensions{
        "SPV_EXT_descriptor_indexing",
        "SPV_KHR_8bit_storage",
        "SPV_KHR_float_controls",
        "SPV_KHR_physical_storage_buffer",
    };
    return {0x00401000u, 0x00010300u, 64, BdaAbi::Version,
            capabilities, extensions, false,
            {1024, 1024, 64}, 1024, 32768, {}, {}};
}

void CheckFlatStore(std::uint32_t waveSize) {
    const std::span<const std::uint32_t> code(
        FlatStoreFixture::FlatStoreCode);

    if (code.size() != 45u) {
        throw std::runtime_error("flat-store fixture size changed");
    }

    const std::array<MemoryRegion, 1> memory{{
        {reinterpret_cast<std::uintptr_t>(code.data()),
         std::as_bytes(code)}
    }};

    std::array<std::uint32_t, 8> userData{};
    userData[0] = 0x10000u;

    const ShaderComputeStageInfo compute{
        {64, 1, 1}, 0, {false, false, false}, false, 1, {}
    };

    RecompileRequest request{
        {ShaderStage::Compute,
         reinterpret_cast<std::uintptr_t>(code.data()),
         code, 0, {}},
        {waveSize, 0, userData, compute,
         std::nullopt, std::nullopt, memory},
        FlatStoreTarget(),
        {0, 0, 0, 128},
        std::nullopt,
        false,
    };

    const auto result = Recompile(request);
    const auto words = result.spirv.Words();

    if (words.size() < 5 || words[0] != spv::MagicNumber) {
        throw std::runtime_error("flat store: invalid SPIR-V header");
    }

    std::size_t storeCount = 0;

    for (std::size_t offset = 5; offset < words.size();) {
        const auto instruction = words[offset];
        const auto count = instruction >> spv::WordCountShift;

        if (count == 0 || count > words.size() - offset) {
            throw std::runtime_error(
                "flat store: malformed SPIR-V instruction");
        }

        if ((instruction & spv::OpCodeMask) == spv::OpStore) {
            ++storeCount;
        }

        offset += count;
    }

    if (storeCount == 0) {
        throw std::runtime_error(
            "flat store: no SPIR-V store instruction");
    }

#if ANYPS5_ENABLE_SPIRV_TOOLS
    std::string diagnostics;
    spvtools::SpirvTools validator(SPV_ENV_VULKAN_1_1);

    validator.SetMessageConsumer(
        [&](spv_message_level_t, const char*,
            const spv_position_t&, const char* message) {
            if (message) {
                diagnostics += std::string(message) + '\n';
            }
        });

    if (!validator.Validate(words)) {
        throw std::runtime_error(
            "flat store: SPIR-V validation failed:\n" + diagnostics);
    }
#endif

    std::printf("flat store wave%u: %zu SPIR-V words\n",
                waveSize, words.size());
}

}

int main() {
    try {
        CheckFlatStore(32);
        CheckFlatStore(64);
        std::puts("CPU-only flat store compilation passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
