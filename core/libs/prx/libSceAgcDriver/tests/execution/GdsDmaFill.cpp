#include "prx/libSceAgcDriver/Execution/include/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Draw/DrawPipeline.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Synchronization/SynchronizationStatistics.hpp"
#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include "prx/libc/include/GuestAllocations.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include "VulkanTestDevice.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using AgcDriver::DriverDetail::DrawPipeline;

constexpr std::uint32_t GdsOffset = 0x40;
constexpr std::uint32_t FillBytes = 64;
constexpr std::size_t BlockBytes = 256 * 1024;
constexpr std::uint32_t BufferFillBytes = 128 * 1024;

void check(bool condition, const std::string& what) {
    if (!condition) throw std::runtime_error("GDS DMA fill: " + what);
}

std::uint8_t* GuestBlock() {
#ifdef _WIN32
    auto* block = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, BlockBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
#else
    auto* block = static_cast<std::uint8_t*>(std::aligned_alloc(65536, BlockBytes));
#endif
    check(block != nullptr, "cannot allocate a guest block");
    std::memset(block, 0, BlockBytes);
    GuestAllocations::Mutation().Add(block, BlockBytes, true, true, true);
    return block;
}

std::uint64_t Address(const void* pointer) {
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(pointer));
}

std::array<std::uint32_t, 7> FillGds(std::uint32_t value) {
    return {0xc0055000u, (2u << 29u) | (1u << 20u), value, 0u, GdsOffset, 0u, FillBytes};
}

std::array<std::uint32_t, 7> PatternFill(std::uint64_t destination, std::uint32_t bytes) {
    return {0xc0055000u, 2u << 29u, 0x1234abcdu, 0u, static_cast<std::uint32_t>(destination), static_cast<std::uint32_t>(destination >> 32u), bytes};
}

std::array<std::uint32_t, 7> CopyGdsTo(std::uint64_t destination) {
    return {0xc0055000u, 1u << 29u, GdsOffset, 0u, static_cast<std::uint32_t>(destination), static_cast<std::uint32_t>(destination >> 32u), FillBytes};
}

template <std::size_t Count>
void Append(std::vector<std::uint32_t>& words, const std::array<std::uint32_t, Count>& packet) {
    words.insert(words.end(), packet.begin(), packet.end());
}

std::array<std::uint32_t, 3> SetShaderRegister(std::uint32_t offset, std::uint32_t value) {
    return {0xc0017600u, offset, value};
}

void Submit(std::vector<std::uint32_t> words) {
    Packet packet{words.data(), static_cast<std::uint32_t>(words.size()), 0, {}};
    check(sceAgcDriverSubmitDcb(&packet) == 0, "submission failed");
}

void ExpectGds(std::uint8_t* readBack, std::uint32_t value, const char* what) {
    std::memset(readBack, 0, FillBytes);
    std::vector<std::uint32_t> words;
    Append(words, CopyGdsTo(Address(readBack)));
    Submit(std::move(words));
    AgcDriverWaitIdle_nid_postfix();
    std::array<std::uint32_t, FillBytes / 4> read{};
    std::memcpy(read.data(), readBack, FillBytes);
    for (const auto word : read) check(word == value, what);
}

}

int main() {
#ifdef _WIN32
    _putenv_s("APS5_PIPELINED_DRAWS", "1");
#else
    setenv("APS5_PIPELINED_DRAWS", "1", 1);
#endif
    try {
        const auto device = OpenVulkanTestDevice();
        if (!device) return VulkanTestSkipped;
        using namespace AgcDriver::DriverDetail;
        auto* readBack = GuestBlock();
        auto* filled = GuestBlock();
        auto* held = GuestBlock();
        auto* code = GuestBlock();
        constexpr std::uint32_t EndProgram = 0xbf810000u;
        std::memcpy(code, &EndProgram, sizeof(EndProgram));

        std::vector<std::uint32_t> words;
        Append(words, SetShaderRegister(0x20c, static_cast<std::uint32_t>(Address(code) >> 8u)));
        Append(words, SetShaderRegister(0x20d, static_cast<std::uint32_t>(Address(code) >> 40u) & 0xffu));
        Append(words, SetShaderRegister(0x207, 64));
        Append(words, SetShaderRegister(0x208, 1));
        Append(words, SetShaderRegister(0x209, 1));
        Append(words, SetShaderRegister(0x213, 0));
        Append(words, std::array<std::uint32_t, 5>{0xc0031500u, 1, 1, 1, 0x41});
        Submit(std::move(words));
        AgcDriverWaitIdle_nid_postfix();

        const auto drainedBefore = storesDrained.load();
        const auto fillsBefore = dmaFills.load();
        const auto gpuBefore = storesOnGpu.load() + storesBehindCompletions.load();
        const auto cpuBefore = storesOnCpu.load();
        words.clear();
        Append(words, PatternFill(Address(filled), BufferFillBytes));
        Append(words, FillGds(0x5eed1234u));
        Submit(std::move(words));
        AgcDriverWaitIdle_nid_postfix();
        check(storesDrained.load() == drainedBefore, "an immediate DMA_DATA fill of GDS drained the device");
        if (dmaFills.load() != fillsBefore) {
            check(storesOnGpu.load() + storesBehindCompletions.load() == gpuBefore + 1, "the fill of GDS after recorded work was not stored on the GPU");
            check(storesOnCpu.load() == cpuBefore, "the fill of GDS after recorded work was written by the CPU");
        } else {
            std::printf("buffer fill not recorded: the device does not store fills into imported guest memory\n");
        }
        ExpectGds(readBack, 0x5eed1234u, "a GDS to memory copy did not see the immediate fill before it");

        {
            std::atomic<bool> release{false};
            const auto address = Address(held);
            DrawPipeline::Queue0().Enqueue([&release] {
                while (!release.load(std::memory_order_acquire)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }, {{address, address + 16}});
            const auto drainsBefore = DrawPipeline::DrainRequestsByOpcode(0x50);
            words.clear();
            Append(words, FillGds(0x0badf00du));
            Submit(std::move(words));
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            while (DrawPipeline::Queue0().Outstanding() < 2 && DrawPipeline::DrainRequestsByOpcode(0x50) == drainsBefore && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(std::chrono::milliseconds(1));
            const auto queued = DrawPipeline::Queue0().Outstanding();
            const auto drains = DrawPipeline::DrainRequestsByOpcode(0x50);
            release.store(true, std::memory_order_release);
            check(drains == drainsBefore, "an immediate DMA_DATA fill of GDS drained the draw pipeline");
            check(queued >= 2, "an immediate DMA_DATA fill of GDS was not queued behind the held draw pipeline item");
        }
        ExpectGds(readBack, 0x0badf00du, "a GDS to memory copy did not see the fill queued in the draw pipeline");

        LibcRunShutdown_nid_postfix();
        std::puts("GDS DMA fill tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        try { LibcRunShutdown_nid_postfix(); } catch (...) {}
        return 1;
    }
}
