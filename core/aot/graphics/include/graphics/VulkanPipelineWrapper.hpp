#ifndef CORE_AOT_GRAPHICS_VULKANPIPELINEWRAPPER_HPP
#define CORE_AOT_GRAPHICS_VULKANPIPELINEWRAPPER_HPP

#include <vulkan/vulkan.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <optional>

namespace Aot::Graphics {

struct ShaderStageDescription {
    VkShaderStageFlagBits Stage;
    std::vector<uint32_t> SpirvBytecode;
    std::string EntryPoint;
};

struct PipelineConfiguration {
    std::vector<ShaderStageDescription> Stages;
    std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
    std::vector<VkPushConstantRange> PushConstantRanges;
    VkRenderPass RenderPass;
    uint32_t SubpassIndex;
    VkExtent2D ViewportExtent;
};

class VulkanPipelineWrapper {
public:
    VulkanPipelineWrapper(VkDevice device, VkPhysicalDevice physicalDevice);
    ~VulkanPipelineWrapper();

    VulkanPipelineWrapper(const VulkanPipelineWrapper&) = delete;
    VulkanPipelineWrapper& operator=(const VulkanPipelineWrapper&) = delete;
    VulkanPipelineWrapper(VulkanPipelineWrapper&&) noexcept;
    VulkanPipelineWrapper& operator=(VulkanPipelineWrapper&&) noexcept;

    bool InitializePipelineCache(const std::filesystem::path& cacheFilePath);
    bool SavePipelineCache(const std::filesystem::path& cacheFilePath) const;

    VkShaderModule CreateShaderModule(const std::vector<uint32_t>& spirvBytecode);
    void DestroyShaderModule(VkShaderModule shaderModule);

    VkDescriptorSetLayout CreateStandardDescriptorSetLayout(
        uint32_t bindingCount,
        VkDescriptorType descriptorType,
        VkShaderStageFlags stageFlags
    );
    void DestroyDescriptorSetLayout(VkDescriptorSetLayout layout);

    VkPipelineLayout CreatePipelineLayout(
        const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
        const std::vector<VkPushConstantRange>& pushConstantRanges
    );

    VkPipeline CompileGraphicsPipeline(const PipelineConfiguration& configuration);
    VkPipeline CompileComputePipeline(
        const std::vector<uint32_t>& computeSpirv,
        const std::vector<VkDescriptorSetLayout>& descriptorLayouts,
        std::string_view entryPoint = "main"
    );

    void WarmupPipelineCache(const std::vector<PipelineConfiguration>& configurations);

    [[nodiscard]] VkPipelineCache GetPipelineCache() const { return pipelineCache; }

private:
    void cleanup();

    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkPipelineCache pipelineCache;
    std::vector<VkPipeline> createdPipelines;
    std::vector<VkPipelineLayout> createdLayouts;
    std::vector<VkShaderModule> createdShaderModules;
    std::vector<VkDescriptorSetLayout> createdDescriptorSetLayouts;
};

} // namespace Aot::Graphics
#endif
