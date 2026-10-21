#include <graphics/VulkanPipelineWrapper.hpp>
#include <fstream>
#include <cstring>
#include <array>

namespace Aot::Graphics {

struct PipelineCacheHeader {
    uint32_t HeaderLength;
    uint32_t HeaderVersion;
    uint32_t VendorId;
    uint32_t DeviceId;
    uint8_t PipelineCacheUuid[VK_UUID_SIZE];
};

VulkanPipelineWrapper::VulkanPipelineWrapper(VkDevice inDevice, VkPhysicalDevice inPhysicalDevice)
    : device(inDevice), physicalDevice(inPhysicalDevice), pipelineCache(VK_NULL_HANDLE) {}

VulkanPipelineWrapper::~VulkanPipelineWrapper() {
    cleanup();
}

VulkanPipelineWrapper::VulkanPipelineWrapper(VulkanPipelineWrapper&& other) noexcept
    : device(other.device),
      physicalDevice(other.physicalDevice),
      pipelineCache(other.pipelineCache),
      createdPipelines(std::move(other.createdPipelines)),
      createdLayouts(std::move(other.createdLayouts)),
      createdShaderModules(std::move(other.createdShaderModules)),
      createdDescriptorSetLayouts(std::move(other.createdDescriptorSetLayouts)) {
    other.pipelineCache = VK_NULL_HANDLE;
}

VulkanPipelineWrapper& VulkanPipelineWrapper::operator=(VulkanPipelineWrapper&& other) noexcept {
    if (this != &other) {
        cleanup();
        device = other.device;
        physicalDevice = other.physicalDevice;
        pipelineCache = other.pipelineCache;
        createdPipelines = std::move(other.createdPipelines);
        createdLayouts = std::move(other.createdLayouts);
        createdShaderModules = std::move(other.createdShaderModules);
        createdDescriptorSetLayouts = std::move(other.createdDescriptorSetLayouts);
        other.pipelineCache = VK_NULL_HANDLE;
    }
    return *this;
}

void VulkanPipelineWrapper::cleanup() {
    for (VkPipeline pipeline : createdPipelines) {
        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline, nullptr);
        }
    }
    createdPipelines.clear();

    for (VkPipelineLayout layout : createdLayouts) {
        if (layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, layout, nullptr);
        }
    }
    createdLayouts.clear();

    for (VkDescriptorSetLayout layout : createdDescriptorSetLayouts) {
        if (layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, layout, nullptr);
        }
    }
    createdDescriptorSetLayouts.clear();

    for (VkShaderModule shaderModule : createdShaderModules) {
        if (shaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, shaderModule, nullptr);
        }
    }
    createdShaderModules.clear();

    if (pipelineCache != VK_NULL_HANDLE) {
        vkDestroyPipelineCache(device, pipelineCache, nullptr);
        pipelineCache = VK_NULL_HANDLE;
    }
}

bool VulkanPipelineWrapper::InitializePipelineCache(const std::filesystem::path& cacheFilePath) {
    std::vector<char> cacheData;
    bool validCacheData = false;

    if (std::filesystem::exists(cacheFilePath)) {
        std::ifstream file(cacheFilePath, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            std::streamsize fileSize = file.tellg();
            if (fileSize >= static_cast<std::streamsize>(sizeof(PipelineCacheHeader))) {
                file.seekg(0, std::ios::beg);
                cacheData.resize(static_cast<size_t>(fileSize));
                if (file.read(cacheData.data(), fileSize)) {
                    VkPhysicalDeviceProperties properties{};
                    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

                    PipelineCacheHeader header{};
                    std::memcpy(&header, cacheData.data(), sizeof(PipelineCacheHeader));

                    if (header.HeaderLength == sizeof(PipelineCacheHeader) &&
                        header.HeaderVersion == VK_PIPELINE_CACHE_HEADER_VERSION_ONE &&
                        header.VendorId == properties.vendorID &&
                        header.DeviceId == properties.deviceID &&
                        std::memcmp(header.PipelineCacheUuid, properties.pipelineCacheUUID, VK_UUID_SIZE) == 0) {
                        validCacheData = true;
                    }
                }
            }
        }
    }

    VkPipelineCacheCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    if (validCacheData) {
        createInfo.initialDataSize = cacheData.size();
        createInfo.pInitialData = cacheData.data();
    }

    return vkCreatePipelineCache(device, &createInfo, nullptr, &pipelineCache) == VK_SUCCESS;
}

bool VulkanPipelineWrapper::SavePipelineCache(const std::filesystem::path& cacheFilePath) const {
    if (pipelineCache == VK_NULL_HANDLE) {
        return false;
    }

    size_t dataSize = 0;
    if (vkGetPipelineCacheData(device, pipelineCache, &dataSize, nullptr) != VK_SUCCESS || dataSize == 0) {
        return false;
    }

    std::vector<char> cacheData(dataSize);
    if (vkGetPipelineCacheData(device, pipelineCache, &dataSize, cacheData.data()) != VK_SUCCESS) {
        return false;
    }

    if (cacheFilePath.has_parent_path()) {
        std::filesystem::create_directories(cacheFilePath.parent_path());
    }

    std::ofstream file(cacheFilePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    file.write(cacheData.data(), static_cast<std::streamsize>(dataSize));
    return file.good();
}

VkShaderModule VulkanPipelineWrapper::CreateShaderModule(const std::vector<uint32_t>& spirvBytecode) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvBytecode.size() * sizeof(uint32_t);
    createInfo.pCode = spirvBytecode.data();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    createdShaderModules.push_back(shaderModule);
    return shaderModule;
}

void VulkanPipelineWrapper::DestroyShaderModule(VkShaderModule shaderModule) {
    if (shaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, shaderModule, nullptr);
        std::erase(createdShaderModules, shaderModule);
    }
}

VkDescriptorSetLayout VulkanPipelineWrapper::CreateStandardDescriptorSetLayout(
    uint32_t bindingCount,
    VkDescriptorType descriptorType,
    VkShaderStageFlags stageFlags) {
    std::vector<VkDescriptorSetLayoutBinding> bindings(bindingCount);
    for (uint32_t i = 0; i < bindingCount; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorType = descriptorType;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = stageFlags;
        bindings[i].pImmutableSamplers = nullptr;
    }

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = bindingCount;
    createInfo.pBindings = bindings.empty() ? nullptr : bindings.data();

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    createdDescriptorSetLayouts.push_back(layout);
    return layout;
}

void VulkanPipelineWrapper::DestroyDescriptorSetLayout(VkDescriptorSetLayout layout) {
    if (layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, layout, nullptr);
        std::erase(createdDescriptorSetLayouts, layout);
    }
}

VkPipelineLayout VulkanPipelineWrapper::CreatePipelineLayout(
    const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
    const std::vector<VkPushConstantRange>& pushConstantRanges) {
    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    createInfo.pSetLayouts = descriptorSetLayouts.empty() ? nullptr : descriptorSetLayouts.data();
    createInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    createInfo.pPushConstantRanges = pushConstantRanges.empty() ? nullptr : pushConstantRanges.data();

    VkPipelineLayout layout = VK_NULL_HANDLE;
    if (vkCreatePipelineLayout(device, &createInfo, nullptr, &layout) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    createdLayouts.push_back(layout);
    return layout;
}

VkPipeline VulkanPipelineWrapper::CompileGraphicsPipeline(const PipelineConfiguration& configuration) {
    std::vector<VkPipelineShaderStageCreateInfo> stageInfos;
    std::vector<VkShaderModule> localModules;

    for (const auto& stage : configuration.Stages) {
        VkShaderModule module = CreateShaderModule(stage.SpirvBytecode);
        if (module == VK_NULL_HANDLE) {
            return VK_NULL_HANDLE;
        }
        localModules.push_back(module);

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = stage.Stage;
        stageInfo.module = module;
        stageInfo.pName = stage.EntryPoint.empty() ? "main" : stage.EntryPoint.c_str();
        stageInfos.push_back(stageInfo);
    }

    VkPipelineLayout layout = CreatePipelineLayout(configuration.DescriptorSetLayouts, configuration.PushConstantRanges);
    if (layout == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(configuration.ViewportExtent.width);
    viewport.height = static_cast<float>(configuration.ViewportExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = configuration.ViewportExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(stageInfos.size());
    pipelineInfo.pStages = stageInfos.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = configuration.RenderPass;
    pipelineInfo.subpass = configuration.SubpassIndex;

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    createdPipelines.push_back(pipeline);
    return pipeline;
}

VkPipeline VulkanPipelineWrapper::CompileComputePipeline(
    const std::vector<uint32_t>& computeSpirv,
    const std::vector<VkDescriptorSetLayout>& descriptorLayouts,
    std::string_view entryPoint) {
    VkShaderModule module = CreateShaderModule(computeSpirv);
    if (module == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    VkPipelineLayout layout = CreatePipelineLayout(descriptorLayouts, {});
    if (layout == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    std::string entryPointStr(entryPoint);

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = module;
    pipelineInfo.stage.pName = entryPointStr.c_str();
    pipelineInfo.layout = layout;

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (vkCreateComputePipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    createdPipelines.push_back(pipeline);
    return pipeline;
}

void VulkanPipelineWrapper::WarmupPipelineCache(const std::vector<PipelineConfiguration>& configurations) {
    for (const auto& config : configurations) {
        CompileGraphicsPipeline(config);
    }
}

} // namespace Aot::Graphics
