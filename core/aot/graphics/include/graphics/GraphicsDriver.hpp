#ifndef CORE_AOT_GRAPHICS_GRAPHICSDRIVER_HPP
#define CORE_AOT_GRAPHICS_GRAPHICSDRIVER_HPP

#include <graphics/ConsoleShaderExtractor.hpp>
#include <graphics/ShaderTranspiler.hpp>
#include <graphics/VulkanPipelineWrapper.hpp>
#include <filesystem>
#include <string>
#include <vector>

namespace Aot::Graphics {

struct GraphicsProcessingOptions {
    std::filesystem::path InputAssetPath;
    std::filesystem::path OutputCachePath;
    std::filesystem::path OutputSpirvDirectory;
    bool WarmupPipelines = false;
};

struct GraphicsProcessingSummary {
    size_t ShadersExtracted = 0;
    size_t ShadersTranspiled = 0;
    size_t PipelinesCompiled = 0;
    bool CacheSaved = false;
};

class GraphicsDriver {
public:
    GraphicsDriver();
    ~GraphicsDriver();

    GraphicsProcessingSummary ProcessAssets(const GraphicsProcessingOptions& options);
};

} // namespace Aot::Graphics
#endif
