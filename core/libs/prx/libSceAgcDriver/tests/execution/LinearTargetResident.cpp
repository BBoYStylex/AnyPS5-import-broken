#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Execution/include/VulkanDevice.hpp"
#include "prx/libSceAgcDriver/Graphics/include/ColorTargetLayout.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Draw.hpp"
#include "prx/libSceAgcDriver/Graphics/include/GuestBufferMemory.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Recorder.hpp"
#include "prx/libSceAgcDriver/Graphics/include/State.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Texture.hpp"
#include "prx/libc/include/GuestAllocations.hpp"
#include "prx/libc/include/GuestArena.hpp"
#include "prx/libc/include/GuestWriteWatch.hpp"
#include "Recompiler.hpp"
#include "VulkanTestDevice.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace AgcDriver {

struct VulkanDeviceTestAccess {
    static Graphics::Context ContextOf(const VulkanDevice& device) { return device.graphicsContext(); }
};

}

namespace {

using AgcDriver::Graphics::Require;
using ShaderRecompiler::ShaderStage;

constexpr std::uint32_t Width = 64;
constexpr std::uint32_t Height = 16;
constexpr std::size_t BlockBytes = 65536;
constexpr std::uint32_t Rgba32FloatInfo = 0x8738u;
constexpr std::uint32_t LinearAttrib3 = 0x09000000u;
constexpr std::uint32_t Float32AbgrExport = 9;
constexpr std::uint32_t NoInput = 0xffffffffu;

alignas(256) constexpr std::array<std::uint32_t, 6> VertexCode{
    0xe0382000, 0x80000005, 0xbf8c3f70, 0xf80008cf, 0x03020100, 0xbf810000,
};

constexpr std::array<std::array<float, 4>, 3> Triangle{{
    {-1.0f, -1.0f, 0.5f, 1.0f}, {3.0f, -1.0f, 0.5f, 1.0f}, {-1.0f, 3.0f, 0.5f, 1.0f}
}};

alignas(256) std::array<std::array<std::uint32_t, 16>, 64> Programs{};
std::size_t programCount = 0;

std::array<std::uint32_t, 4> BufferDescriptor(const void* data, std::uint32_t stride, std::uint32_t count) {
    const auto address = reinterpret_cast<std::uintptr_t>(data);
    return {static_cast<std::uint32_t>(address), static_cast<std::uint32_t>((address >> 32u) & 0xffffu) | (stride << 16u), count, 0x01016facu};
}

std::span<const std::uint32_t> Program(std::uint32_t fixedVgpr, float blue = 0.0f) {
    Require(programCount < Programs.size(), "too many test programs");
    auto& code = Programs[programCount++];
    std::size_t at = 0;
    code[at++] = 0x36000000u | (20u << 17u) | (fixedVgpr << 9u) | 0xffu;
    code[at++] = 0x0000ffffu;
    code[at++] = 0x2c000000u | (21u << 17u) | (fixedVgpr << 9u) | 0x90u;
    code[at++] = 0x7e000c00u | (20u << 17u) | (256u + 20u);
    code[at++] = 0x7e000c00u | (21u << 17u) | (256u + 21u);
    code[at++] = 0x7e0002ffu | (22u << 17u);
    code[at++] = std::bit_cast<std::uint32_t>(blue);
    code[at++] = 0x7e0002f2u | (23u << 17u);
    code[at++] = 0xf800180fu;
    code[at++] = 0x17161514u;
    code[at++] = 0xbf810000u;
    return std::span<const std::uint32_t>(code.data(), at);
}

std::span<const std::uint32_t> ConstantProgram(float r, float g, float b, float a) {
    Require(programCount < Programs.size(), "too many test programs");
    auto& code = Programs[programCount++];
    std::size_t at = 0;
    code[at++] = 0x7e0002ffu | (20u << 17u);
    code[at++] = std::bit_cast<std::uint32_t>(r);
    code[at++] = 0x7e0002ffu | (21u << 17u);
    code[at++] = std::bit_cast<std::uint32_t>(g);
    code[at++] = 0x7e0002ffu | (22u << 17u);
    code[at++] = std::bit_cast<std::uint32_t>(b);
    code[at++] = 0x7e0002ffu | (23u << 17u);
    code[at++] = std::bit_cast<std::uint32_t>(a);
    code[at++] = 0xf800180fu;
    code[at++] = 0x17161514u;
    code[at++] = 0xbf810000u;
    return std::span<const std::uint32_t>(code.data(), at);
}

VkPipelineColorBlendAttachmentState DefaultBlend() {
    VkPipelineColorBlendAttachmentState blend{};
    blend.blendEnable = VK_FALSE;
    blend.colorWriteMask = 0xf;
    return blend;
}

VkPipelineColorBlendAttachmentState Blend(VkBlendFactor source, VkBlendFactor destination, VkBlendOp operation, VkColorComponentFlags mask = 0xf) {
    VkPipelineColorBlendAttachmentState blend{};
    blend.blendEnable = VK_TRUE;
    blend.srcColorBlendFactor = source;
    blend.dstColorBlendFactor = destination;
    blend.colorBlendOp = operation;
    blend.srcAlphaBlendFactor = source;
    blend.dstAlphaBlendFactor = destination;
    blend.alphaBlendOp = operation;
    blend.colorWriteMask = mask;
    return blend;
}

VkPipelineColorBlendAttachmentState MaskedBlend(VkColorComponentFlags mask) {
    VkPipelineColorBlendAttachmentState blend{};
    blend.blendEnable = VK_FALSE;
    blend.colorWriteMask = mask;
    return blend;
}

struct Block {
    explicit Block(std::size_t bytes) : bytes(bytes), watched(AgcDriver::GuestMemory::WriteWatched()) {
#ifdef _WIN32
        if (watched) {
            data = static_cast<std::uint8_t*>(GuestArena::GuestArenaAllocate_nid_postfix(bytes, BlockBytes));
            GuestArena::GuestArenaCommit_nid_postfix(data, bytes, PAGE_READWRITE, bytes);
        } else {
            data = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        }
#else
        if (watched) {
            void* raw = mmap(nullptr, bytes + BlockBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            Require(raw != MAP_FAILED, "cannot map the color target");
            const auto begin = reinterpret_cast<std::uintptr_t>(raw);
            const auto aligned = (begin + BlockBytes - 1) & ~(static_cast<std::uintptr_t>(BlockBytes) - 1);
            if (aligned != begin) munmap(raw, aligned - begin);
            if (aligned + bytes != begin + bytes + BlockBytes) munmap(reinterpret_cast<void*>(aligned + bytes), begin + BlockBytes - aligned);
            data = reinterpret_cast<std::uint8_t*>(aligned);
            GuestWriteWatch::GuestWriteWatchRegister_nid_postfix(data, bytes);
        } else {
            data = static_cast<std::uint8_t*>(std::aligned_alloc(BlockBytes, bytes));
        }
#endif
        Require(data != nullptr, "cannot allocate the color target");
        Require(!watched || AgcDriver::GuestMemory::Watched(Address(), bytes), "the color target is not write-watched");
        GuestAllocations::Mutation mutation;
        mutation.Add(data, bytes, true, true, true);
    }
    ~Block() {
        AgcDriver::Graphics::StorageTexture::FlushPending(Address(), bytes, nullptr, "test release");
        {
            GuestAllocations::Mutation mutation;
            mutation.Remove(data);
        }
#ifdef _WIN32
        if (watched) {
            GuestArena::GuestArenaReset_nid_postfix(data, bytes);
            GuestArena::GuestArenaRelease_nid_postfix(data, bytes);
        } else {
            VirtualFree(data, 0, MEM_RELEASE);
        }
#else
        if (watched) {
            munmap(data, bytes);
            GuestWriteWatch::GuestWriteWatchUnregister_nid_postfix(data, bytes);
        } else {
            std::free(data);
        }
#endif
    }
    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;
    std::uint64_t Address() const { return reinterpret_cast<std::uintptr_t>(data); }
    std::size_t bytes;
    bool watched;
    std::uint8_t* data = nullptr;
};

AgcDriver::Graphics::ColorTarget DecodeTarget(const Block& block, std::uint32_t info) {
    AgcDriver::Registers cx;
    cx[0x318] = static_cast<std::uint32_t>(block.Address() >> 8u);
    cx[0x31b] = 0;
    cx[0x31c] = info;
    cx[0x31d] = 0;
    cx[0x323] = 0;
    cx[0x324] = 0;
    cx[0x3b0] = ((Width - 1u) << 14u) | (Height - 1u);
    cx[0x3b8] = LinearAttrib3;
    cx[0x390] = static_cast<std::uint32_t>(block.Address() >> 40u);
    auto color = AgcDriver::Graphics::DecodeColorBuffer(cx, 0);
    color.exportIndex = 0;
    return color;
}

void Draw(AgcDriver::VulkanDevice& device, const AgcDriver::Graphics::ColorTarget& color, std::uint32_t inputAddr, std::span<const std::uint32_t> pixelCode, VkRect2D scissor = {{0, 0}, {Width, Height}}, VkPipelineColorBlendAttachmentState blend = DefaultBlend()) {
    const auto target = device.Target();
    constexpr std::uint32_t waveSize = 64;
    std::vector<std::uint32_t> vertexUserData(4, 0u);
    const auto vertexBuffer = BufferDescriptor(Triangle.data(), 16u, static_cast<std::uint32_t>(Triangle.size()));
    std::copy(vertexBuffer.begin(), vertexBuffer.end(), vertexUserData.begin());
    const std::array<ShaderRecompiler::MemoryRegion, 1> vertexMemory{{{reinterpret_cast<std::uintptr_t>(VertexCode.data()), std::as_bytes(std::span(VertexCode))}}};
    ShaderRecompiler::RecompileRequest vertex{
        {ShaderStage::Vertex, reinterpret_cast<std::uintptr_t>(VertexCode.data()), VertexCode, 0, {}},
        {waveSize, 0, vertexUserData, std::nullopt, std::nullopt, ShaderRecompiler::ShaderVertexStageInfo{}, vertexMemory},
        target,
        {0, 0, 0, 64}
    };
    vertex.useCache = false;
    const auto vertexResult = ShaderRecompiler::Recompile(vertex);
    const auto vertexPush = static_cast<std::uint32_t>(vertexResult.pushConstants.size());

    ShaderRecompiler::ShaderPixelStageInfo pixel{};
    pixel.inputAddr = inputAddr;
    pixel.targetOutputMode.at(0) = Float32AbgrExport;
    pixel.targetExportMapping.fill(0xe4u);
    const std::vector<std::uint32_t> pixelUserData(8, 0u);
    const std::array<ShaderRecompiler::MemoryRegion, 1> pixelMemory{{{reinterpret_cast<std::uintptr_t>(pixelCode.data()), std::as_bytes(pixelCode)}}};
    ShaderRecompiler::RecompileRequest fragment{
        {ShaderStage::Fragment, reinterpret_cast<std::uintptr_t>(pixelCode.data()), pixelCode, 0, {}},
        {waveSize, 0, pixelUserData, std::nullopt, pixel, std::nullopt, pixelMemory},
        target,
        PixelPushLayout(vertexPush, target)
    };
    fragment.useCache = false;
    const auto pixelResult = ShaderRecompiler::Recompile(fragment);
    const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{
        {ShaderStage::Vertex, &vertexResult, 0},
        {ShaderStage::Fragment, &pixelResult, PixelPushOffset(vertexPush, target)}
    }};

    AgcDriver::Graphics::State state{};
    state.stages = {AgcDriver::Graphics::ShaderPath::Vertex, 0u, waveSize, waveSize, std::nullopt, std::nullopt};
    state.colors = {color};
    state.color = color;
    state.hasColorTarget = true;
    state.renderExtent = {Width, Height};
    state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    state.viewport = {0, static_cast<float>(Height), static_cast<float>(Width), -static_cast<float>(Height), 0, 1};
    state.negativeOneToOne = false;
    state.scissor = scissor;
    state.cullMode = VK_CULL_MODE_NONE;
    state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    state.blends = {blend};
    state.blend = blend;
    state.blendConstants = {};
    const AgcDriver::Pm4::DrawParameters draw{0, static_cast<std::uint32_t>(Triangle.size()), 0, 1, 0, false};
    device.Draw(state, draw, shaders);
}

void Check(const std::vector<std::uint8_t>& stored, float expectedBlue, const std::string& what) {
    for (std::uint32_t y = 0; y < Height; ++y) {
        for (std::uint32_t x = 0; x < Width; ++x) {
            std::array<float, 4> texel{};
            std::memcpy(texel.data(), stored.data() + (static_cast<std::size_t>(y) * Width + x) * 16u, sizeof(texel));
            const std::array<float, 4> expected{static_cast<float>(x), static_cast<float>(y), expectedBlue, 1.0f};
            Require(texel == expected, what + ": pixel (" + std::to_string(x) + ", " + std::to_string(y) + ") holds (" + std::to_string(texel[0]) + ", " + std::to_string(texel[1]) + ", " + std::to_string(texel[2]) + ", " + std::to_string(texel[3]) + ")");
        }
    }
}

void CheckPartition(const std::vector<std::uint8_t>& stored, std::uint32_t splitX, float leftBlue, float rightBlue, const std::string& what) {
    for (std::uint32_t y = 0; y < Height; ++y) {
        for (std::uint32_t x = 0; x < Width; ++x) {
            std::array<float, 4> texel{};
            std::memcpy(texel.data(), stored.data() + (static_cast<std::size_t>(y) * Width + x) * 16u, sizeof(texel));
            const float expectedBlue = x < splitX ? leftBlue : rightBlue;
            const std::array<float, 4> expected{static_cast<float>(x), static_cast<float>(y), expectedBlue, 1.0f};
            Require(texel == expected, what + ": pixel (" + std::to_string(x) + ", " + std::to_string(y) + ") holds (" + std::to_string(texel[0]) + ", " + std::to_string(texel[1]) + ", " + std::to_string(texel[2]) + ", " + std::to_string(texel[3]) + ")");
        }
    }
}

bool ExpectSingleDraw(AgcDriver::VulkanDevice& device) {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputBit;
    constexpr std::size_t bytes = std::size_t{Width} * Height * 16u;
    Block block(bytes);
    std::fill(block.data, block.data + bytes, std::uint8_t{0xff});
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    if (AgcDriver::Graphics::HostImportFor(context, block.Address(), bytes) == nullptr) return false;
    const auto color = DecodeTarget(block, Rgba32FloatInfo);
    Require(color.tileMode == AgcDriver::Graphics::ColorTileMode::Linear && color.bytes == bytes, "the target is not a linear R32G32B32A32 target");
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 0.0f));
    const auto* recorder = AgcDriver::Graphics::Recorder::Active();
    Require(recorder != nullptr && !recorder->Idle(), "the draw into an imported linear target waited for the GPU instead of being recorded");
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "the draw into an imported linear target did not leave its results pending");
    device.WaitIdle();
    std::vector<std::uint8_t> stored(bytes);
    AgcDriver::GuestMemory::Read(block.Address(), std::as_writable_bytes(std::span(stored)), 1);
    Check(stored, 0.0f, "after the recorded draw, guest read");
    Require(!AgcDriver::Graphics::StorageTexture::AnyPending(), "the guest read left the linear target results pending");
    std::memcpy(stored.data(), block.data, bytes);
    Check(stored, 0.0f, "after the guest read, raw memory");
    return true;
}

bool ExpectConsecutiveDraws(AgcDriver::VulkanDevice& device) {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputBit;
    constexpr std::size_t bytes = std::size_t{Width} * Height * 16u;
    Block block(bytes);
    std::fill(block.data, block.data + bytes, std::uint8_t{0xff});
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    if (AgcDriver::Graphics::HostImportFor(context, block.Address(), bytes) == nullptr) return false;
    const auto color = DecodeTarget(block, Rgba32FloatInfo);
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 0.0f));
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "first draw did not leave results pending");
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 10.0f), {{0, 0}, {Width / 2u, Height}});
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "second consecutive draw did not leave results pending");
    device.WaitIdle();
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "idle device cleared pending storage texture before access");
    std::vector<std::uint8_t> stored(bytes);
    AgcDriver::GuestMemory::Read(block.Address(), std::as_writable_bytes(std::span(stored)), 1);
    CheckPartition(stored, Width / 2u, 10.0f, 0.0f, "after consecutive recorded draws, guest read");
    Require(!AgcDriver::Graphics::StorageTexture::AnyPending(), "guest read left storage texture pending");
    return true;
}

bool ExpectDrawAcrossBatches(AgcDriver::VulkanDevice& device) {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputBit;
    constexpr std::size_t bytes = std::size_t{Width} * Height * 16u;
    Block block(bytes);
    std::fill(block.data, block.data + bytes, std::uint8_t{0xff});
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    if (AgcDriver::Graphics::HostImportFor(context, block.Address(), bytes) == nullptr) return false;
    const auto color = DecodeTarget(block, Rgba32FloatInfo);
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 1.0f));
    device.WaitIdle();
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "idle after batch 1 discarded pending storage texture");
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 20.0f), {{Width / 2u, 0}, {Width / 2u, Height}});
    device.WaitIdle();
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "idle after batch 2 discarded pending storage texture");
    std::vector<std::uint8_t> stored(bytes);
    AgcDriver::GuestMemory::Read(block.Address(), std::as_writable_bytes(std::span(stored)), 1);
    CheckPartition(stored, Width / 2u, 1.0f, 20.0f, "after draws across batches, guest read");
    Require(!AgcDriver::Graphics::StorageTexture::AnyPending(), "guest read left storage texture pending");
    return true;
}

bool ExpectMultipleSequentialDraws(AgcDriver::VulkanDevice& device) {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputBit;
    constexpr std::size_t bytes = std::size_t{Width} * Height * 16u;
    Block block(bytes);
    std::fill(block.data, block.data + bytes, std::uint8_t{0xff});
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    if (AgcDriver::Graphics::HostImportFor(context, block.Address(), bytes) == nullptr) return false;
    const auto color = DecodeTarget(block, Rgba32FloatInfo);
    constexpr std::uint32_t columns = 4;
    constexpr std::uint32_t colWidth = Width / columns;
    const std::array<float, 4> blues{100.0f, 200.0f, 300.0f, 400.0f};
    for (std::uint32_t i = 0; i < columns; ++i) {
        Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, blues[i]), {{static_cast<std::int32_t>(i * colWidth), 0}, {colWidth, Height}});
    }
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "sequential draws did not leave results pending");
    device.WaitIdle();
    std::vector<std::uint8_t> stored(bytes);
    AgcDriver::GuestMemory::Read(block.Address(), std::as_writable_bytes(std::span(stored)), 1);
    for (std::uint32_t y = 0; y < Height; ++y) {
        for (std::uint32_t x = 0; x < Width; ++x) {
            std::array<float, 4> texel{};
            std::memcpy(texel.data(), stored.data() + (static_cast<std::size_t>(y) * Width + x) * 16u, sizeof(texel));
            const auto expectedBlue = blues[x / colWidth];
            const std::array<float, 4> expected{static_cast<float>(x), static_cast<float>(y), expectedBlue, 1.0f};
            Require(texel == expected, "sequential draw column test pixel mismatch at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        }
    }
    return true;
}

bool ExpectStoreIdleResults(AgcDriver::VulkanDevice& device) {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputBit;
    constexpr std::size_t bytes = std::size_t{Width} * Height * 16u;
    Block block(bytes);
    std::fill(block.data, block.data + bytes, std::uint8_t{0xff});
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    if (AgcDriver::Graphics::HostImportFor(context, block.Address(), bytes) == nullptr) return false;
    const auto color = DecodeTarget(block, Rgba32FloatInfo);
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 5.0f));
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "draw did not leave results pending");
    device.WaitIdle();
    device.StoreIdleResults();
    Require(!AgcDriver::Graphics::StorageTexture::AnyPending(), "StoreIdleResults left results pending");
    device.WaitIdle();
    std::vector<std::uint8_t> stored(bytes);
    std::memcpy(stored.data(), block.data, bytes);
    Check(stored, 5.0f, "after StoreIdleResults, raw memory");
    return true;
}

bool ExpectCpuWritesWithGuestMemory(AgcDriver::VulkanDevice& device) {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputBit;
    constexpr std::size_t bytes = std::size_t{Width} * Height * 16u;
    Block block(bytes);
    std::fill(block.data, block.data + bytes, std::uint8_t{0xff});
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    if (AgcDriver::Graphics::HostImportFor(context, block.Address(), bytes) == nullptr) return false;
    const auto color = DecodeTarget(block, Rgba32FloatInfo);
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 7.0f), {{0, 0}, {Width / 2u, Height}});
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "draw 1 did not leave results pending");
    constexpr std::size_t rowBytes = std::size_t{Width} * 16u;
    constexpr std::size_t drawnBytes = std::size_t{Width / 2u} * 16u;
    const std::vector<std::uint8_t> sentinel(rowBytes - drawnBytes, 0x42u);
    for (std::uint32_t y = 0; y < Height; ++y) {
        AgcDriver::GuestMemory::Write(block.Address() + y * rowBytes + drawnBytes, std::as_bytes(std::span(sentinel)));
    }
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 9.0f), {{0, 0}, {Width / 4u, Height}});
    device.WaitIdle();
    std::vector<std::uint8_t> stored(bytes);
    AgcDriver::GuestMemory::Read(block.Address(), std::as_writable_bytes(std::span(stored)), 1);
    for (std::uint32_t y = 0; y < Height; ++y) {
        for (std::uint32_t x = 0; x < Width; ++x) {
            const auto offset = (static_cast<std::size_t>(y) * Width + x) * 16u;
            if (x >= Width / 2u) {
                Require(std::all_of(stored.data() + offset, stored.data() + offset + 16u, [](std::uint8_t b) { return b == 0x42u; }), "CPU written bytes outside scissor were overwritten");
            } else {
                std::array<float, 4> texel{};
                std::memcpy(texel.data(), stored.data() + offset, sizeof(texel));
                const float expectedBlue = x < Width / 4u ? 9.0f : 7.0f;
                const std::array<float, 4> expected{static_cast<float>(x), static_cast<float>(y), expectedBlue, 1.0f};
                Require(texel == expected, "CPU write test pixel mismatch at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
            }
        }
    }
    return true;
}

bool ExpectDirectCpuWriteAfterDraw(AgcDriver::VulkanDevice& device) {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputBit;
    constexpr std::size_t bytes = std::size_t{Width} * Height * 16u;
    Block block(bytes);
    std::fill(block.data, block.data + bytes, std::uint8_t{0xff});
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    if (AgcDriver::Graphics::HostImportFor(context, block.Address(), bytes) == nullptr) return false;
    const auto color = DecodeTarget(block, Rgba32FloatInfo);
    constexpr std::size_t rowBytes = std::size_t{Width} * 16u;
    constexpr std::size_t drawnBytes = std::size_t{Width / 2u} * 16u;
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 3.0f), {{0, 0}, {Width / 2u, Height}});
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "draw did not leave results pending");
    for (std::uint32_t y = 0; y < Height; ++y) {
        std::memset(block.data + y * rowBytes + drawnBytes, 0x42, rowBytes - drawnBytes);
    }
    device.WaitIdle();
    for (std::uint32_t y = 0; y < Height; ++y) {
        const auto* row = block.data + y * rowBytes;
        Require(std::all_of(row + drawnBytes, row + rowBytes, [](std::uint8_t b) { return b == 0x42u; }), "direct CPU write was overwritten by GPU store");
    }
    return true;
}

bool ExpectAdditiveBlending(AgcDriver::VulkanDevice& device) {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputBit;
    constexpr std::size_t bytes = std::size_t{Width} * Height * 16u;
    Block block(bytes);
    std::fill(block.data, block.data + bytes, std::uint8_t{0xff});
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    if (AgcDriver::Graphics::HostImportFor(context, block.Address(), bytes) == nullptr) return false;
    const auto color = DecodeTarget(block, Rgba32FloatInfo);
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 5.0f));
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "draw 1 did not leave results pending");
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), ConstantProgram(10.0f, 20.0f, 30.0f, 2.0f), {{0, 0}, {Width / 2u, Height}}, Blend(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD));
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "draw 2 did not leave results pending");
    device.WaitIdle();
    std::vector<std::uint8_t> stored(bytes);
    AgcDriver::GuestMemory::Read(block.Address(), std::as_writable_bytes(std::span(stored)), 1);
    for (std::uint32_t y = 0; y < Height; ++y) {
        for (std::uint32_t x = 0; x < Width; ++x) {
            std::array<float, 4> texel{};
            std::memcpy(texel.data(), stored.data() + (static_cast<std::size_t>(y) * Width + x) * 16u, sizeof(texel));
            const std::array<float, 4> expected = x < Width / 2u
                ? std::array<float, 4>{static_cast<float>(x) + 10.0f, static_cast<float>(y) + 20.0f, 35.0f, 3.0f}
                : std::array<float, 4>{static_cast<float>(x), static_cast<float>(y), 5.0f, 1.0f};
            Require(texel == expected, "additive blend mismatch at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        }
    }
    return true;
}

bool ExpectColorWriteMask(AgcDriver::VulkanDevice& device) {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputBit;
    constexpr std::size_t bytes = std::size_t{Width} * Height * 16u;
    Block block(bytes);
    std::fill(block.data, block.data + bytes, std::uint8_t{0xff});
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    if (AgcDriver::Graphics::HostImportFor(context, block.Address(), bytes) == nullptr) return false;
    const auto color = DecodeTarget(block, Rgba32FloatInfo);
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), ConstantProgram(1.0f, 2.0f, 3.0f, 4.0f));
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "draw 1 did not leave results pending");
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), ConstantProgram(99.0f, 99.0f, 55.0f, 99.0f), {{0, 0}, {Width, Height}}, MaskedBlend(VK_COLOR_COMPONENT_B_BIT));
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "draw 2 did not leave results pending");
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), ConstantProgram(77.0f, 88.0f, 99.0f, 99.0f), {{0, 0}, {Width / 2u, Height}}, MaskedBlend(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT));
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "draw 3 did not leave results pending");
    device.WaitIdle();
    std::vector<std::uint8_t> stored(bytes);
    AgcDriver::GuestMemory::Read(block.Address(), std::as_writable_bytes(std::span(stored)), 1);
    for (std::uint32_t y = 0; y < Height; ++y) {
        for (std::uint32_t x = 0; x < Width; ++x) {
            std::array<float, 4> texel{};
            std::memcpy(texel.data(), stored.data() + (static_cast<std::size_t>(y) * Width + x) * 16u, sizeof(texel));
            const std::array<float, 4> expected = x < Width / 2u
                ? std::array<float, 4>{77.0f, 88.0f, 55.0f, 4.0f}
                : std::array<float, 4>{1.0f, 2.0f, 55.0f, 4.0f};
            Require(texel == expected, "color write mask mismatch at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        }
    }
    return true;
}

bool ExpectOverlappingScissors(AgcDriver::VulkanDevice& device) {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputBit;
    constexpr std::size_t bytes = std::size_t{Width} * Height * 16u;
    Block block(bytes);
    std::fill(block.data, block.data + bytes, std::uint8_t{0xff});
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    if (AgcDriver::Graphics::HostImportFor(context, block.Address(), bytes) == nullptr) return false;
    const auto color = DecodeTarget(block, Rgba32FloatInfo);
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 1.0f), {{0, 0}, {48u, Height}});
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 2.0f), {{16, 0}, {48u, Height}});
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 3.0f), {{24, 0}, {16u, Height}});
    device.WaitIdle();
    std::vector<std::uint8_t> stored(bytes);
    AgcDriver::GuestMemory::Read(block.Address(), std::as_writable_bytes(std::span(stored)), 1);
    for (std::uint32_t y = 0; y < Height; ++y) {
        for (std::uint32_t x = 0; x < Width; ++x) {
            std::array<float, 4> texel{};
            std::memcpy(texel.data(), stored.data() + (static_cast<std::size_t>(y) * Width + x) * 16u, sizeof(texel));
            const float expectedBlue = (x >= 24 && x < 40) ? 3.0f : (x >= 16) ? 2.0f : 1.0f;
            const std::array<float, 4> expected{static_cast<float>(x), static_cast<float>(y), expectedBlue, 1.0f};
            Require(texel == expected, "overlapping scissors mismatch at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        }
    }
    return true;
}

bool ExpectMultipleLinearTargets(AgcDriver::VulkanDevice& device) {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputBit;
    constexpr std::size_t bytes = std::size_t{Width} * Height * 16u;
    Block blockA(bytes);
    Block blockB(bytes);
    std::fill(blockA.data, blockA.data + bytes, std::uint8_t{0xff});
    std::fill(blockB.data, blockB.data + bytes, std::uint8_t{0xee});
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    if (AgcDriver::Graphics::HostImportFor(context, blockA.Address(), bytes) == nullptr ||
        AgcDriver::Graphics::HostImportFor(context, blockB.Address(), bytes) == nullptr) return false;
    const auto colorA = DecodeTarget(blockA, Rgba32FloatInfo);
    const auto colorB = DecodeTarget(blockB, Rgba32FloatInfo);
    Draw(device, colorA, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 11.0f));
    Draw(device, colorB, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 22.0f));
    Draw(device, colorA, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 33.0f), {{0, 0}, {Width / 2u, Height}});
    Draw(device, colorB, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 44.0f), {{Width / 2u, 0}, {Width / 2u, Height}});
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "targets were not left pending");
    device.WaitIdle();
    std::vector<std::uint8_t> storedB(bytes);
    AgcDriver::GuestMemory::Read(blockB.Address(), std::as_writable_bytes(std::span(storedB)), 1);
    CheckPartition(storedB, Width / 2u, 22.0f, 44.0f, "multiple targets: target B");
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "reading target B flushed target A prematurely");
    std::vector<std::uint8_t> storedA(bytes);
    AgcDriver::GuestMemory::Read(blockA.Address(), std::as_writable_bytes(std::span(storedA)), 1);
    CheckPartition(storedA, Width / 2u, 33.0f, 11.0f, "multiple targets: target A");
    Require(!AgcDriver::Graphics::StorageTexture::AnyPending(), "pending textures remained after reading both targets");
    return true;
}

bool ExpectPartialGuestRead(AgcDriver::VulkanDevice& device) {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputBit;
    constexpr std::size_t bytes = std::size_t{Width} * Height * 16u;
    Block block(bytes);
    std::fill(block.data, block.data + bytes, std::uint8_t{0xff});
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    if (AgcDriver::Graphics::HostImportFor(context, block.Address(), bytes) == nullptr) return false;
    const auto color = DecodeTarget(block, Rgba32FloatInfo);
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 42.0f));
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "draw did not leave results pending");
    device.WaitIdle();
    std::array<std::byte, 64> partialSlice{};
    AgcDriver::GuestMemory::Read(block.Address(), partialSlice, 1);
    Require(!AgcDriver::Graphics::StorageTexture::AnyPending(), "partial guest read did not flush overlapping storage texture");
    for (std::uint32_t x = 0; x < 4; ++x) {
        std::array<float, 4> texel{};
        std::memcpy(texel.data(), partialSlice.data() + x * 16u, sizeof(texel));
        const std::array<float, 4> expected{static_cast<float>(x), 0.0f, 42.0f, 1.0f};
        Require(texel == expected, "partial read slice pixel (" + std::to_string(x) + ", 0) mismatch");
    }
    std::vector<std::uint8_t> stored(bytes);
    std::memcpy(stored.data(), block.data, bytes);
    Check(stored, 42.0f, "after partial guest read, raw memory");
    return true;
}

bool ExpectEmptyScissorDraw(AgcDriver::VulkanDevice& device) {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputBit;
    constexpr std::size_t bytes = std::size_t{Width} * Height * 16u;
    Block block(bytes);
    std::fill(block.data, block.data + bytes, std::uint8_t{0xff});
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    if (AgcDriver::Graphics::HostImportFor(context, block.Address(), bytes) == nullptr) return false;
    const auto color = DecodeTarget(block, Rgba32FloatInfo);
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 77.0f));
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "draw 1 did not leave results pending");
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 99.0f), {{10, 10}, {0, 0}});
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "empty scissor draw cleared pending state");
    device.WaitIdle();
    std::vector<std::uint8_t> stored(bytes);
    AgcDriver::GuestMemory::Read(block.Address(), std::as_writable_bytes(std::span(stored)), 1);
    Check(stored, 77.0f, "after empty scissor draw");
    return true;
}

bool ExpectTargetReallocationWithoutRead(AgcDriver::VulkanDevice& device) {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputBit;
    constexpr std::size_t bytes = std::size_t{Width} * Height * 16u;
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    {
        Block tempBlock(bytes);
        std::fill(tempBlock.data, tempBlock.data + bytes, std::uint8_t{0xaa});
        if (AgcDriver::Graphics::HostImportFor(context, tempBlock.Address(), bytes) == nullptr) return false;
        const auto color = DecodeTarget(tempBlock, Rgba32FloatInfo);
        Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 13.0f));
        Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "draw in tempBlock did not leave results pending");
        device.WaitIdle();
    }
    Block freshBlock(bytes);
    std::fill(freshBlock.data, freshBlock.data + bytes, std::uint8_t{0xbb});
    if (AgcDriver::Graphics::HostImportFor(context, freshBlock.Address(), bytes) == nullptr) return false;
    const auto freshColor = DecodeTarget(freshBlock, Rgba32FloatInfo);
    Draw(device, freshColor, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 88.0f));
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "draw in freshBlock did not leave results pending");
    device.WaitIdle();
    std::vector<std::uint8_t> stored(bytes);
    AgcDriver::GuestMemory::Read(freshBlock.Address(), std::as_writable_bytes(std::span(stored)), 1);
    Check(stored, 88.0f, "after target reallocation");
    return true;
}

bool ExpectDirectCpuInterleavedDraw(AgcDriver::VulkanDevice& device) {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputBit;
    constexpr std::size_t bytes = std::size_t{Width} * Height * 16u;
    Block block(bytes);
    std::fill(block.data, block.data + bytes, std::uint8_t{0xff});
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    if (AgcDriver::Graphics::HostImportFor(context, block.Address(), bytes) == nullptr) return false;
    const auto color = DecodeTarget(block, Rgba32FloatInfo);
    constexpr std::size_t rowBytes = std::size_t{Width} * 16u;
    constexpr std::size_t drawnBytes = std::size_t{Width / 2u} * 16u;
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 1.0f), {{0, 0}, {Width / 2u, Height}});
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "draw 1 did not leave results pending");
    for (std::uint32_t y = 0; y < Height; ++y) {
        std::memset(block.data + y * rowBytes + drawnBytes, 0x42, rowBytes - drawnBytes);
    }
    Draw(device, color, PixelInputBit(PixelInput::PositionFixedPoint), Program(0u, 2.0f), {{0, 0}, {Width / 4u, Height}});
    Require(AgcDriver::Graphics::StorageTexture::AnyPending(), "draw 2 did not leave results pending");
    device.WaitIdle();
    for (std::uint32_t y = 0; y < Height; ++y) {
        const auto* row = block.data + y * rowBytes;
        Require(std::all_of(row + drawnBytes, row + rowBytes, [](std::uint8_t b) { return b == 0x42u; }), "direct CPU write was overwritten by subsequent GPU draw");
    }
    return true;
}

bool Expect(AgcDriver::VulkanDevice& device) {
    if (!ExpectSingleDraw(device)) return false;
    if (!ExpectConsecutiveDraws(device)) return false;
    if (!ExpectDrawAcrossBatches(device)) return false;
    if (!ExpectMultipleSequentialDraws(device)) return false;
    if (!ExpectStoreIdleResults(device)) return false;
    if (!ExpectCpuWritesWithGuestMemory(device)) return false;
    if (!ExpectDirectCpuWriteAfterDraw(device)) return false;
    if (!ExpectAdditiveBlending(device)) return false;
    if (!ExpectColorWriteMask(device)) return false;
    if (!ExpectOverlappingScissors(device)) return false;
    if (!ExpectMultipleLinearTargets(device)) return false;
    if (!ExpectPartialGuestRead(device)) return false;
    if (!ExpectEmptyScissorDraw(device)) return false;
    if (!ExpectTargetReallocationWithoutRead(device)) return false;
    if (!ExpectDirectCpuInterleavedDraw(device)) return false;
    return true;
}

}

int main() {
    try {
        const auto device = OpenVulkanTestDevice();
        if (!device) return VulkanTestSkipped;
        if (!Expect(*device)) {
            std::puts("linear target resident test skipped: no host import on this device");
            return VulkanTestSkipped;
        }
        std::puts("linear target resident tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
