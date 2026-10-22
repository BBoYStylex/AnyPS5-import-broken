#include <graphics/ShaderTranspiler.hpp>
#include <graphics/ShaderInstructionDecoder.hpp>
#include <cstring>

namespace Aot::Graphics {

ShaderTranspiler::ShaderTranspiler() = default;
ShaderTranspiler::~ShaderTranspiler() = default;

std::vector<uint32_t> ShaderTranspiler::emitSpirvHeader(uint32_t boundId) {
    return {
        0x07230203,
        0x00010300,
        0x00000000,
        boundId,
        0x00000000
    };
}

std::optional<TranspiledShaderResult> ShaderTranspiler::Transpile(const ExtractedShaderPackage& package) {
    VkShaderStageFlagBits stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    uint32_t executionModel = 0;

    switch (package.Stage) {
        case ConsoleShaderStage::Vertex:
            stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
            executionModel = 0;
            break;
        case ConsoleShaderStage::Fragment:
            stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
            executionModel = 4;
            break;
        case ConsoleShaderStage::Compute:
            stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;
            executionModel = 5;
            break;
        case ConsoleShaderStage::Geometry:
            stageFlag = VK_SHADER_STAGE_GEOMETRY_BIT;
            executionModel = 3;
            break;
        case ConsoleShaderStage::Hull:
            stageFlag = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            executionModel = 1;
            break;
        case ConsoleShaderStage::Domain:
            stageFlag = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            executionModel = 2;
            break;
    }

    std::vector<uint32_t> spirvWords;

    if (package.RawBytecode.size() >= 4) {
        uint32_t magic = 0;
        std::memcpy(&magic, package.RawBytecode.data(), sizeof(uint32_t));
        if (magic == 0x07230203) {
            spirvWords.resize(package.RawBytecode.size() / sizeof(uint32_t));
            std::memcpy(spirvWords.data(), package.RawBytecode.data(), package.RawBytecode.size());

            return TranspiledShaderResult{
                .Stage = stageFlag,
                .EntryPoint = "main",
                .SpirvBytecode = std::move(spirvWords),
                .Bindings = package.ResourceBindings,
                .PushConstantSize = package.PushConstantSize
            };
        }
    }

    ShaderInstructionDecoder decoder;
    std::vector<DecodedShaderInstruction> instructions = decoder.DecodeBytecode(package.RawBytecode);
    spirvWords = decoder.GenerateSpirvFromInstructions(instructions, executionModel, package.ResourceBindings);

    return TranspiledShaderResult{
        .Stage = stageFlag,
        .EntryPoint = "main",
        .SpirvBytecode = std::move(spirvWords),
        .Bindings = package.ResourceBindings,
        .PushConstantSize = package.PushConstantSize
    };
}

} // namespace Aot::Graphics
