#include <graphics/GraphicsDriver.hpp>
#include <fstream>

namespace Aot::Graphics {

GraphicsDriver::GraphicsDriver() = default;
GraphicsDriver::GraphicsDriver(VulkanPipelineWrapper* inPipelineWrapper)
    : pipelineWrapper(inPipelineWrapper) {}
GraphicsDriver::~GraphicsDriver() = default;

void GraphicsDriver::SetPipelineWrapper(VulkanPipelineWrapper* inPipelineWrapper) {
    pipelineWrapper = inPipelineWrapper;
}

GraphicsProcessingSummary GraphicsDriver::ProcessAssets(const GraphicsProcessingOptions& options) {
    GraphicsProcessingSummary summary{};

    ConsoleShaderExtractor extractor;
    std::vector<ExtractedShaderPackage> packages = extractor.ExtractFromFile(options.InputAssetPath);
    summary.ShadersExtracted = packages.size();

    if (packages.empty()) {
        return summary;
    }

    if (!options.OutputSpirvDirectory.empty()) {
        std::filesystem::create_directories(options.OutputSpirvDirectory);
    }

    ShaderTranspiler transpiler;
    std::vector<TranspiledShaderResult> transpiledShaders;

    for (size_t index = 0; index < packages.size(); ++index) {
        auto resultOpt = transpiler.Transpile(packages[index]);
        if (resultOpt.has_value()) {
            ++summary.ShadersTranspiled;

            if (!options.OutputSpirvDirectory.empty()) {
                std::string filename = "shader_" + std::to_string(index) + ".spv";
                std::filesystem::path outputPath = options.OutputSpirvDirectory / filename;

                std::ofstream file(outputPath, std::ios::binary | std::ios::trunc);
                if (file.is_open()) {
                    file.write(
                        reinterpret_cast<const char*>(resultOpt->SpirvBytecode.data()),
                        static_cast<std::streamsize>(resultOpt->SpirvBytecode.size() * sizeof(uint32_t))
                    );
                }
            }

            transpiledShaders.push_back(std::move(*resultOpt));
        }
    }

    if (pipelineWrapper != nullptr && !transpiledShaders.empty()) {
        if (!options.OutputCachePath.empty()) {
            pipelineWrapper->InitializePipelineCache(options.OutputCachePath);
        }

        std::vector<PipelineConfiguration> configs;
        for (const auto& shader : transpiledShaders) {
            if (shader.Stage == ConsoleShaderStage::Compute) {
                VkPipeline pipeline = pipelineWrapper->CompileComputePipeline(shader.SpirvBytecode, {}, "main");
                if (pipeline != VK_NULL_HANDLE) {
                    ++summary.PipelinesCompiled;
                }
            } else {
                PipelineConfiguration config{};
                VkShaderStageFlagBits stageBit = VK_SHADER_STAGE_VERTEX_BIT;
                if (shader.Stage == ConsoleShaderStage::Fragment) {
                    stageBit = VK_SHADER_STAGE_FRAGMENT_BIT;
                } else if (shader.Stage == ConsoleShaderStage::Geometry) {
                    stageBit = VK_SHADER_STAGE_GEOMETRY_BIT;
                }
                config.Stages.push_back({
                    .Stage = stageBit,
                    .SpirvBytecode = shader.SpirvBytecode,
                    .EntryPoint = "main"
                });
                config.ViewportExtent = {1920, 1080};
                configs.push_back(std::move(config));
            }
        }

        if (options.WarmupPipelines && !configs.empty()) {
            pipelineWrapper->WarmupPipelineCache(configs);
            summary.PipelinesCompiled += configs.size();
        }

        if (!options.OutputCachePath.empty()) {
            summary.CacheSaved = pipelineWrapper->SavePipelineCache(options.OutputCachePath);
        }
    }

    return summary;
}

} // namespace Aot::Graphics
