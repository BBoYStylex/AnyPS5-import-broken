#include "prx/libSceAgcDriver/Execution/include/VulkanDevice.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Draw.hpp"
#include "Recompiler.hpp"
#include "VulkanTestDevice.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

namespace {

using AgcDriver::Graphics::Require;
using ShaderRecompiler::ShaderStage;

constexpr std::uint32_t Extent = 64u;
alignas(256) std::array<std::byte, Extent * Extent * 4u> pixels{};
alignas(256) std::array<std::byte, 256> vertices{};
alignas(256) std::array<std::byte, 256> movedVertices{};
constexpr std::array<std::array<float, 4>, 3> positions{{{-1.0f, -1.0f, 0.0f, 1.0f}, {3.0f, -1.0f, 0.0f, 1.0f}, {-1.0f, 3.0f, 0.0f, 1.0f}}};
constexpr std::array<std::uint32_t, 11> vertexCode{0xf4080100u, 0xfa000000u, 0x4a0a0a02u, 0x4a0a0b08u, 0x4a0a0a03u, 0xe00c2000u, 0x80010005u, 0xbf8c3f70u, 0xf80008cfu, 0x03020100u, 0xbf810000u};
constexpr std::array<std::uint32_t, 5> pixelCode{0x7e0002f2u, 0x7e020280u, 0xf800180fu, 0x00010100u, 0xbf810000u};
void Run(AgcDriver::VulkanDevice& device) {
    ShaderRecompiler::RecompileResult incompatible;
    incompatible.runtimeAbiVersion = ShaderRecompiler::RuntimeAbi::Version + 1u;
    const auto rejectAbi = [](auto action) {
        try {
            action();
        } catch (const std::runtime_error& error) {
            Require(std::string(error.what()).find("incompatible version") != std::string::npos, "unexpected runtime ABI validation error");
            return;
        }
        throw std::runtime_error("driver accepted an incompatible shader runtime ABI");
    };
    rejectAbi([&] { device.PrepareDispatch(incompatible, {}); });
    rejectAbi([&] { device.Dispatch(incompatible, 1u, 1u, 1u); });
    rejectAbi([&] { device.DispatchIndirect(incompatible, 0u); });
    const std::array<AgcDriver::Graphics::CompiledShader, 1> incompatibleShaders{{{ShaderStage::Vertex, &incompatible, 0u}}};
    rejectAbi([&] { device.Draw({}, {}, incompatibleShaders); });
    ShaderRecompiler::RecompileResult invalidSpirv;
    invalidSpirv.spirv = {0, 1, 2};
    invalidSpirv.variantId = 999;
    auto invalidPrepared = device.PrepareDispatch(invalidSpirv, {});
    Require(invalidPrepared == nullptr, "PrepareDispatch should return nullptr for invalid SPIR-V");
    Require(!device.HasComputePipeline(invalidSpirv.PipelineVariantId()), "invalid SPIR-V should not be cached in compute pipelines");
    ShaderRecompiler::RecompileResult computeShader;
    computeShader.spirv = {
        0x07230203u, 0x00010000u, 0u, 7u, 0u,
        0x00020011u, 1u,
        0x0003000eu, 0u, 1u,
        0x0005000fu, 5u, 3u, 0x6e69616du, 0u,
        0x00060010u, 3u, 17u, 1u, 1u, 1u,
        0x00040047u, 6u, 1u, 7u,
        0x00020013u, 1u,
        0x00030021u, 2u, 1u,
        0x00040015u, 5u, 32u, 0u,
        0x00040032u, 5u, 6u, 1u,
        0x00050036u, 1u, 3u, 0u, 2u,
        0x000200f8u, 4u,
        0x000100fdu,
        0x00010038u
    };
    computeShader.variantId = 42;
    computeShader.specialization = {{7u, 1u}};
    const auto countBefore = device.ComputePipelineCount();
    Require(!device.HasComputePipeline(computeShader.PipelineVariantId()), "pipeline should not be cached before prewarm");
    auto prepared1 = device.PrepareDispatch(computeShader, {});
    Require(prepared1 != nullptr, "PrepareDispatch returned null");
    Require(device.HasComputePipeline(computeShader.PipelineVariantId()), "PrepareDispatch did not pre-warm compute pipeline");
    Require(device.ComputePipelineCount() == countBefore + 1, "unexpected pipeline count after prewarm");
    auto prepared2 = device.PrepareDispatch(computeShader, {});
    Require(prepared2 != nullptr, "PrepareDispatch returned null on cached variant");
    Require(device.ComputePipelineCount() == countBefore + 1, "PrepareDispatch re-created warmed pipeline");
    device.PrewarmComputePipeline(computeShader);
    Require(device.ComputePipelineCount() == countBefore + 1, "PrewarmComputePipeline re-created warmed pipeline");
    std::array<std::uint32_t, 4> descriptor{};
    const auto tableAddress = reinterpret_cast<std::uintptr_t>(descriptor.data());
    std::array<std::uint32_t, 4> userData{static_cast<std::uint32_t>(tableAddress), static_cast<std::uint32_t>(tableAddress >> 32u), 1u, 1u};
    const std::array<ShaderRecompiler::MemoryRegion, 1> memory{{{tableAddress, std::as_bytes(std::span(descriptor))}}};
    ShaderRecompiler::ShaderVertexStageInfo vertexInfo{};
    vertexInfo.fetchEmbedded = true;
    vertexInfo.resourcesNum = 1u;
    vertexInfo.resourcesDst[0] = {0, 4, 0, 0};
    ShaderRecompiler::RecompileRequest request{{ShaderStage::Vertex, reinterpret_cast<std::uintptr_t>(vertexCode.data()), vertexCode, 0, {}}, {32u, 0u, userData, std::nullopt, std::nullopt, vertexInfo, memory}, device.Target(), {0u, 0u, 0u, 96u}};
    ShaderRecompiler::RecompileResult first;
    struct Case {
        std::uint32_t stride;
        std::uint32_t format;
        bool moved;
        std::uint32_t offset;
        bool specializedPixel = false;
    };
    constexpr std::array<Case, 5> cases{{{16u, 77u, false, 1u}, {32u, 77u, true, 0u}, {8u, 71u, false, 1u}, {16u, 71u, true, 0u}, {16u, 77u, false, 1u, true}}};
    for (const auto& value : cases) {
        auto& storage = value.moved ? movedVertices : vertices;
        storage.fill(std::byte{0});
        userData[2] = value.offset;
        const auto firstRecord = value.offset + userData[3] + 2u;
        for (std::size_t index = 0; index < positions.size(); ++index) {
            auto* destination = storage.data() + (firstRecord + index) * value.stride;
            if (value.format == 77u) std::memcpy(destination, positions[index].data(), 16u);
            else {
                for (std::size_t component = 0; component < 4u; ++component) {
                    const auto scalar = positions[index][component];
                    const std::uint16_t half = scalar == -1.0f ? 0xbc00u : scalar == 3.0f ? 0x4200u : scalar == 1.0f ? 0x3c00u : 0u;
                    std::memcpy(destination + component * 2u, &half, sizeof(half));
                }
            }
        }
        const auto address = reinterpret_cast<std::uintptr_t>(storage.data());
        descriptor = {static_cast<std::uint32_t>(address), static_cast<std::uint32_t>(address >> 32u) | (value.stride << 16u), 8u, 0x01000facu | (value.format << 12u)};
        request.context.vertex->resources[0].fields = descriptor;
        const auto vertex = ShaderRecompiler::Recompile(request);
        Require(vertex.vertexInputs.empty() && vertex.vertexAttributes.empty(), "runtime vertex fetch created Vulkan vertex attributes");
        Require(vertex.vertexOffsetSgpr == -1 && vertex.instanceOffsetSgpr == -1, "runtime vertex fetch requested draw offset folding");
        Require(vertex.runtimeAbiVersion == ShaderRecompiler::RuntimeAbi::Version, "runtime vertex fetch has an incompatible runtime ABI");
        if (first.spirv.empty()) first = vertex;
        else Require(vertex.cacheHit && vertex.variantId == first.variantId, "runtime vertex descriptor changed the artifact");
        const auto pushBytes = static_cast<std::uint32_t>(vertex.pushConstants.size());
        ShaderRecompiler::ShaderPixelStageInfo pixelInfo{};
        pixelInfo.wave32 = true;
        pixelInfo.targetOutputMode[0] = 9u;
        pixelInfo.targetExportMapping.fill(0xe4u);
        ShaderRecompiler::RecompileRequest pixelRequest{{ShaderStage::Fragment, reinterpret_cast<std::uintptr_t>(pixelCode.data()), pixelCode, 0, {}}, {32u, 0u, {}, std::nullopt, pixelInfo, std::nullopt, {}}, device.Target(), PixelPushLayout(pushBytes, device.Target())};
        if (value.specializedPixel) pixelRequest.target.registrationWordLimit = 1u;
        const auto pixel = ShaderRecompiler::Recompile(pixelRequest);
        if (device.Target().registrationModules) Require(vertex.registrationModule && pixel.registrationModule != value.specializedPixel, "registration modules: the word limit did not choose the module mode");
        const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderStage::Vertex, &vertex, 0u}, {ShaderStage::Fragment, &pixel, PixelPushOffset(pushBytes, device.Target())}}};
        AgcDriver::Graphics::State state{};
        state.stages = {AgcDriver::Graphics::ShaderPath::Vertex, 0u, 32u, 32u, std::nullopt, std::nullopt};
        state.color = {reinterpret_cast<std::uintptr_t>(pixels.data()), {Extent, Extent}, VK_FORMAT_R8G8B8A8_UNORM, pixels.size(), 0xe4u};
        state.colors = {state.color};
        state.hasColorTarget = true;
        state.renderExtent = {Extent, Extent};
        state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        state.viewport = {0.0f, static_cast<float>(Extent), static_cast<float>(Extent), -static_cast<float>(Extent), 0.0f, 1.0f};
        state.scissor = {{0, 0}, {Extent, Extent}};
        state.cullMode = VK_CULL_MODE_NONE;
        state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        state.blend.colorWriteMask = 15u;
        state.blends = {state.blend};
        AgcDriver::Pm4::DrawParameters draw{0u, 3u, 0u, 1u, 0u, false};
        draw.firstVertex = 1u;
        draw.firstInstance = 1u;
        pixels.fill(std::byte{0});
        device.Draw(state, draw, shaders);
        device.WaitIdle();
        for (std::size_t index = 0; index < pixels.size(); index += 4u) {
            Require(pixels[index] == std::byte{255} && pixels[index + 1u] == std::byte{0} && pixels[index + 2u] == std::byte{0} && pixels[index + 3u] == std::byte{255}, "runtime vertex fetch produced the wrong triangle");
        }
    }
}

}

int main() {
    try {
        const auto device = OpenVulkanTestDevice();
        if (!device) return VulkanTestSkipped;
        Run(*device);
        std::cout << "runtime vertex fetch tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
