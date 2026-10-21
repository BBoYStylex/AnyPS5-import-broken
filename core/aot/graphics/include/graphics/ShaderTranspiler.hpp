#ifndef CORE_AOT_GRAPHICS_SHADERTRANSPILER_HPP
#define CORE_AOT_GRAPHICS_SHADERTRANSPILER_HPP

#include <graphics/ConsoleShaderExtractor.hpp>
#include <cstdint>
#include <vector>
#include <string>
#include <optional>

namespace Aot::Graphics {

struct TranspiledShaderResult {
    VkShaderStageFlagBits Stage;
    std::string EntryPoint;
    std::vector<uint32_t> SpirvBytecode;
    std::vector<ResourceBindingInfo> Bindings;
    uint32_t PushConstantSize;
};

class ShaderTranspiler {
public:
    ShaderTranspiler();
    ~ShaderTranspiler();

    [[nodiscard]] std::optional<TranspiledShaderResult> Transpile(const ExtractedShaderPackage& package);

private:
    std::vector<uint32_t> emitSpirvHeader(uint32_t boundId);
};

} // namespace Aot::Graphics
#endif
