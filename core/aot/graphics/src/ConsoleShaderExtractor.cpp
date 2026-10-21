#include <graphics/ConsoleShaderExtractor.hpp>
#include <fstream>
#include <cstring>

namespace Aot::Graphics {

struct RawShaderContainerHeader {
    uint32_t Magic;
    uint32_t Version;
    uint32_t ShaderCount;
    uint32_t DataOffset;
};

struct RawShaderEntry {
    uint32_t Stage;
    uint32_t Reserved;
    uint64_t Hash;
    uint32_t BytecodeOffset;
    uint32_t BytecodeSize;
    uint32_t BindingCount;
    uint32_t PushConstantSize;
};

struct RawBindingEntry {
    uint32_t Binding;
    uint32_t DescriptorSet;
    uint32_t DescriptorType;
    uint32_t Count;
};

std::vector<ExtractedShaderPackage> ConsoleShaderExtractor::ExtractFromFile(const std::filesystem::path& assetPath) {
    std::ifstream file(assetPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return {};
    }

    std::streamsize fileSize = file.tellg();
    if (fileSize < static_cast<std::streamsize>(sizeof(RawShaderContainerHeader))) {
        return {};
    }

    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        return {};
    }

    return ExtractFromMemory(buffer);
}

std::vector<ExtractedShaderPackage> ConsoleShaderExtractor::ExtractFromMemory(std::span<const uint8_t> assetData) {
    if (assetData.size() < sizeof(RawShaderContainerHeader)) {
        return {};
    }

    uint32_t magic = 0;
    std::memcpy(&magic, assetData.data(), sizeof(uint32_t));

    if (magic == 0x30304253 || magic == 0x31304253 || magic == 0x32304253 || magic == 0x53424147) {
        if (assetData.size() >= 24) {
            uint8_t stageByte = assetData[8];
            ConsoleShaderStage stage = ConsoleShaderStage::Vertex;
            if (stageByte == 0) stage = ConsoleShaderStage::Fragment;
            else if (stageByte == 1) stage = ConsoleShaderStage::Vertex;
            else if (stageByte == 2) stage = ConsoleShaderStage::Geometry;
            else if (stageByte == 3) stage = ConsoleShaderStage::Hull;
            else if (stageByte == 4) stage = ConsoleShaderStage::Domain;
            else if (stageByte == 5) stage = ConsoleShaderStage::Compute;

            uint32_t codeSize = 0;
            uint32_t codeOffset = 0;
            std::memcpy(&codeSize, assetData.data() + 16, sizeof(uint32_t));
            std::memcpy(&codeOffset, assetData.data() + 20, sizeof(uint32_t));

            if (codeOffset == 0 || codeOffset >= assetData.size()) {
                codeOffset = 24;
            }
            if (codeSize == 0 || (codeOffset + codeSize > assetData.size())) {
                codeSize = static_cast<uint32_t>(assetData.size() - codeOffset);
            }

            uint64_t hash = 14695981039346656037ULL;
            for (size_t b = 0; b < codeSize; ++b) {
                hash ^= assetData[codeOffset + b];
                hash *= 1099511628211ULL;
            }

            ExtractedShaderPackage package{};
            package.Stage = stage;
            package.ShaderHash = hash;
            package.PushConstantSize = 128;
            package.RawBytecode.assign(
                assetData.data() + codeOffset,
                assetData.data() + codeOffset + codeSize
            );
            return {std::move(package)};
        }
    }

    const auto* header = reinterpret_cast<const RawShaderContainerHeader*>(assetData.data());
    std::vector<ExtractedShaderPackage> result;

    size_t currentOffset = sizeof(RawShaderContainerHeader);

    for (uint32_t i = 0; i < header->ShaderCount; ++i) {
        if (currentOffset + sizeof(RawShaderEntry) > assetData.size()) {
            break;
        }

        const auto* entry = reinterpret_cast<const RawShaderEntry*>(assetData.data() + currentOffset);
        currentOffset += sizeof(RawShaderEntry);

        ExtractedShaderPackage package{};
        package.Stage = static_cast<ConsoleShaderStage>(entry->Stage);
        package.ShaderHash = entry->Hash;
        package.PushConstantSize = entry->PushConstantSize;

        if (entry->BytecodeOffset + entry->BytecodeSize <= assetData.size()) {
            package.RawBytecode.assign(
                assetData.data() + entry->BytecodeOffset,
                assetData.data() + entry->BytecodeOffset + entry->BytecodeSize
            );
        }

        for (uint32_t b = 0; b < entry->BindingCount; ++b) {
            if (currentOffset + sizeof(RawBindingEntry) > assetData.size()) {
                break;
            }

            const auto* rawBinding = reinterpret_cast<const RawBindingEntry*>(assetData.data() + currentOffset);
            currentOffset += sizeof(RawBindingEntry);

            ResourceBindingInfo bindingInfo{
                .Binding = rawBinding->Binding,
                .DescriptorSet = rawBinding->DescriptorSet,
                .DescriptorType = static_cast<VkDescriptorType>(rawBinding->DescriptorType),
                .StageFlags = (package.Stage == ConsoleShaderStage::Compute) ? static_cast<VkShaderStageFlags>(VK_SHADER_STAGE_COMPUTE_BIT) : static_cast<VkShaderStageFlags>(VK_SHADER_STAGE_ALL_GRAPHICS),
                .Count = rawBinding->Count
            };
            package.ResourceBindings.push_back(bindingInfo);
        }

        result.push_back(std::move(package));
    }

    return result;
}

} // namespace Aot::Graphics
