#ifndef CORE_SHADER_RECOMPILER_SPIRVBACKEND_SPIRVOPTIMIZER_HPP
#define CORE_SHADER_RECOMPILER_SPIRVBACKEND_SPIRVOPTIMIZER_HPP

#include <cstdint>
#include <span>
#include <vector>

namespace ShaderRecompiler {

namespace detail {

[[nodiscard]] inline std::vector<std::uint32_t> SelectValidatedSpirv(
    std::span<const std::uint32_t> input, std::span<const std::uint32_t> optimized, bool optimizedValid) {
    return optimizedValid ? std::vector<std::uint32_t>(optimized.begin(), optimized.end())
                          : std::vector<std::uint32_t>(input.begin(), input.end());
}

}

[[nodiscard]] std::vector<std::uint32_t> ValidateAndOptimizeSpirv(std::span<const std::uint32_t> spirv, std::uint32_t vulkanVersion, std::uint32_t spirvVersion, bool allowOffsetTextureOperand = false, bool optimize = true, bool specialize = false);

}

#endif
