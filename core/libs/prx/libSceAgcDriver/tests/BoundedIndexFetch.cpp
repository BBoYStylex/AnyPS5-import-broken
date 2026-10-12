#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Execution/include/VulkanDevice.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Pipeline.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Texture.hpp"
#include "prx/libc/include/GuestAllocations.hpp"
#include "Triangle_vert_spv.h"
#include "Triangle_frag_spv.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif
namespace {
using namespace AgcDriver::Graphics;
constexpr std::uint32_t Width = 128, Height = 64;
alignas(4096) std::array<std::byte, Width * Height * 4> Pixels{};
State MakeState(bool negativeOneToOne) {
    State state{};
    state.stages = {ShaderPath::Vertex, 0, 64, 64, std::nullopt, std::nullopt};
    state.color = {reinterpret_cast<std::uintptr_t>(Pixels.data()), {Width, Height}, VK_FORMAT_R8G8B8A8_UNORM, Pixels.size(), 0xe4u};
    state.colors = {state.color};
    state.hasColorTarget = true;
    state.renderExtent = {Width, Height};
    state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    state.viewport = {0, static_cast<float>(Height), static_cast<float>(Width), -static_cast<float>(Height), 0, 1};
    state.negativeOneToOne = negativeOneToOne;
    state.scissor = {{0, 0}, {Width, Height}};
    state.cullMode = VK_CULL_MODE_NONE;
    state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    state.blend.colorWriteMask = 15;
    state.blends = {state.blend};
    return state;
}

std::vector<std::byte> Render(AgcDriver::VulkanDevice& device, const State& state, std::span<const CompiledShader> shaders, const AgcDriver::Pm4::DrawParameters& draw = {0, 9, 0, 1, 0, false}) {
    std::lock_guard lock(AgcDriver::GuestMemory::GpuMutex());
    std::fill(Pixels.begin(), Pixels.end(), std::byte{0});
    AgcDriver::GuestMemory::BumpCollectEpoch();
    device.Draw(state, draw, shaders);
    StorageTexture::FlushPending(state.color.address, state.color.bytes, nullptr, "bounded index regression");
    device.WaitIdle();
    return {Pixels.begin(), Pixels.end()};
}

class GuardedGpuIndices {
public:
    GuardedGpuIndices() {
#ifdef _WIN32
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        pageSize = info.dwPageSize;
        memory = static_cast<std::byte*>(VirtualAlloc(nullptr, pageSize * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        Require(memory != nullptr, "GPU index guard allocation failed");
        DWORD previous = 0;
        Require(VirtualProtect(memory + pageSize, pageSize, PAGE_NOACCESS, &previous) != 0, "GPU index guard protection failed");
#else
        pageSize = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
        auto* mapped = mmap(nullptr, pageSize * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        Require(mapped != MAP_FAILED, "GPU index guard allocation failed");
        memory = static_cast<std::byte*>(mapped);
        Require(mprotect(memory + pageSize, pageSize, PROT_NONE) == 0, "GPU index guard protection failed");
#endif
        GuestAllocations::Mutation mutation;
        mutation.Add(memory, pageSize, true, true, true);
    }
    ~GuardedGpuIndices() {
        {
            GuestAllocations::Mutation mutation;
            mutation.Remove(memory);
        }
#ifdef _WIN32
        VirtualFree(memory, 0, MEM_RELEASE);
#else
        munmap(memory, pageSize * 2);
#endif
    }
    std::byte* Tail(std::size_t bytes) const { return memory + pageSize - bytes; }
private:
    std::byte* memory = nullptr;
    std::size_t pageSize = 0;
};

void CheckBoundedIndexGpu(AgcDriver::VulkanDevice& device, const ShaderRecompiler::RecompileResult& fragment) {
    ShaderRecompiler::RecompileResult vertex;
    vertex.spirv = std::vector<std::uint32_t>(std::begin(TRIANGLE_vert_SPV), std::end(TRIANGLE_vert_SPV));
    const std::array<CompiledShader, 2> shaders{{
        {ShaderRecompiler::ShaderStage::Vertex, &vertex, 0},
        {ShaderRecompiler::ShaderStage::Fragment, &fragment, 0}}};
    auto state = MakeState(false);
    state.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    constexpr std::uint32_t requested = 42;
    for (const auto size : {2u, 4u}) {
        for (const auto readable : {0u, 1u, 3u, requested}) {
            GuardedGpuIndices guarded;
            std::vector<std::byte> explicitIndices(requested * size, std::byte{0});
            for (std::uint32_t index = 0; index < readable; ++index) {
                const std::uint32_t value = index % 2u + 1u;
                std::memcpy(explicitIndices.data() + index * size, &value, size);
            }
            auto* source = guarded.Tail(readable * size);
            if (readable != 0) std::memcpy(source, explicitIndices.data(), readable * size);
            AgcDriver::Pm4::DrawParameters limited{reinterpret_cast<std::uintptr_t>(source), requested, size, 1, 0};
            limited.readableIndexCount = readable;
            AgcDriver::Pm4::DrawParameters reference{reinterpret_cast<std::uintptr_t>(explicitIndices.data()), requested, size, 1, 0};
            {
                GuestAllocations::Mutation mutation;
                mutation.Add(explicitIndices.data(), explicitIndices.size(), true, true, true);
            }
            const auto expected = Render(device, state, shaders, reference);
            const auto actual = Render(device, state, shaders, limited);
            Require(actual == expected, "bounded index rendering differed from explicit zero padding");
            if (readable == 1) {
                Require(std::any_of(expected.begin(), expected.end(), [](std::byte value) { return value != std::byte{0}; }), "limited index fixture drew no visible line");
                auto truncated = reference;
                truncated.indexCount = 1;
                Require(Render(device, state, shaders, truncated) != expected, "bounded index fixture cannot distinguish dropping requested invocations");
            }
            {
                GuestAllocations::Mutation mutation;
                mutation.Remove(explicitIndices.data());
            }
        }
        GuardedGpuIndices guarded;
        std::vector<std::byte> explicitIndices(requested * size, std::byte{0});
        const auto restart = size == 2 ? 0xffffu : 0xffffffffu;
        std::memcpy(explicitIndices.data(), &restart, size);
        const std::uint32_t vertexOne = 1;
        std::memcpy(explicitIndices.data() + size, &vertexOne, size);
        auto* source = guarded.Tail(2 * size);
        std::memcpy(source, explicitIndices.data(), 2 * size);
        AgcDriver::Pm4::DrawParameters limited{reinterpret_cast<std::uintptr_t>(source), requested, size, 1, 0};
        limited.readableIndexCount = 2;
        AgcDriver::Pm4::DrawParameters reference{reinterpret_cast<std::uintptr_t>(explicitIndices.data()), requested, size, 1, 0};
        state.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        state.primitiveRestart = true;
        {
            GuestAllocations::Mutation mutation;
            mutation.Add(explicitIndices.data(), explicitIndices.size(), true, true, true);
        }
        const auto expected = Render(device, state, shaders, reference);
        Require(std::any_of(expected.begin(), expected.end(), [](std::byte value) { return value != std::byte{0}; }), "restart zero-tail fixture drew no visible line");
        Require(Render(device, state, shaders, limited) == expected, "out-of-range zero indices became primitive restart markers");
        {
            GuestAllocations::Mutation mutation;
            mutation.Remove(explicitIndices.data());
        }
        state.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        state.primitiveRestart = false;
    }
}

}
int main() {
    try {
        ShaderRecompiler::RecompileResult fragment;
        fragment.spirv = std::vector<std::uint32_t>(std::begin(TRIANGLE_frag_SPV), std::end(TRIANGLE_frag_SPV));
        std::unique_ptr<AgcDriver::VulkanDevice> device;
        try { device = std::make_unique<AgcDriver::VulkanDevice>(); }
        catch (const std::exception& error) {
            if (std::getenv("ANYPS5_REQUIRE_VULKAN")) throw;
            std::cout << "GPU skipped: " << error.what() << '\n';
            return 77;
        }
        {
            GuestAllocations::Mutation mutation;
            mutation.Add(Pixels.data(), Pixels.size(), true, true, true);
        }
        CheckBoundedIndexGpu(*device, fragment);
        device->WaitIdle();
        device.reset();
        {
            GuestAllocations::Mutation mutation;
            mutation.Remove(Pixels.data());
        }
        std::cout << "bounded index zero-fill rendering passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
