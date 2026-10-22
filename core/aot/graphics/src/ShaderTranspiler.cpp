#include <graphics/ShaderTranspiler.hpp>
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

    uint32_t boundId = 8 + static_cast<uint32_t>(package.ResourceBindings.size() * 2);
    spirvWords = emitSpirvHeader(boundId);

    spirvWords.push_back((2 << 16) | 17);
    spirvWords.push_back(1);

    spirvWords.push_back((3 << 16) | 14);
    spirvWords.push_back(0);
    spirvWords.push_back(1);

    spirvWords.push_back((6 << 16) | 15);
    spirvWords.push_back(executionModel);
    spirvWords.push_back(1);
    spirvWords.push_back(0x6e69616d);
    spirvWords.push_back(0x00000000);
    spirvWords.push_back(0);

    if (stageFlag == VK_SHADER_STAGE_FRAGMENT_BIT) {
        spirvWords.push_back((3 << 16) | 16);
        spirvWords.push_back(1);
        spirvWords.push_back(7);
    }

    uint32_t varId = 10;
    for (const auto& binding : package.ResourceBindings) {
        spirvWords.push_back((4 << 16) | 71);
        spirvWords.push_back(varId);
        spirvWords.push_back(33);
        spirvWords.push_back(binding.Binding);

        spirvWords.push_back((4 << 16) | 71);
        spirvWords.push_back(varId);
        spirvWords.push_back(34);
        spirvWords.push_back(binding.DescriptorSet);
        ++varId;
    }

    if (varId >= boundId) {
        spirvWords[3] = varId + 1;
    }

    spirvWords.push_back((2 << 16) | 19);
    spirvWords.push_back(2);

    spirvWords.push_back((3 << 16) | 33);
    spirvWords.push_back(3);
    spirvWords.push_back(2);

    spirvWords.push_back((5 << 16) | 54);
    spirvWords.push_back(2);
    spirvWords.push_back(1);
    spirvWords.push_back(0);
    spirvWords.push_back(3);

    spirvWords.push_back((2 << 16) | 248);
    spirvWords.push_back(4);

    spirvWords.push_back((1 << 16) | 253);

    spirvWords.push_back((1 << 16) | 56);

    return TranspiledShaderResult{
        .Stage = stageFlag,
        .EntryPoint = "main",
        .SpirvBytecode = std::move(spirvWords),
        .Bindings = package.ResourceBindings,
        .PushConstantSize = package.PushConstantSize
    };
}

} // namespace Aot::Graphics
