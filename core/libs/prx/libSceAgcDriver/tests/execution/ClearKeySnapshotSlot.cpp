#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Execution/include/VulkanDevice.hpp"
#include "prx/libSceAgcDriver/Graphics/include/ColorTargetLayout.hpp"
#include "prx/libSceAgcDriver/Graphics/include/DccMetadata.hpp"
#include "prx/libSceAgcDriver/Graphics/include/ShaderResources.hpp"
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
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace AgcDriver {

struct VulkanDeviceTestAccess {
    static Graphics::Context ContextOf(const VulkanDevice& device) { return device.graphicsContext(); }
};

}

namespace {

using AgcDriver::Graphics::DccKeys;
using AgcDriver::Graphics::Require;
using AgcDriver::Graphics::SampledTextureCounters;
using AgcDriver::Graphics::ShaderResources;
using AgcDriver::Graphics::StorageTexture;
using AgcDriver::Graphics::Texture;
using ShaderRecompiler::ShaderStage;

constexpr std::uint32_t Threads = 32;
constexpr std::uint32_t Side = 128;
constexpr std::uint32_t Format8888Unorm = 56;
constexpr std::uint32_t TileRenderTarget64KB = 0x1b;
constexpr std::uint32_t Type2D = 9;
constexpr std::size_t SurfaceBytes = 65536;
constexpr std::size_t KeyBytes = SurfaceBytes / 256;
constexpr std::size_t KeysOffset = SurfaceBytes;
constexpr std::size_t BlockBytes = 2 * 65536;
constexpr std::uint32_t WrittenTexel = 0x3f800000u;
constexpr std::array<std::uint32_t, 4> WrittenRgba{WrittenTexel, WrittenTexel, WrittenTexel, WrittenTexel};

struct ClearCase {
    std::uint8_t code;
    DccKeys keys;
    std::array<std::uint32_t, 4> rgba;
    const char* name;
};

constexpr std::array<ClearCase, 4> ClearCases{{
    {0x00, DccKeys::Clear0000, {0u, 0u, 0u, 0u}, "0000"},
    {0x40, DccKeys::Clear0001, {0u, 0u, 0u, WrittenTexel}, "0001"},
    {0x80, DccKeys::Clear1110, {WrittenTexel, WrittenTexel, WrittenTexel, 0u}, "1110"},
    {0xc0, DccKeys::Clear1111, WrittenRgba, "1111"},
}};

alignas(256) constexpr std::array<std::uint32_t, 8> WriteCode{
    0x7e020280, 0x7e0402f2, 0x7e0602f2, 0x7e0802f2, 0x7e0a02f2, 0xf0201f08, 0x00010200, 0xbf810000,
};

alignas(256) constexpr std::array<std::uint32_t, 11> SampleSecondTexelCode{
    0x34060084, 0x7e2802ff, 0x3c400000, 0x7e2a02ff, 0x3b800000, 0xf09c0f08, 0x00610a14, 0xbf8c3f70,
    0xe0781000, 0x80000a03, 0xbf810000,
};

constexpr std::array<std::uint32_t, 4> PointSampler{0x00000092u, 0x00fff000u, 0u, 0u};

std::uint64_t AddressOf(const void* data) {
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(data));
}

class GuestBlock {
public:
    explicit GuestBlock(bool watch) : watched(watch) {
#ifdef _WIN32
        if (watched) {
            block = static_cast<std::uint8_t*>(GuestArena::GuestArenaAllocate_nid_postfix(BlockBytes, 65536));
            if (block != nullptr) GuestArena::GuestArenaCommit_nid_postfix(block, BlockBytes, PAGE_READWRITE, BlockBytes);
        } else {
            block = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, BlockBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        }
#else
        if (watched) {
            constexpr std::uintptr_t alignment = 65536;
            void* mapped = mmap(nullptr, BlockBytes + alignment, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (mapped != MAP_FAILED) {
                const auto begin = reinterpret_cast<std::uintptr_t>(mapped);
                const auto aligned = (begin + alignment - 1) & ~(alignment - 1);
                if (aligned != begin) munmap(mapped, aligned - begin);
                if (aligned + BlockBytes != begin + BlockBytes + alignment) munmap(reinterpret_cast<void*>(aligned + BlockBytes), begin + alignment - aligned);
                block = reinterpret_cast<std::uint8_t*>(aligned);
                GuestWriteWatch::GuestWriteWatchRegister_nid_postfix(block, BlockBytes);
            }
        } else {
            block = static_cast<std::uint8_t*>(std::aligned_alloc(65536, BlockBytes));
        }
#endif
        Require(block != nullptr, "clear key snapshot slot: cannot allocate the guest block");
        Require(!watched || AgcDriver::GuestMemory::Watched(AddressOf(block), BlockBytes), "clear key snapshot slot: the watched guest block is not write-watched");
        GuestAllocations::Mutation().Add(block, BlockBytes, true, true, true);
    }

    ~GuestBlock() {
        GuestAllocations::Mutation().Remove(block);
#ifdef _WIN32
        if (watched) {
            GuestArena::GuestArenaReset_nid_postfix(block, BlockBytes);
            GuestArena::GuestArenaRelease_nid_postfix(block, BlockBytes);
        } else {
            VirtualFree(block, 0, MEM_RELEASE);
        }
#else
        if (watched) {
            munmap(block, BlockBytes);
            GuestWriteWatch::GuestWriteWatchUnregister_nid_postfix(block, BlockBytes);
        } else {
            std::free(block);
        }
#endif
    }

    GuestBlock(const GuestBlock&) = delete;
    GuestBlock& operator=(const GuestBlock&) = delete;

    std::uint8_t* Data() { return block; }

private:
    bool watched;
    std::uint8_t* block = nullptr;
};

std::array<std::uint32_t, 8> TextureDescriptor(const std::uint8_t* texels, const std::uint8_t* keys) {
    const auto address = AddressOf(texels);
    const auto meta = AddressOf(keys) >> 8u;
    return {
        static_cast<std::uint32_t>(address >> 8u),
        static_cast<std::uint32_t>((address >> 40u) & 0xffu) | (Format8888Unorm << 20u) | (((Side - 1u) & 3u) << 30u),
        ((Side - 1u) >> 2u) | ((Side - 1u) << 14u),
        0xfacu | (TileRenderTarget64KB << 20u) | (Type2D << 28u),
        0u,
        0u,
        (1u << 20u) | (1u << 21u) | (1u << 22u) | (static_cast<std::uint32_t>(meta & 0xffu) << 24u),
        static_cast<std::uint32_t>(meta >> 8u),
    };
}

std::array<std::uint32_t, 4> BufferDescriptor(const void* data, std::uint32_t bytes) {
    const auto address = AddressOf(data);
    return {static_cast<std::uint32_t>(address), static_cast<std::uint32_t>((address >> 32u) & 0xffffu), bytes, 0x31016facu};
}

ShaderRecompiler::RecompileResult SampledProgram(const std::array<std::uint32_t, 8>& words) {
    ShaderRecompiler::DescriptorBinding binding;
    binding.kind = ShaderRecompiler::DescriptorKind::SampledImage;
    binding.role = ShaderRecompiler::DescriptorRole::GuestImages;
    binding.descriptorSet = 0;
    binding.binding = ShaderRecompiler::RuntimeAbi::FirstImageBinding;
    binding.count = 1;
    binding.imageSamplers = {0};
    binding.imageShape = ShaderRecompiler::DescriptorImageShape::Image2D;
    binding.guestDescriptor.assign(words.begin(), words.end());
    ShaderRecompiler::RecompileResult program;
    program.bindings.push_back(binding);
    return program;
}

void Dispatch(AgcDriver::VulkanDevice& device, std::span<const std::uint32_t> code, const std::vector<std::uint32_t>& userData) {
    const std::array<ShaderRecompiler::MemoryRegion, 1> memory{{{reinterpret_cast<std::uintptr_t>(code.data()), std::as_bytes(code)}}};
    const ShaderRecompiler::ShaderComputeStageInfo compute{{Threads, 1, 1}, 0u, {false, false, false}, false, 1};
    ShaderRecompiler::RecompileRequest request{
        {ShaderStage::Compute, reinterpret_cast<std::uintptr_t>(code.data()), code, 0, {}},
        {32, 0, userData, compute, std::nullopt, std::nullopt, memory},
        device.Target(),
        {0, 0, 0, 128}
    };
    request.useCache = false;
    const auto result = ShaderRecompiler::Recompile(request);
    device.Dispatch(result, 1, 1, 1, {}, reinterpret_cast<std::uintptr_t>(code.data()));
    device.WaitIdle();
}

std::shared_ptr<Texture> RunCycles(AgcDriver::VulkanDevice& device, std::uint8_t* block, std::uint32_t* output, const char* memory, const ClearCase& clear) {
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    auto* texels = block;
    auto* keys = block + KeysOffset;
    const auto surface = AddressOf(texels);
    const auto prefix = std::string("clear key snapshot slot ") + clear.name + " over " + memory + " memory: ";
    std::memset(texels, 0x55, SurfaceBytes);
    std::memset(keys, 0xff, KeyBytes);
    AgcDriver::GuestMemory::BumpCollectEpoch();
    const auto words = TextureDescriptor(texels, keys);
    std::vector<std::uint32_t> writeData(16, 0u);
    std::copy(words.begin(), words.end(), writeData.begin() + 4);
    const auto results = BufferDescriptor(output, Threads * 16u);
    std::vector<std::uint32_t> sampleData(24, 0u);
    std::copy(results.begin(), results.end(), sampleData.begin());
    std::copy(words.begin(), words.end(), sampleData.begin() + 4);
    std::copy(PointSampler.begin(), PointSampler.end(), sampleData.begin() + 12);
    const auto program = SampledProgram(words);
    const AgcDriver::Graphics::CompiledShader compute{ShaderStage::Compute, &program, 0};
    std::lock_guard gpu(AgcDriver::GuestMemory::GpuMutex());
    std::shared_ptr<StorageTexture> image;
    const auto write = [&] {
        Dispatch(device, WriteCode, writeData);
        const auto pending = StorageTexture::FindPending(surface, SurfaceBytes);
        Require(pending != nullptr, prefix + "the kernel's write left no results pending over the surface");
        Require(image == nullptr || pending == image, prefix + "the kernel wrote another storage image of the surface");
        image = pending;
    };
    const auto fill = [&] {
        Require(StorageTexture::NoteKeysFill(AddressOf(keys), KeyBytes, clear.code) == 1, prefix + "the key fill did not cover the surface");
        const auto word = static_cast<std::uint32_t>(clear.code) * 0x01010101u;
        const std::array<std::uint32_t, 4> pattern{word, word, word, word};
        if (device.FillBuffer(AddressOf(keys), KeyBytes, pattern)) {
            AgcDriver::Graphics::NoteKeysFillOnGpu(AddressOf(keys), KeyBytes, clear.keys);
        } else {
            device.WaitIdle();
            const std::vector<std::byte> values(KeyBytes, static_cast<std::byte>(clear.code));
            AgcDriver::GuestMemory::Write(AddressOf(keys), values);
        }
        Require(StorageTexture::FindPending(surface, SurfaceBytes) == image && image->FilledKeys() == clear.keys, prefix + "the key fill did not leave the image's results pending under the fill's code");
    };
    const auto sample = [&](bool view, const std::array<std::uint32_t, 4>& expected, const std::string& when) {
        AgcDriver::GuestMemory::BumpCollectEpoch();
        auto texture = AgcDriver::Graphics::CachedSampledTexture(context, words);
        const bool viewed = texture != nullptr && texture->ViewsStorageImage();
        Require(texture != nullptr && viewed == view, prefix + "the surface sampled " + when + " is " + (viewed ? "a view of its image" : "a snapshot") + ", not " + (view ? "a view of its image" : "a snapshot"));
        std::fill(output, output + Threads * 4u, 0xdeadbeefu);
        Dispatch(device, SampleSecondTexelCode, sampleData);
        for (std::uint32_t index = 0; index < Threads * 4u; ++index) {
            char message[256];
            std::snprintf(message, sizeof(message), "component %u of thread %u sampled %s is 0x%08x, expected 0x%08x", index % 4u, index / 4u, when.c_str(), output[index], expected[index % 4u]);
            Require(output[index] == expected[index % 4u], prefix + message);
        }
        return texture;
    };
    const auto bound = [&](const std::shared_ptr<ShaderResources>& resources) {
        Require(resources->SampledTextures().size() == 1, prefix + "the compute object does not bind one sampled texture");
        return resources->SampledTextures().front();
    };
    write();
    fill();
    const auto cleared = sample(false, clear.rgba, "after the first key fill");
    const auto clearObject = std::make_shared<ShaderResources>(context, compute);
    Require(bound(clearObject) == cleared, prefix + "the compute object built after the first key fill does not bind the clear snapshot");
    write();
    const auto viewed = sample(true, WrittenRgba, "after the first write");
    const auto viewObject = std::make_shared<ShaderResources>(context, compute);
    Require(bound(viewObject) == viewed, prefix + "the compute object built after the first write does not bind the view of the image");
    const auto before = SampledTextureCounters();
    fill();
    Require(sample(false, clear.rgba, "after the second key fill") == cleared, prefix + "the second key fill made another snapshot of the clear");
    Require(clearObject->Revalidate(std::span(&compute, 1)), prefix + "the compute object built after the first key fill was not reused after the second");
    write();
    Require(sample(true, WrittenRgba, "after the second write") == viewed, prefix + "the second write made another view of the image");
    Require(viewObject->Revalidate(std::span(&compute, 1)), prefix + "the compute object built after the first write was not reused after the second");
    const auto after = SampledTextureCounters();
    Require(after.snapshots == before.snapshots && after.views == before.views, prefix + "the second key fill and write made " + std::to_string(after.snapshots - before.snapshots) + " snapshots and " + std::to_string(after.views - before.views) + " views");
    StorageTexture::FlushPending(surface, SurfaceBytes, nullptr, "test");
    device.WaitIdle();
    return cleared;
}

void RunAllCodes(AgcDriver::VulkanDevice& device, std::uint8_t* block, std::uint32_t* output, const char* memory) {
    std::array<std::shared_ptr<Texture>, ClearCases.size()> snapshots;
    for (std::size_t index = 0; index < ClearCases.size(); ++index) {
        snapshots[index] = RunCycles(device, block, output, memory, ClearCases[index]);
        for (std::size_t previous = 0; previous < index; ++previous) Require(snapshots[index] != snapshots[previous], "different constant clear codes shared a snapshot");
    }
    Require(RunCycles(device, block, output, memory, ClearCases.front()) == snapshots.front(), "returning to a prior constant clear code made another snapshot");
}

std::shared_ptr<Texture> SampleWords(AgcDriver::VulkanDevice& device, const std::array<std::uint32_t, 8>& words, std::uint32_t* output, const std::array<std::uint32_t, 4>& expected, const char* when) {
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    std::vector<std::uint32_t> sampleData(24, 0u);
    const auto results = BufferDescriptor(output, Threads * 16u);
    std::copy(results.begin(), results.end(), sampleData.begin());
    std::copy(words.begin(), words.end(), sampleData.begin() + 4);
    std::copy(PointSampler.begin(), PointSampler.end(), sampleData.begin() + 12);
    AgcDriver::GuestMemory::BumpCollectEpoch();
    auto texture = AgcDriver::Graphics::CachedSampledTexture(context, words);
    Require(texture != nullptr && !texture->ViewsStorageImage(), "clear snapshot control unexpectedly used a storage view");
    std::fill(output, output + Threads * 4u, 0xdeadbeefu);
    Dispatch(device, SampleSecondTexelCode, sampleData);
    for (std::uint32_t index = 0; index < Threads * 4u; ++index) {
        char message[256];
        std::snprintf(message, sizeof(message), "%s: component %u of thread %u is 0x%08x, expected 0x%08x", when, index % 4u, index / 4u, output[index], expected[index % 4u]);
        Require(output[index] == expected[index % 4u], message);
    }
    return texture;
}

void RunAlphaAndComponents(AgcDriver::VulkanDevice& device, std::uint8_t* block, std::uint32_t* output) {
    std::memset(block, 0x55, SurfaceBytes);
    std::memset(block + KeysOffset, 0x40, KeyBytes);
    const auto base = TextureDescriptor(block, block + KeysOffset);
    std::lock_guard gpu(AgcDriver::GuestMemory::GpuMutex());
    std::array<std::shared_ptr<Texture>, 4> snapshots;
    for (std::size_t index = 0; index < snapshots.size(); ++index) {
        auto words = base;
        const bool alphaOnMsb = (index & 1u) == 0u;
        const bool swap = (index & 2u) != 0u;
        if (!alphaOnMsb) words[6] &= ~(1u << 22u);
        if (swap) words[3] = (words[3] & ~0xfffu) | 0x9afu;
        std::array<std::uint32_t, 4> expected{};
        expected[alphaOnMsb != swap ? 3u : 0u] = WrittenTexel;
        snapshots[index] = SampleWords(device, words, output, expected, "constant clear alpha placement and component mapping");
        for (std::size_t previous = 0; previous < index; ++previous) Require(snapshots[index] != snapshots[previous], "different clear alpha placement or component mapping shared a snapshot");
        Require(SampleWords(device, words, output, expected, "repeated alpha placement and component mapping") == snapshots[index], "an unchanged clear mapping made another snapshot");
    }
}

void RunSingleMutation(AgcDriver::VulkanDevice& device, std::uint8_t* block, std::uint32_t* output) {
    using AgcDriver::Graphics::ColorTargetLayout;
    using AgcDriver::Graphics::ColorTileMode;
    const ColorTargetLayout layout(Side, Side, ColorTileMode::RenderTarget, 4u);
    const auto first = layout.Offset(1u, 0u) / 256u * 256u;
    std::memset(block, 0x00, SurfaceBytes);
    std::memset(block + first, 0xff, 4u);
    std::memset(block + KeysOffset, 0x10, KeyBytes);
    const auto words = TextureDescriptor(block, block + KeysOffset);
    std::lock_guard gpu(AgcDriver::GuestMemory::GpuMutex());
    const auto before = SampleWords(device, words, output, WrittenRgba, "comp-to-single before a texel change");
    std::memset(block + first, 0x00, 4u);
    Require(SampleWords(device, words, output, ClearCases.front().rgba, "comp-to-single after a texel change") != before, "comp-to-single incorrectly reused a constant clear snapshot after its texels changed");
}

void RunHeldEviction(AgcDriver::VulkanDevice& device, std::uint8_t* block, std::uint32_t* output) {
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    std::memset(block, 0x55, SurfaceBytes);
    std::memset(block + KeysOffset, 0x40, KeyBytes);
    const auto words = TextureDescriptor(block, block + KeysOffset);
    AgcDriver::GuestMemory::BumpCollectEpoch();
    const auto program = SampledProgram(words);
    const AgcDriver::Graphics::CompiledShader compute{ShaderStage::Compute, &program, 0u};
    std::lock_guard gpu(AgcDriver::GuestMemory::GpuMutex());
    const auto held = std::make_shared<ShaderResources>(context, compute);
    Require(held->SampledTextures().size() == 1u, "eviction control did not retain one clear snapshot");
    const auto previous = held->SampledTextures().front();
    Require(SampleWords(device, words, output, ClearCases[1u].rgba, "before clear snapshot eviction") == previous, "eviction control bound another clear snapshot");
    AgcDriver::Graphics::ClearCachedTextures(context.device);
    Require(held->SampledTextures().front() == previous && previous->AllocationBytes() != 0u && AgcDriver::Graphics::SampledTextureMemory() >= previous->AllocationBytes(), "eviction released a clear snapshot its consumer still holds");
    Require(!held->Revalidate(std::span(&compute, 1u)), "an evicted clear snapshot was accepted for another resource-cache use");
    Require(SampleWords(device, words, output, ClearCases[1u].rgba, "after clear snapshot eviction") != previous, "the cache returned an evicted clear snapshot");
}

void RunStageARecord(AgcDriver::VulkanDevice& device, std::uint8_t* block) {
    const auto context = AgcDriver::VulkanDeviceTestAccess::ContextOf(device);
    auto* texels = block;
    auto* keys = block + KeysOffset;
    std::memset(texels, 0x55, SurfaceBytes);
    std::memset(keys, 0x00, KeyBytes);
    AgcDriver::GuestMemory::BumpCollectEpoch();
    const auto words = TextureDescriptor(texels, keys);
    const auto program = SampledProgram(words);
    const AgcDriver::Graphics::CompiledShader compute{ShaderStage::Compute, &program, 0};
    std::lock_guard gpu(AgcDriver::GuestMemory::GpuMutex());
    const auto first = std::make_shared<ShaderResources>(context, compute);
    Require(first->SampledTextures().size() == 1 && !first->SampledTextures().front()->ViewsStorageImage(), "clear key snapshot slot: a 0000-keyed surface no image wrote was not sampled through a snapshot");
    const auto before = SampledTextureCounters();
    AgcDriver::GuestMemory::BumpCollectEpoch();
    const auto second = std::make_shared<ShaderResources>(context, compute);
    const auto after = SampledTextureCounters();
    Require(second->SampledTextures().size() == 1 && second->SampledTextures().front() == first->SampledTextures().front(), "clear key snapshot slot: the second compute object over an unchanged 0000-keyed surface binds another texture");
    Require(after.fastHits == before.fastHits + 1 && after.fastMisses == before.fastMisses && after.snapshots == before.snapshots, "clear key snapshot slot: the second compute object over an unchanged 0000-keyed surface took " + std::to_string(after.fastHits - before.fastHits) + " fast hits, " + std::to_string(after.fastMisses - before.fastMisses) + " fast misses and " + std::to_string(after.snapshots - before.snapshots) + " snapshots, not its stage-A record's texture");
    Require(AgcDriver::Graphics::CachedSampledTexture(context, words) == first->SampledTextures().front(), "clear key snapshot slot: the full lookup of an unchanged 0000-keyed surface returns another texture than its stage-A record");
}

}

int main() {
    try {
#ifdef _WIN32
        Require(_putenv_s("APS5_NO_CLEARED_VIEW", "1") == 0, "cannot select the clear snapshot test path");
#else
        Require(setenv("APS5_NO_CLEARED_VIEW", "1", 1) == 0, "cannot select the clear snapshot test path");
#endif
        std::optional<GuestBlock> unwatched;
        std::optional<GuestBlock> watched;
        std::optional<GuestBlock> recorded;
        std::optional<GuestBlock> controls;
        std::optional<GuestBlock> output;
        const auto device = OpenVulkanTestDevice();
        if (!device) return VulkanTestSkipped;
        output.emplace(false);
        unwatched.emplace(false);
        RunAllCodes(*device, unwatched->Data(), reinterpret_cast<std::uint32_t*>(output->Data()), "unwatched");
        controls.emplace(false);
        RunAlphaAndComponents(*device, controls->Data(), reinterpret_cast<std::uint32_t*>(output->Data()));
        RunSingleMutation(*device, controls->Data(), reinterpret_cast<std::uint32_t*>(output->Data()));
        RunHeldEviction(*device, controls->Data(), reinterpret_cast<std::uint32_t*>(output->Data()));
        if (AgcDriver::GuestMemory::WriteWatched()) {
            watched.emplace(true);
            RunAllCodes(*device, watched->Data(), reinterpret_cast<std::uint32_t*>(output->Data()), "write-watched");
            recorded.emplace(true);
            RunStageARecord(*device, recorded->Data());
        } else {
            std::puts("guest memory has no write watch: write-watched surfaces are not tested");
        }
        std::puts("clear key snapshot slot tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
