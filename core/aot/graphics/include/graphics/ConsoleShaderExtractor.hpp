#ifndef CORE_AOT_GRAPHICS_CONSOLESHADEREXTRACTOR_HPP
#define CORE_AOT_GRAPHICS_CONSOLESHADEREXTRACTOR_HPP

#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <filesystem>
#include <span>
#include <optional>

namespace Aot::Graphics {

enum class ConsoleShaderStage {
    Vertex,
    Fragment,
    Compute,
    Geometry,
    Hull,
    Domain
};

struct ResourceBindingInfo {
    uint32_t Binding;
    uint32_t DescriptorSet;
    VkDescriptorType DescriptorType;
    VkShaderStageFlags StageFlags;
    uint32_t Count;
};

struct ExtractedShaderPackage {
    ConsoleShaderStage Stage;
    uint64_t ShaderHash;
    std::vector<uint8_t> RawBytecode;
    std::vector<ResourceBindingInfo> ResourceBindings;
    uint32_t PushConstantSize;
};

class ConsoleShaderExtractor {
public:
    [[nodiscard]] std::vector<ExtractedShaderPackage> ExtractFromFile(const std::filesystem::path& assetPath);
    [[nodiscard]] std::vector<ExtractedShaderPackage> ExtractFromMemory(std::span<const uint8_t> assetData);

private:
    bool parseHeader(const uint8_t* data, size_t size);
};

} // namespace Aot::Graphics
#endif
