#include "prx/libSceAgcDriver/Execution/include/Driver/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Pipeline.hpp"
#include "prx/libSceAgcDriver/Graphics/include/PipelineLookahead.hpp"
#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include "prx/libc/include/GuestAllocations.hpp"
#include "prx/libc/include/GuestArena.hpp"
#include "prx/libc/include/GuestWriteWatch.hpp"
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
#include <atomic>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <span>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

using AgcDriver::Graphics::Require;

constexpr std::uint32_t LinearWidth = 64;
constexpr std::uint32_t LinearHeight = 4;
constexpr std::uint32_t TiledSide = 128;
constexpr std::size_t TiledBytes = 65536;
constexpr std::uint32_t ClearWord = 0x40404040u;
constexpr std::uint32_t StoredValue = 0xf7e54000u;
constexpr std::size_t MaxPrograms = 256;
constexpr std::uint32_t Color32FloatInfo = (4u << 2u) | (7u << 8u);
constexpr std::uint32_t TiledAttrib3 = (0x4dc6c000u & ~(0x1fu << 14u)) | (0x1bu << 14u);
using LinearImage = std::array<std::uint32_t, LinearWidth * LinearHeight>;

alignas(256) constexpr std::array<std::uint32_t, 8> VertexCode{0xe0382000u, 0x80020005u, 0xbf8c3f70u, 0xf80008cfu, 0x03020100u, 0xbf810000u, 0xbf810000u, 0xbf810000u};
alignas(256) constexpr std::array<std::array<float, 4>, 3> Triangle{{{-1.0f, -1.0f, 0.5f, 1.0f}, {3.0f, -1.0f, 0.5f, 1.0f}, {-1.0f, 3.0f, 0.5f, 1.0f}}};
alignas(256) std::array<std::array<std::uint32_t, 1024>, MaxPrograms> PixelPrograms{};
alignas(256) std::array<std::array<std::uint32_t, 64>, MaxPrograms> ComputePrograms{};
alignas(256) LinearImage LinearPixels{};
alignas(256) std::array<std::uint32_t, 4> Value{};
alignas(256) std::array<std::uint32_t, 2> Table{};
alignas(256) std::array<std::uint32_t, MaxPrograms> Output{};
alignas(256) std::array<std::uint32_t, 4> OutputDescriptor{};
alignas(256) volatile std::uint32_t Gate = 0;

struct Header {
    Shader shader{};
    ShaderUserData userData{};
};

alignas(8) Header VertexHeader{};
alignas(8) std::array<Header, MaxPrograms> PixelHeaders{};

class GuestBlock {
public:
    GuestBlock() {
#ifdef _WIN32
        block = static_cast<std::uint8_t*>(GuestArena::GuestArenaAllocate_nid_postfix(TiledBytes, TiledBytes));
        if (block != nullptr) GuestArena::GuestArenaCommit_nid_postfix(block, TiledBytes, PAGE_READWRITE, TiledBytes);
#else
        void* mapped = mmap(nullptr, TiledBytes * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mapped != MAP_FAILED) {
            const auto begin = reinterpret_cast<std::uintptr_t>(mapped);
            const auto aligned = (begin + TiledBytes - 1) & ~(static_cast<std::uintptr_t>(TiledBytes) - 1);
            if (aligned != begin) munmap(mapped, aligned - begin);
            if (aligned + TiledBytes != begin + TiledBytes * 2) munmap(reinterpret_cast<void*>(aligned + TiledBytes), begin + TiledBytes - aligned);
            block = reinterpret_cast<std::uint8_t*>(aligned);
            if (AgcDriver::GuestMemory::WriteWatched()) GuestWriteWatch::GuestWriteWatchRegister_nid_postfix(block, TiledBytes);
        }
#endif
        Require(block != nullptr, "cannot allocate the tiled target");
        GuestAllocations::Mutation().Add(block, TiledBytes, true, true, true);
    }

    ~GuestBlock() {
        GuestAllocations::Mutation().Remove(block);
#ifdef _WIN32
        GuestArena::GuestArenaReset_nid_postfix(block, TiledBytes);
        GuestArena::GuestArenaRelease_nid_postfix(block, TiledBytes);
#else
        munmap(block, TiledBytes);
        if (AgcDriver::GuestMemory::WriteWatched()) GuestWriteWatch::GuestWriteWatchUnregister_nid_postfix(block, TiledBytes);
#endif
    }

    GuestBlock(const GuestBlock&) = delete;
    GuestBlock& operator=(const GuestBlock&) = delete;
    std::uint64_t Address() const { return reinterpret_cast<std::uintptr_t>(block); }
    std::span<std::uint32_t> Words() { return {reinterpret_cast<std::uint32_t*>(block), TiledBytes / sizeof(std::uint32_t)}; }

private:
    std::uint8_t* block = nullptr;
};

void Registered(Header& header, const std::uint32_t* code, std::size_t bytes, std::uint8_t type) {
    header.shader.file_header = 0x34333231u;
    header.shader.version = 0x18u;
    header.shader.header_size = sizeof(Header);
    header.shader.shader_size = static_cast<std::uint32_t>(bytes);
    header.shader.code = code;
    header.shader.type = type;
    header.shader.user_data = &header.userData;
    AgcDriverRegisterShader_nid_postfix(&header.shader);
}

std::uint32_t Low(const volatile void* pointer) {
    return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(pointer));
}

std::uint32_t High(const volatile void* pointer) {
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(pointer)) >> 32u);
}

std::span<const std::uint32_t> PixelProgram(std::size_t index, const std::array<float, 4>& color, std::uint32_t chain) {
    auto& words = PixelPrograms.at(index);
    std::size_t at = 0;
    for (std::uint32_t channel = 0; channel < 4; ++channel) {
        words[at++] = 0x7e0002ffu | ((4u + channel) << 17u);
        words[at++] = std::bit_cast<std::uint32_t>(color[channel]);
    }
    if (chain != 0) {
        words[at++] = 0x7e080208u;
        for (std::uint32_t step = 0; step < chain; ++step) {
            words[at++] = 0x100808ffu;
            words[at++] = std::bit_cast<std::uint32_t>(1.0f + static_cast<float>(index * 1024u + step) * 1e-7f);
            words[at++] = 0x060808ffu;
            words[at++] = std::bit_cast<std::uint32_t>(static_cast<float>(step + 1u) * 1e-6f);
        }
        words[at++] = 0x100808ffu;
        words[at++] = std::bit_cast<std::uint32_t>(0.0f);
        words[at++] = 0x060808ffu;
        words[at++] = std::bit_cast<std::uint32_t>(color[0]);
    }
    words[at++] = 0xf800180fu;
    words[at++] = 0x07060504u;
    words[at++] = 0xbf810000u;
    return std::span<const std::uint32_t>(words.data(), at);
}

std::span<const std::uint32_t> ComputeProgram(std::size_t index) {
    auto& words = ComputePrograms.at(index);
    const std::array<std::uint32_t, 12> reader{0xf4040100u, 0xfa000000u, 0xf4080201u, 0xfa000000u, 0xbf8cc07fu, 0xf4000182u, 0xfa000000u, 0xbf8cc07fu, 0x7e000206u, 0xe0700000u | static_cast<std::uint32_t>(index * 4u), 0x80020000u, 0xbf810000u};
    std::copy(reader.begin(), reader.end(), words.begin());
    return std::span<const std::uint32_t>(words.data(), reader.size());
}

void Append(std::vector<std::uint32_t>& commands, std::uint32_t opcode, std::initializer_list<std::uint32_t> payload) {
    commands.push_back(0xc0000000u | (static_cast<std::uint32_t>(payload.size() - 1) << 16u) | (opcode << 8u));
    commands.insert(commands.end(), payload);
}

void AppendWaitEqual(std::vector<std::uint32_t>& commands, const volatile std::uint32_t* target, std::uint32_t value) {
    commands.insert(commands.end(), {0xc0053c00u, 0x13u, Low(target), High(target), value, 0xffffffffu, 0x19u});
}

void AppendDrawState(std::vector<std::uint32_t>& commands) {
    const auto vertex = reinterpret_cast<std::uintptr_t>(VertexCode.data());
    const auto buffer = reinterpret_cast<std::uintptr_t>(Triangle.data());
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> context{
        {0x2d5, 0x2000},
        {0x1b6, 0}, {0x207, 0}, {0x200, 0}, {0x203, 0x800},
        {0x2dc, 0xaa00}, {0x2f8, 0}, {0x292, 2}, {0x293, 0},
        {0x80, 0}, {0x8d, 0}, {0x83, 0xffff}, {0x8c, 0xa},
        {0x2f9, 0x2d}, {0x313, 0x6000}, {0x30e, 0xffffffff}, {0x30f, 0xffffffff},
        {0x206, 0x43f}, {0x204, 0x80000}, {0x205, 0x240},
        {0x8e, 0xf}, {0x8f, 0xf}, {0x202, 0xcc0010},
        {0x1c4, 0}, {0x1c5, 9}, {0x1c3, 4}, {0x1e0, 0},
        {0x113, std::bit_cast<std::uint32_t>(1.0f)}, {0x114, 0},
        {0xb4, 0}, {0xb5, std::bit_cast<std::uint32_t>(1.0f)},
        {0x2fe, 0}, {0x302, 0}, {0x306, 0}, {0x30a, 0},
    };
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> shader{
        {0xc8, static_cast<std::uint32_t>(vertex >> 8u)}, {0xc9, static_cast<std::uint32_t>(vertex >> 40u)}, {0x8b, 4u << 1u},
        {0x8c, static_cast<std::uint32_t>(buffer)}, {0x8d, static_cast<std::uint32_t>((buffer >> 32u) & 0xffffu) | (16u << 16u)}, {0x8e, static_cast<std::uint32_t>(Triangle.size())}, {0x8f, 0x01016facu},
        {0x00b, 1u << 1u}, {0x00c, std::bit_cast<std::uint32_t>(0.5f)},
    };
    Append(commands, 0x79, {0x242, 4});
    for (const auto& [offset, value] : context) Append(commands, 0x69, {offset, value});
    for (const auto& [offset, value] : shader) Append(commands, 0x76, {offset, value});
}

void AppendViewport(std::vector<std::uint32_t>& commands, std::uint32_t width, std::uint32_t height) {
    const auto corner = (height << 16u) | width;
    Append(commands, 0x69, {0xc, 0});
    Append(commands, 0x69, {0xd, corner});
    Append(commands, 0x69, {0x81, 0x80000000});
    Append(commands, 0x69, {0x82, corner});
    Append(commands, 0x69, {0x90, 0x80000000});
    Append(commands, 0x69, {0x91, corner});
    Append(commands, 0x69, {0x94, 0x80000000});
    Append(commands, 0x69, {0x95, corner});
    Append(commands, 0x69, {0x10f, std::bit_cast<std::uint32_t>(static_cast<float>(width) / 2.0f)});
    Append(commands, 0x69, {0x110, std::bit_cast<std::uint32_t>(static_cast<float>(width) / 2.0f)});
    Append(commands, 0x69, {0x111, std::bit_cast<std::uint32_t>(-static_cast<float>(height) / 2.0f)});
    Append(commands, 0x69, {0x112, std::bit_cast<std::uint32_t>(static_cast<float>(height) / 2.0f)});
}

void AppendLinearTarget(std::vector<std::uint32_t>& commands) {
    const auto color = reinterpret_cast<std::uintptr_t>(LinearPixels.data());
    AppendViewport(commands, LinearWidth, LinearHeight);
    Append(commands, 0x69, {0x31c, 0x28028});
    Append(commands, 0x69, {0x31b, 0});
    Append(commands, 0x69, {0x31d, 0});
    Append(commands, 0x69, {0x3b0, ((LinearWidth - 1u) << 14u) | (LinearHeight - 1u)});
    Append(commands, 0x69, {0x3b8, 0x9000000});
    Append(commands, 0x69, {0x318, static_cast<std::uint32_t>(color >> 8u)});
    Append(commands, 0x69, {0x390, static_cast<std::uint32_t>(color >> 40u)});
}

void AppendTiledTarget(std::vector<std::uint32_t>& commands, std::uint64_t address) {
    AppendViewport(commands, TiledSide, TiledSide);
    Append(commands, 0x69, {0x31c, Color32FloatInfo});
    Append(commands, 0x69, {0x31b, 0});
    Append(commands, 0x69, {0x31d, 0});
    Append(commands, 0x69, {0x3b0, ((TiledSide - 1u) << 14u) | (TiledSide - 1u)});
    Append(commands, 0x69, {0x3b8, TiledAttrib3});
    Append(commands, 0x69, {0x318, static_cast<std::uint32_t>(address >> 8u)});
    Append(commands, 0x69, {0x390, static_cast<std::uint32_t>(address >> 40u)});
}

void AppendDraw(std::vector<std::uint32_t>& commands, std::span<const std::uint32_t> pixelCode) {
    const auto pixel = reinterpret_cast<std::uintptr_t>(pixelCode.data());
    Append(commands, 0x76, {0x008, static_cast<std::uint32_t>(pixel >> 8u)});
    Append(commands, 0x76, {0x009, static_cast<std::uint32_t>(pixel >> 40u)});
    Append(commands, 0x2d, {static_cast<std::uint32_t>(Triangle.size()), 2});
}

void AppendDispatch(std::vector<std::uint32_t>& commands, std::span<const std::uint32_t> code) {
    const auto address = reinterpret_cast<std::uintptr_t>(code.data());
    const std::array<std::uint32_t, 4> userData{Low(Table.data()), High(Table.data()), Low(OutputDescriptor.data()), High(OutputDescriptor.data())};
    Append(commands, 0x76, {0x20c, static_cast<std::uint32_t>(address >> 8u)});
    Append(commands, 0x76, {0x20d, static_cast<std::uint32_t>(address >> 40u) & 0xffu});
    for (const std::uint32_t offset : {0x207u, 0x208u, 0x209u}) Append(commands, 0x76, {offset, 1});
    Append(commands, 0x76, {0x212, 0});
    Append(commands, 0x76, {0x213, static_cast<std::uint32_t>(userData.size()) << 1u});
    for (std::size_t index = 0; index < userData.size(); ++index) Append(commands, 0x76, {static_cast<std::uint32_t>(0x240 + index), userData[index]});
    commands.insert(commands.end(), {0xc0031500u, 1, 1, 1, 0x8041u});
}

void PrepareMemory() {
    LinearPixels.fill(ClearWord);
    Value[0] = StoredValue;
    Table = {Low(Value.data()), High(Value.data())};
    Output.fill(0);
    OutputDescriptor = {Low(Output.data()), High(Output.data()) & 0xffffu, static_cast<std::uint32_t>(Output.size() * sizeof(std::uint32_t)), 0x31016facu};
    Gate = 0;
}

void Submit(std::vector<std::uint32_t>& commands) {
    Packet packet{commands.data(), static_cast<std::uint32_t>(commands.size()), 0, {}};
    Require(sceAgcDriverSubmitDcb(&packet) == 0, "the draw submission failed");
}

std::uint32_t ExpectedLinearPixel(const std::array<float, 4>& color) {
    const auto channel = [&](std::size_t index) { return static_cast<std::uint32_t>(color[index] * 255.0f + 0.5f); };
    return channel(0) | (channel(1) << 8u) | (channel(2) << 16u) | (channel(3) << 24u);
}

void ExpectWords(std::span<const std::uint32_t> words, std::uint32_t expected, const char* what) {
    for (std::size_t index = 0; index < words.size(); ++index) {
        if (words[index] == expected) continue;
        char detail[128];
        std::snprintf(detail, sizeof(detail), ": word %zu holds 0x%08x, expected 0x%08x", index, words[index], expected);
        throw std::runtime_error(std::string(what) + detail);
    }
}

void ExpectLinearImage(std::uint32_t expected, const char* what) {
    AgcDriver::GuestMemory::FlushGpuWrites(reinterpret_cast<std::uintptr_t>(LinearPixels.data()), sizeof(LinearPixels));
    ExpectWords(LinearPixels, expected, what);
}

void ExpectTiledImage(GuestBlock& target, float expected, const char* what) {
    AgcDriver::GuestMemory::FlushGpuWrites(target.Address(), TiledBytes);
    ExpectWords(target.Words(), std::bit_cast<std::uint32_t>(expected), what);
}

void ExpectOutput(std::size_t count, const char* what) {
    AgcDriver::GuestMemory::FlushGpuWrites(reinterpret_cast<std::uintptr_t>(Output.data()), sizeof(Output));
    for (std::size_t index = 0; index < count; ++index) Require(Output[index] == StoredValue, std::string(what) + ": output " + std::to_string(index) + " holds " + std::to_string(Output[index]));
}

struct Totals {
    AgcDriver::DriverDetail::Driver::SubmitLookaheadCounters walk;
    AgcDriver::Graphics::PipelineLookaheadCounters pool;
    AgcDriver::Graphics::PipelineStoreCounters store;
    std::uint64_t dispatchBuilds;
};

Totals Snapshot() {
    return {AgcDriver::DriverDetail::Driver::SubmitLookaheadTotals(), AgcDriver::Graphics::PipelineLookaheadTotals(), AgcDriver::Graphics::PipelineStoreTotals(), AgcDriver::VulkanDevice::DispatchPipelineBuilds()};
}

void WaitForPool(const Totals& before, std::uint64_t graphics, std::uint64_t compute, std::chrono::seconds timeout) {
    const auto start = std::chrono::steady_clock::now();
    for (;;) {
        const auto totals = Snapshot();
        Require(totals.walk.failures == before.walk.failures && totals.pool.failed == before.pool.failed, "the lookahead failed to resolve or build");
        if (totals.walk.graphicsAhead >= before.walk.graphicsAhead + graphics && totals.walk.computeAhead >= before.walk.computeAhead + compute && totals.pool.built + totals.pool.present >= before.pool.built + before.pool.present + graphics + compute) return;
        Require(std::chrono::steady_clock::now() - start < timeout, "the lookahead did not build the pipelines of the gated submission in time");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void LookaheadBuildsBeforeTheWorker(bool enabled) {
    const std::array<float, 4> red{1.0f, 0.0f, 0.0f, 1.0f};
    const std::array<float, 4> green{0.75f, 1.0f, 0.0f, 1.0f};
    PrepareMemory();
    GuestBlock tiled;
    std::fill(tiled.Words().begin(), tiled.Words().end(), ClearWord);
    Registered(VertexHeader, VertexCode.data(), sizeof(VertexCode), 2);
    const auto redCode = PixelProgram(0, red, 0);
    const auto greenCode = PixelProgram(1, green, 0);
    Registered(PixelHeaders[0], redCode.data(), redCode.size() * sizeof(std::uint32_t), 1);
    Registered(PixelHeaders[1], greenCode.data(), greenCode.size() * sizeof(std::uint32_t), 1);
    const auto computeCode = ComputeProgram(0);
    std::vector<std::uint32_t> commands;
    AppendWaitEqual(commands, &Gate, 1);
    AppendDrawState(commands);
    AppendLinearTarget(commands);
    AppendDraw(commands, redCode);
    AppendTiledTarget(commands, tiled.Address());
    AppendDraw(commands, greenCode);
    AppendDispatch(commands, computeCode);
    const auto before = Snapshot();
    Submit(commands);
    if (enabled) {
        WaitForPool(before, 2, 1, std::chrono::seconds(60));
        const auto gated = Snapshot();
        Require(gated.walk.draws == before.walk.draws + 2 && gated.walk.dispatches == before.walk.dispatches + 1, "the lookahead did not decode two draws and one dispatch behind the wait");
        Require(gated.pool.enqueued == before.pool.enqueued + 3 && gated.pool.built == before.pool.built + 3, "the pool did not build the draws' and the dispatch's pipelines while the worker waited");
        Require(gated.store.misses == before.store.misses + 2 && gated.store.hits == before.store.hits, "the pool's graphics pipelines did not enter the store as misses");
        Require(gated.dispatchBuilds == before.dispatchBuilds, "a dispatch built a compute pipeline before the gate opened");
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        const auto gated = Snapshot();
        Require(gated.walk.runs == before.walk.runs && gated.pool.enqueued == before.pool.enqueued, "the lookahead ran although it is switched off");
    }
    Gate = 1;
    std::atomic_thread_fence(std::memory_order_seq_cst);
    AgcDriverWaitIdle_nid_postfix();
    const auto after = Snapshot();
    ExpectLinearImage(ExpectedLinearPixel(red), "the gated draw into the linear target");
    ExpectTiledImage(tiled, green[0], "the gated draw into the tiled target");
    ExpectOutput(1, "the gated dispatch");
    if (enabled) {
        Require(after.store.hits == before.store.hits + 2 && after.store.misses == before.store.misses + 2, "the worker's draws did not find the pipelines the pool built");
        Require(after.dispatchBuilds == before.dispatchBuilds, "the worker's dispatch built a compute pipeline the pool had built");
        Require(after.pool.built == before.pool.built + 3 && after.pool.failed == before.pool.failed, "the pool built more or fewer pipelines than the submission's draws and dispatch");
    } else {
        Require(after.store.misses == before.store.misses + 2 && after.dispatchBuilds == before.dispatchBuilds + 1, "without the lookahead the worker did not build the three pipelines itself");
    }
    std::vector<std::uint32_t> again;
    AppendDrawState(again);
    AppendLinearTarget(again);
    AppendDraw(again, redCode);
    AppendTiledTarget(again, tiled.Address());
    AppendDraw(again, greenCode);
    AppendDispatch(again, computeCode);
    Submit(again);
    AgcDriverWaitIdle_nid_postfix();
    const auto repeated = Snapshot();
    ExpectLinearImage(ExpectedLinearPixel(red), "the repeated draw into the linear target");
    ExpectTiledImage(tiled, green[0], "the repeated draw into the tiled target");
    Require(repeated.store.misses == after.store.misses && repeated.dispatchBuilds == after.dispatchBuilds, "a repeated submission built pipelines again");
    if (enabled) Require(repeated.walk.drawsSeen + repeated.walk.dispatchesSeen >= after.walk.drawsSeen + after.walk.dispatchesSeen + 3 && repeated.pool.enqueued == after.pool.enqueued, "a repeated submission was decoded ahead again instead of being skipped as seen");
}

void Benchmark(std::uint32_t count, std::uint32_t chain, bool enabled) {
    Require(count >= 1 && count < MaxPrograms, "the benchmark takes between 1 and 255 programs");
    Require(chain <= 200, "the benchmark's instruction chain is at most 200 steps");
    PrepareMemory();
    GuestBlock tiled;
    std::fill(tiled.Words().begin(), tiled.Words().end(), ClearWord);
    Registered(VertexHeader, VertexCode.data(), sizeof(VertexCode), 2);
    const auto color = [&](std::uint32_t index) { return std::array<float, 4>{static_cast<float>(index + 1u) / static_cast<float>(count + 2u), static_cast<float>(count + 1u - index) / static_cast<float>(count + 2u), 0.5f, 1.0f}; };
    const auto warmCode = PixelProgram(MaxPrograms - 1, color(count), chain);
    Registered(PixelHeaders[MaxPrograms - 1], warmCode.data(), warmCode.size() * sizeof(std::uint32_t), 1);
    std::vector<std::uint32_t> warm;
    AppendDrawState(warm);
    AppendTiledTarget(warm, tiled.Address());
    AppendDraw(warm, warmCode);
    AppendDispatch(warm, ComputeProgram(MaxPrograms - 1));
    Submit(warm);
    AgcDriverWaitIdle_nid_postfix();
    std::vector<std::uint32_t> commands;
    AppendDrawState(commands);
    AppendTiledTarget(commands, tiled.Address());
    for (std::uint32_t index = 0; index < count; ++index) {
        const auto pixelCode = PixelProgram(index, color(index), chain);
        Registered(PixelHeaders[index], pixelCode.data(), pixelCode.size() * sizeof(std::uint32_t), 1);
        AppendDraw(commands, pixelCode);
    }
    for (std::uint32_t index = 0; index < count; ++index) AppendDispatch(commands, ComputeProgram(index));
    const auto before = Snapshot();
    const auto started = std::chrono::steady_clock::now();
    Submit(commands);
    AgcDriverWaitIdle_nid_postfix();
    const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    const auto after = Snapshot();
    ExpectOutput(count, "the benchmark's dispatches");
    ExpectTiledImage(tiled, color(count - 1u)[0], "the benchmark's last draw");
    std::printf("submit lookahead benchmark (%s): %u draws and %u dispatches of new configurations (pixel chain of %u steps), submitted and idle in %.1f ms\n", enabled ? "on" : "off", count, count, chain, ms);
    std::printf("  graphics pipelines: %llu store misses (builds by the pool or the worker), %llu draws found their pipeline in the store\n", static_cast<unsigned long long>(after.store.misses - before.store.misses), static_cast<unsigned long long>(after.store.hits - before.store.hits));
    std::printf("  compute pipelines: %llu built at their dispatches by the queue worker\n", static_cast<unsigned long long>(after.dispatchBuilds - before.dispatchBuilds));
    std::printf("  lookahead: %llu draws and %llu dispatches decoded ahead in %.1f ms on the worker; pool of %u threads built %llu pipelines in %.1f ms of thread time, %llu already present, %llu failed\n", static_cast<unsigned long long>(after.walk.draws - before.walk.draws), static_cast<unsigned long long>(after.walk.dispatches - before.walk.dispatches), after.walk.resolveMs - before.walk.resolveMs, after.pool.threads, static_cast<unsigned long long>(after.pool.built - before.pool.built), after.pool.buildMs - before.pool.buildMs, static_cast<unsigned long long>(after.pool.present - before.pool.present), static_cast<unsigned long long>(after.pool.failed - before.pool.failed));
}

}

int main(int argc, char** argv) {
    try {
        {
            const auto device = OpenVulkanTestDevice();
            if (!device) return VulkanTestSkipped;
        }
        const bool enabled = AgcDriver::Graphics::SubmitLookaheadEnabled();
        if (argc >= 2 && std::string(argv[1]) == "--benchmark") {
            Benchmark(argc >= 3 ? static_cast<std::uint32_t>(std::strtoul(argv[2], nullptr, 10)) : 24u, argc >= 4 ? static_cast<std::uint32_t>(std::strtoul(argv[3], nullptr, 10)) : 0u, enabled);
            AgcDriverShutdown_nid_postfix();
            return 0;
        }
        LookaheadBuildsBeforeTheWorker(enabled);
        AgcDriverShutdown_nid_postfix();
        std::cout << "submit lookahead " << (enabled ? "built the pipelines of a gated submission before its worker reached them" : "off: the worker built the pipelines itself") << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        try {
            AgcDriverShutdown_nid_postfix();
        } catch (const std::exception& shutdownError) {
            std::cerr << shutdownError.what() << '\n';
        }
        return 1;
    }
}
