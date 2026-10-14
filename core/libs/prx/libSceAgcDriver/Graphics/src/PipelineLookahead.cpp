#include "prx/libSceAgcDriver/Graphics/include/PipelineLookahead.hpp"
#include "prx/libSceAgcDriver/Graphics/include/PipelineJournal.hpp"
#include "prx/libSceAgcDriver/Execution/include/ProfileOutput.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <utility>

namespace AgcDriver::Graphics {

namespace {

struct LookaheadCounters {
    std::atomic<std::uint64_t> enqueued{0};
    std::atomic<std::uint64_t> built{0};
    std::atomic<std::uint64_t> present{0};
    std::atomic<std::uint64_t> failed{0};
    std::atomic<std::uint64_t> dropped{0};
    std::atomic<std::uint64_t> buildNs{0};
    std::atomic<std::uint32_t> threads{0};
};

LookaheadCounters& lookaheadCounters() {
    static LookaheadCounters counters;
    return counters;
}

}

unsigned SubmitLookaheadThreads() {
    static const auto threads = static_cast<unsigned>(std::min<std::uint64_t>(PipelineSwitch("APS5_SUBMIT_LOOKAHEAD_THREADS", std::max(1u, std::thread::hardware_concurrency() / 2u)), 64u));
    return threads;
}

bool SubmitLookaheadEnabled() {
    static const bool enabled = std::getenv("APS5_NO_SUBMIT_LOOKAHEAD") == nullptr && SubmitLookaheadThreads() != 0;
    return enabled;
}

PipelineLookaheadCounters PipelineLookaheadTotals() {
    const auto& counters = lookaheadCounters();
    PipelineLookaheadCounters totals;
    totals.enqueued = counters.enqueued.load(std::memory_order_relaxed);
    totals.built = counters.built.load(std::memory_order_relaxed);
    totals.present = counters.present.load(std::memory_order_relaxed);
    totals.failed = counters.failed.load(std::memory_order_relaxed);
    totals.dropped = counters.dropped.load(std::memory_order_relaxed);
    totals.buildMs = static_cast<double>(counters.buildNs.load(std::memory_order_relaxed)) / 1e6;
    totals.threads = counters.threads.load(std::memory_order_relaxed);
    return totals;
}

void ReportPipelineLookahead() {
    static const bool profile = std::getenv("APS5_PROFILE_DRAW") != nullptr;
    if (!profile) return;
    const auto totals = PipelineLookaheadTotals();
    if (totals.enqueued == 0) return;
    AgcDriver::ProfilePrint_nid_no_patch("[lookahead] pool of %u threads: %llu pipelines enqueued, %llu built in %.0f ms, %llu already present, %llu failed, %llu dropped at teardown\n", totals.threads, static_cast<unsigned long long>(totals.enqueued), static_cast<unsigned long long>(totals.built), totals.buildMs, static_cast<unsigned long long>(totals.present), static_cast<unsigned long long>(totals.failed), static_cast<unsigned long long>(totals.dropped));
}

PipelineLookaheadPool::PipelineLookaheadPool(unsigned threads) {
    const auto count = std::max(1u, threads);
    lookaheadCounters().threads.store(count, std::memory_order_relaxed);
    workers.reserve(count);
    for (unsigned index = 0; index < count; ++index) workers.emplace_back([this] { run(); });
}

PipelineLookaheadPool::~PipelineLookaheadPool() {
    Cancel();
    Wait();
}

void PipelineLookaheadPool::Enqueue(Build build) {
    {
        std::lock_guard lock(mutex);
        if (cancelled) {
            lookaheadCounters().dropped.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        queue.push_back(std::move(build));
        lookaheadCounters().enqueued.fetch_add(1, std::memory_order_relaxed);
    }
    changed.notify_one();
}

void PipelineLookaheadPool::Cancel() {
    {
        std::lock_guard lock(mutex);
        cancelled = true;
        lookaheadCounters().dropped.fetch_add(queue.size(), std::memory_order_relaxed);
        queue.clear();
    }
    changed.notify_all();
    settled.notify_all();
}

void PipelineLookaheadPool::Wait() {
    Cancel();
    for (auto& worker : workers) {
        if (worker.joinable()) worker.join();
    }
    workers.clear();
}

void PipelineLookaheadPool::Drain() {
    std::unique_lock lock(mutex);
    settled.wait(lock, [&] { return cancelled || (queue.empty() && running == 0); });
}

std::size_t PipelineLookaheadPool::Pending() const {
    std::lock_guard lock(mutex);
    return queue.size() + running;
}

void PipelineLookaheadPool::run() {
    auto& counters = lookaheadCounters();
    for (;;) {
        Build build;
        {
            std::unique_lock lock(mutex);
            changed.wait(lock, [&] { return cancelled || !queue.empty(); });
            if (cancelled) return;
            build = std::move(queue.front());
            queue.pop_front();
            ++running;
        }
        const auto started = std::chrono::steady_clock::now();
        try {
            (build() ? counters.built : counters.present).fetch_add(1, std::memory_order_relaxed);
        } catch (const std::exception& error) {
            counters.failed.fetch_add(1, std::memory_order_relaxed);
            static std::atomic<std::uint64_t> reported{0};
            if (reported.fetch_add(1) < 8) std::fprintf(stderr, "[lookahead] pipeline build failed: %s\n", error.what());
        } catch (...) {
            counters.failed.fetch_add(1, std::memory_order_relaxed);
        }
        counters.buildNs.fetch_add(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - started).count()), std::memory_order_relaxed);
        {
            std::lock_guard lock(mutex);
            --running;
            if (running == 0 && queue.empty()) settled.notify_all();
        }
    }
}

}
