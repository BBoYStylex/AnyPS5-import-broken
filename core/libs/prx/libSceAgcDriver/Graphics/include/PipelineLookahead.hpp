#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_PIPELINELOOKAHEAD_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_PIPELINELOOKAHEAD_HPP

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace AgcDriver::Graphics {

struct PipelineLookaheadCounters {
    std::uint64_t enqueued = 0;
    std::uint64_t built = 0;
    std::uint64_t present = 0;
    std::uint64_t failed = 0;
    std::uint64_t dropped = 0;
    double buildMs = 0;
    std::uint32_t threads = 0;
};

[[nodiscard]] bool SubmitLookaheadEnabled();
[[nodiscard]] unsigned SubmitLookaheadThreads();
[[nodiscard]] PipelineLookaheadCounters PipelineLookaheadTotals();
void ReportPipelineLookahead();

class PipelineLookaheadPool {
public:
    using Build = std::function<bool()>;
    explicit PipelineLookaheadPool(unsigned threads);
    ~PipelineLookaheadPool();
    PipelineLookaheadPool(const PipelineLookaheadPool&) = delete;
    PipelineLookaheadPool& operator=(const PipelineLookaheadPool&) = delete;
    void Enqueue(Build build);
    void Cancel();
    void Wait();
    void Drain();
    [[nodiscard]] std::size_t Pending() const;

private:
    void run();
    mutable std::mutex mutex;
    std::condition_variable changed;
    std::condition_variable settled;
    std::deque<Build> queue;
    std::vector<std::thread> workers;
    unsigned running = 0;
    bool cancelled = false;
};

}

#endif
