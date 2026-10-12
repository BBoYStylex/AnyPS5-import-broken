#include "Recompiler.hpp"
#include <spirv/unified1/spirv.hpp>
#include <array>
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

constexpr std::uint32_t LayerExportControl = (1u << 18u) | (1u << 21u);
constexpr std::array<std::uint32_t, 6> LayerExport{0x7e080282u, 0xf80000d4u, 0x00040000u, 0xf80008cfu, 0u, 0xbf810000u};
constexpr std::array<std::uint32_t, 3> Capabilities{spv::CapabilityClipDistance, spv::CapabilityShaderViewportIndexLayerEXT, spv::CapabilityShaderLayer};
constexpr std::array<std::string_view, 1> Extensions{"SPV_EXT_shader_viewport_index_layer"};

struct Module {
    std::uint32_t version = 0;
    std::set<std::uint32_t> capabilities;
    std::set<std::string> extensions;
    bool layerOutput = false;
};

Module inspect(const ShaderRecompiler::RecompileResult& result) {
    const auto& words = result.spirv.Words();
    Module module;
    module.version = words[1];
    std::set<std::uint32_t> outputs;
    std::set<std::uint32_t> layers;
    for (std::size_t at = 5; at < words.size() && (words[at] >> 16u) != 0; at += words[at] >> 16u) {
        const auto op = static_cast<spv::Op>(words[at] & 0xffffu);
        const auto count = words[at] >> 16u;
        if (op == spv::OpCapability) module.capabilities.insert(words[at + 1]);
        if (op == spv::OpExtension) {
            std::string name;
            for (std::size_t word = 1; word < count; ++word) {
                for (std::uint32_t shift = 0; shift < 32u; shift += 8u) {
                    const auto character = static_cast<char>((words[at + word] >> shift) & 0xffu);
                    if (character == '\0') break;
                    name.push_back(character);
                }
            }
            module.extensions.insert(name);
        }
        if (op == spv::OpVariable && words[at + 3] == spv::StorageClassOutput) outputs.insert(words[at + 2]);
        if (op == spv::OpDecorate && count == 4 && words[at + 2] == spv::DecorationBuiltIn && words[at + 3] == spv::BuiltInLayer) layers.insert(words[at + 1]);
    }
    for (const auto layer : layers) module.layerOutput = module.layerOutput || outputs.contains(layer);
    return module;
}

ShaderRecompiler::RecompileRequest request(std::uint32_t control, std::uint32_t vulkanVersion, std::uint32_t spirvVersion) {
    using namespace ShaderRecompiler;
    RecompileRequest request{};
    request.shader = {ShaderStage::Vertex, 0x10000u, LayerExport, 0, {}};
    request.context.waveSize = 64;
    request.context.vertex = ShaderVertexStageInfo{};
    request.context.vertex->paClVsOutCntl = control;
    request.target.vulkanVersion = vulkanVersion;
    request.target.spirvVersion = spirvVersion;
    request.target.subgroupSize = 64;
    request.target.supportedCapabilities = Capabilities;
    request.target.supportedExtensions = Extensions;
    request.layout.pushConstantSizeBytes = 128;
    request.useCache = false;
    return request;
}

void require(bool condition, const char* what) {
    if (!condition) throw std::runtime_error(what);
}

} // namespace

int main() {
    try {
        const auto extension = inspect(ShaderRecompiler::Recompile(request(LayerExportControl, 0x00401000u, 0x00010300u)));
        require(extension.layerOutput, "the SPIR-V 1.3 module declares no Layer output for the render target index export");
        require(extension.version == 0x00010300u, "the SPIR-V 1.3 module raised its version for the Layer output");
        require(extension.capabilities.contains(spv::CapabilityShaderViewportIndexLayerEXT) && !extension.capabilities.contains(spv::CapabilityShaderLayer), "the SPIR-V 1.3 module does not take the Layer output from SPV_EXT_shader_viewport_index_layer");
        require(extension.extensions.contains("SPV_EXT_shader_viewport_index_layer"), "the SPIR-V 1.3 module does not declare SPV_EXT_shader_viewport_index_layer");

        const auto core = inspect(ShaderRecompiler::Recompile(request(LayerExportControl, 0x00402000u, 0x00010500u)));
        require(core.layerOutput, "the SPIR-V 1.5 module declares no Layer output for the render target index export");
        require(core.capabilities.contains(spv::CapabilityShaderLayer) && !core.capabilities.contains(spv::CapabilityShaderViewportIndexLayerEXT), "the SPIR-V 1.5 module does not take the Layer output from ShaderLayer");
        require(!core.extensions.contains("SPV_EXT_shader_viewport_index_layer"), "the SPIR-V 1.5 module declares SPV_EXT_shader_viewport_index_layer");

        const auto plain = inspect(ShaderRecompiler::Recompile(request(1u << 21u, 0x00401000u, 0x00010300u)));
        require(!plain.layerOutput && !plain.capabilities.contains(spv::CapabilityShaderViewportIndexLayerEXT), "a misc vector export without the render target index declares a Layer output");

        std::cout << "Vertex layer exports reach gl_Layer\n";
        return 0;
    } catch (const std::exception& error) {
        const std::string message(error.what());
        std::cerr << message.substr(0, message.find("RecompileRequest:")) << '\n';
        return 1;
    }
}
