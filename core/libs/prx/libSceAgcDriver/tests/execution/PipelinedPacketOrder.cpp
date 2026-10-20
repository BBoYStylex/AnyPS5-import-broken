#include "prx/libSceAgcDriver/Execution/include/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Draw/DrawPipeline.hpp"
#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using AgcDriver::DriverDetail::DrawPipeline;

constexpr std::uint32_t CondExecOpcode = 0x22;

void check(bool condition, const std::string& what) {
    if (!condition) throw std::runtime_error("pipelined packet order: " + what);
}

std::array<std::uint32_t, 5> CondExec(std::uint64_t address) {
    return {0xc0032200u, static_cast<std::uint32_t>(address), static_cast<std::uint32_t>(address >> 32u), 0u, 0u};
}

void Submit(std::vector<std::uint32_t>& words) {
    Packet packet{words.data(), static_cast<std::uint32_t>(words.size()), 0, {}};
    check(sceAgcDriverSubmitDcb(&packet) == 0, "submission failed");
}

struct Gate {
    std::atomic<bool> release{false};
    ~Gate() { release.store(true, std::memory_order_release); }
};

void Hold(Gate& gate, std::uint64_t address, std::size_t bytes) {
    DrawPipeline::Queue0().Enqueue([&gate] {
        while (!gate.release.load(std::memory_order_acquire)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }, {{address, address + bytes}});
}

void ExpectDrains(std::vector<std::uint32_t>& words, std::uint32_t opcode, Gate& gate, const char* what) {
    const auto before = DrawPipeline::DrainRequestsByOpcode(opcode);
    std::thread releaser([&] {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (DrawPipeline::DrainRequestsByOpcode(opcode) == before && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        gate.release.store(true, std::memory_order_release);
    });
    Submit(words);
    releaser.join();
    check(DrawPipeline::DrainRequestsByOpcode(opcode) == before + 1, what);
}

void ExpectNoDrain(std::uint32_t opcode, std::uint64_t before, const char* what) {
    check(DrawPipeline::DrainRequestsByOpcode(opcode) == before, what);
}

}

int main() {
#ifdef _WIN32
    _putenv_s("APS5_PIPELINED_DRAWS", "1");
#else
    setenv("APS5_PIPELINED_DRAWS", "1", 1);
#endif
    try {
        alignas(256) static std::array<std::uint32_t, 4> pending{};
        alignas(256) static std::array<std::uint32_t, 4> elsewhere{};
        pending.fill(0);
        elsewhere.fill(0);
        const auto pendingAddress = reinterpret_cast<std::uint64_t>(pending.data());
        const auto elsewhereAddress = reinterpret_cast<std::uint64_t>(elsewhere.data());
        std::vector<std::uint32_t> words;
        {
            Gate gate;
            Hold(gate, pendingAddress, sizeof(pending));
            const auto before = DrawPipeline::DrainRequestsByOpcode(CondExecOpcode);
            words.clear();
            const auto packet = CondExec(elsewhereAddress);
            words.insert(words.end(), packet.begin(), packet.end());
            Submit(words);
            ExpectNoDrain(CondExecOpcode, before, "a conditional read of memory the queued work does not touch drained the pipeline");
        }
        AgcDriverWaitIdle_nid_postfix();
        {
            Gate gate;
            Hold(gate, pendingAddress, sizeof(pending));
            words.clear();
            const auto packet = CondExec(pendingAddress);
            words.insert(words.end(), packet.begin(), packet.end());
            ExpectDrains(words, CondExecOpcode, gate, "a conditional read of the queued work's memory did not drain the pipeline");
        }
        AgcDriverWaitIdle_nid_postfix();
        {
            Gate gate;
            Hold(gate, pendingAddress, sizeof(pending));
            const auto before = DrawPipeline::DrainRequestsByOpcode(CondExecOpcode);
            words.clear();
            const auto packet = CondExec(pendingAddress + sizeof(pending));
            words.insert(words.end(), packet.begin(), packet.end());
            Submit(words);
            ExpectNoDrain(CondExecOpcode, before, "a conditional read past the queued work's memory drained the pipeline");
        }
        AgcDriverWaitIdle_nid_postfix();
        LibcRunShutdown_nid_postfix();
        std::puts("pipelined packet order tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        try { LibcRunShutdown_nid_postfix(); } catch (...) {}
        return 1;
    }
}
