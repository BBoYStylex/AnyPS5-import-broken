#include <graphics/GraphicsDriver.hpp>
#include <fstream>

namespace Aot::Graphics {

GraphicsDriver::GraphicsDriver() = default;
GraphicsDriver::~GraphicsDriver() = default;

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

    return summary;
}

} // namespace Aot::Graphics
