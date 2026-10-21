#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {
int* APS5_VABI __error_nid_postfix();
int APS5_VABI fcntl_nid_postfix(int descriptor, int command, ...);
int APS5_VABI sceKernelFstat(int descriptor, FileStat* status);
std::int64_t APS5_VABI sceKernelPread(int descriptor, void* buffer, std::size_t length, std::int64_t offset);
std::int64_t APS5_VABI sceKernelPwrite(int descriptor, const void* buffer, std::size_t length, std::int64_t offset);
int APS5_VABI sceKernelFsync(int descriptor);
int APS5_VABI raise_nid_postfix(int signal);
int APS5_VABI pthread_getthreadid_np_nid_postfix(void);
}

namespace {

constexpr int GuestEbadf = 9;
constexpr int GuestEfault = 14;
constexpr int GuestEinval = 22;
constexpr int GuestEagain = 35;
constexpr int GuestEinprogress = 36;
constexpr int GuestEopnotsupp = 45;
constexpr int GuestGetDescriptorFlags = 1;
constexpr int GuestGetStatusFlags = 3;
constexpr int GuestAccessMode = 3;
constexpr int GuestReadOnly = 0;
constexpr int GuestWriteOnly = 1;
constexpr int GuestSync = 0x80;
constexpr int GuestFileType = 0170000;
constexpr int GuestDirectory = 0040000;
constexpr int GuestCharacterDevice = 0020000;
constexpr int GuestRegularFile = 0100000;
constexpr int GuestSocket = 0140000;
constexpr int LioWrite = 1;
constexpr int LioRead = 2;
constexpr int LioSync = 3;
constexpr int SigevNone = 0;
constexpr int SigevSignal = 1;
constexpr int SigevKevent = 3;
constexpr int SigevThreadId = 4;
constexpr int MaxSignal = 128;
constexpr std::uint16_t KeventFlags = 0x0010 | 0x0020 | 0x0080;
constexpr int AioNotCanceled = 2;
constexpr int AioAllDone = 3;
constexpr int AioListioMax = 16;
constexpr std::size_t MaxJobsPerProcess = 256;
constexpr std::int64_t NanosecondsPerSecond = 1000000000;

struct GuestSigevent {
    int notify;
    int signo;
    void* value;
    union {
        std::int32_t threadId;
        std::uint16_t keventFlags;
        std::int64_t spare[8];
    };
};
static_assert(sizeof(GuestSigevent) == 80 && offsetof(GuestSigevent, threadId) == 16);

struct GuestAiocb {
    int fildes;
    std::int64_t offset;
    void* buf;
    std::size_t nbytes;
    int spare[2];
    void* spare2;
    int lioOpcode;
    int reqprio;
    std::int64_t status;
    std::int64_t error;
    void* kernelinfo;
    GuestSigevent sigevent;
};
static_assert(sizeof(GuestAiocb) == 160 && offsetof(GuestAiocb, lioOpcode) == 48 && offsetof(GuestAiocb, status) == 56);
static_assert(offsetof(GuestAiocb, kernelinfo) == 72 && offsetof(GuestAiocb, sigevent) == 80);

struct Job {
    GuestAiocb* guest;
    int descriptor;
    bool finished;
    std::uint64_t completion;
    std::int64_t status;
    std::int64_t error;
};

enum class Target { Vnode, Device, Socket, Other };

using Deadline = std::optional<std::chrono::steady_clock::time_point>;

std::mutex g_mutex;
std::condition_variable g_completed;
bool g_started = false;
std::list<Job> g_jobs;
std::uint64_t g_completions = 0;
std::intptr_t g_nextJobId = 1;

int Fail(int error) {
    *__error_nid_postfix() = error;
    return -1;
}

int Rejected(GuestAiocb* job, int error) {
    job->error = error;
    return Fail(error);
}

Target Classify(int descriptor) {
    FileStat status{};
    if (sceKernelFstat(descriptor, &status) != 0) return Target::Other;
    switch (status.st_mode & GuestFileType) {
    case GuestRegularFile:
    case GuestDirectory: return Target::Vnode;
    case GuestCharacterDevice: return Target::Device;
    case GuestSocket: return Target::Socket;
    default: return Target::Other;
    }
}

bool ValidTimeout(const KernelTimespec& timeout) {
    return timeout.tv_nsec >= 0 && timeout.tv_nsec < NanosecondsPerSecond && timeout.tv_sec >= 0;
}

std::chrono::steady_clock::time_point DeadlineAfter(const KernelTimespec& timeout) {
    const auto seconds = std::min<std::int64_t>(timeout.tv_sec, std::numeric_limits<std::int32_t>::max());
    return std::chrono::steady_clock::now() + std::chrono::seconds(seconds) + std::chrono::nanoseconds(timeout.tv_nsec);
}

bool Wait(std::unique_lock<std::mutex>& lock, const Deadline& deadline) {
    if (!deadline) {
        g_completed.wait(lock);
        return true;
    }
    return g_completed.wait_until(lock, *deadline) == std::cv_status::no_timeout;
}

std::list<Job>::iterator FirstDone(const GuestAiocb* guest) {
    auto first = g_jobs.end();
    for (auto job = g_jobs.begin(); job != g_jobs.end(); ++job) {
        if (!job->finished || (guest != nullptr && job->guest != guest)) continue;
        if (first == g_jobs.end() || job->completion < first->completion) first = job;
    }
    return first;
}

std::int64_t Collect(std::list<Job>::iterator job, std::unique_lock<std::mutex>& lock) {
    GuestAiocb* guest = job->guest;
    const std::int64_t status = job->status;
    const std::int64_t error = job->error;
    g_jobs.erase(job);
    lock.unlock();
    guest->error = error;
    guest->status = status;
    return error == 0 ? status : Fail(static_cast<int>(error));
}

void Run(const GuestAiocb& request, int opcode, std::int64_t& status, std::int64_t& error) {
    std::int64_t result = 0;
    if (opcode == LioSync) result = sceKernelFsync(request.fildes);
    else if (opcode == LioWrite) result = sceKernelPwrite(request.fildes, request.buf, request.nbytes, request.offset);
    else if (request.nbytes != 0) result = sceKernelPread(request.fildes, request.buf, request.nbytes, request.offset);
    status = result < 0 ? -1 : result;
    error = result < 0 ? result & 0xffff : 0;
}

int Submit(GuestAiocb* job, int opcode, const std::string& function) {
    {
        std::lock_guard lock(g_mutex);
        g_started = true;
        if (job != nullptr) {
            job->status = -1;
            job->error = 0;
            job->kernelinfo = reinterpret_cast<void*>(std::intptr_t{-1});
        }
        if (g_jobs.size() >= MaxJobsPerProcess) {
            if (job != nullptr) job->error = GuestEagain;
            return Fail(GuestEagain);
        }
    }
    if (job == nullptr) return Fail(GuestEfault);
    const GuestAiocb request = *job;
    if (request.nbytes > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) return Fail(GuestEinval);
    const int notify = request.sigevent.notify;
    if (notify != SigevNone && notify != SigevSignal && notify != SigevKevent && notify != SigevThreadId) return Rejected(job, GuestEinval);
    const bool signals = notify == SigevSignal || notify == SigevThreadId;
    if (signals && (request.sigevent.signo <= 0 || request.sigevent.signo > MaxSignal)) return Fail(GuestEinval);
    const int status = fcntl_nid_postfix(request.fildes, GuestGetStatusFlags);
    const int access = status & GuestAccessMode;
    if (status < 0 || (opcode == LioRead && access == GuestWriteOnly) || (opcode == LioWrite && access == GuestReadOnly))
        return Rejected(job, GuestEbadf);
    const Target target = Classify(request.fildes);
    if (opcode == LioSync && (target == Target::Socket || target == Target::Other)) return Rejected(job, GuestEinval);
    if (opcode != LioSync && request.offset == -1) return Rejected(job, GuestEinval);
    {
        std::lock_guard lock(g_mutex);
        job->kernelinfo = reinterpret_cast<void*>(g_nextJobId++);
    }
    if (notify == SigevKevent) {
        if ((request.sigevent.keventFlags & ~KeventFlags) != 0) return Rejected(job, GuestEinval);
        NotImplemented_nid_no_patch((function + ": SIGEV_KEVENT completion notification").c_str());
    }
    if (notify == SigevThreadId && request.sigevent.threadId != pthread_getthreadid_np_nid_postfix())
        NotImplemented_nid_no_patch((function + ": SIGEV_THREAD_ID completion notification to thread " + std::to_string(request.sigevent.threadId) + ", which is not the caller").c_str());
    job->error = GuestEinprogress;
    if (opcode != LioSync && target == Target::Socket) NotImplemented_nid_no_patch((function + ": a request on socket descriptor " + std::to_string(request.fildes)).c_str());
    if (target != Target::Vnode) return Rejected(job, GuestEopnotsupp);
    std::list<Job>::iterator entry;
    {
        std::lock_guard lock(g_mutex);
        entry = g_jobs.insert(g_jobs.end(), Job{job, request.fildes, false, 0, -1, 0});
    }
    std::int64_t result = 0;
    std::int64_t error = 0;
    Run(request, opcode, result, error);
    {
        std::lock_guard lock(g_mutex);
        entry->finished = true;
        entry->completion = ++g_completions;
        entry->status = result;
        entry->error = error;
    }
    g_completed.notify_all();
    if (signals) raise_nid_postfix(request.sigevent.signo);
    return 0;
}

}

extern "C" {

int APS5_VABI aio_read_nid_postfix(GuestAiocb* job) {
    return Submit(job, LioRead, "aio_read");
}

int APS5_VABI aio_write_nid_postfix(GuestAiocb* job) {
    return Submit(job, LioWrite, "aio_write");
}

int APS5_VABI aio_fsync_nid_postfix(int operation, GuestAiocb* job) {
    if (operation != GuestSync) return Fail(GuestEinval);
    return Submit(job, LioSync, "aio_fsync");
}

int APS5_VABI aio_error_nid_postfix(const GuestAiocb* job) {
    {
        std::lock_guard lock(g_mutex);
        if (!g_started) return GuestEinval;
        for (const auto& entry : g_jobs) {
            if (entry.guest == job) return entry.finished ? static_cast<int>(entry.error) : GuestEinprogress;
        }
    }
    if (job == nullptr) return -1;
    return job->status == -1 ? static_cast<int>(job->error) : GuestEinval;
}

std::int64_t APS5_VABI aio_return_nid_postfix(GuestAiocb* job) {
    std::unique_lock lock(g_mutex);
    const auto done = job == nullptr ? g_jobs.end() : FirstDone(job);
    if (done == g_jobs.end()) return Fail(GuestEinval);
    return Collect(done, lock);
}

int APS5_VABI aio_suspend_nid_postfix(const GuestAiocb* const* list, int count, const KernelTimespec* timeout) {
    if (count < 0 || count > AioListioMax) return Fail(GuestEinval);
    if (list == nullptr && count > 0) return Fail(GuestEfault);
    const std::vector<const GuestAiocb*> wanted(list, list + count);
    Deadline deadline;
    if (timeout != nullptr) {
        if (!ValidTimeout(*timeout)) return Fail(GuestEinval);
        deadline = DeadlineAfter(*timeout);
    }
    std::unique_lock lock(g_mutex);
    if (!g_started) return Fail(GuestEagain);
    for (;;) {
        bool listed = false;
        for (const auto& job : g_jobs) {
            if (std::find(wanted.begin(), wanted.end(), job.guest) == wanted.end()) continue;
            if (job.finished) return 0;
            listed = true;
        }
        if (!listed) return 0;
        if (!Wait(lock, deadline)) return Fail(GuestEagain);
    }
}

int APS5_VABI aio_cancel_nid_postfix(int descriptor, GuestAiocb* job) {
    if (fcntl_nid_postfix(descriptor, GuestGetDescriptorFlags) < 0) return Fail(GuestEbadf);
    std::lock_guard lock(g_mutex);
    for (const auto& entry : g_jobs) {
        if (!entry.finished && entry.descriptor == descriptor && (job == nullptr || entry.guest == job)) return AioNotCanceled;
    }
    return AioAllDone;
}

std::int64_t APS5_VABI aio_waitcomplete_nid_postfix(GuestAiocb** completed, const KernelTimespec* timeout) {
    if (completed != nullptr) *completed = nullptr;
    const bool poll = timeout != nullptr && timeout->tv_sec == 0 && timeout->tv_nsec == 0;
    Deadline deadline;
    if (timeout != nullptr && !poll) {
        if (!ValidTimeout(*timeout)) return Fail(GuestEinval);
        deadline = DeadlineAfter(*timeout);
    }
    std::unique_lock lock(g_mutex);
    g_started = true;
    auto done = FirstDone(nullptr);
    while (done == g_jobs.end()) {
        if (poll || !Wait(lock, deadline)) return Fail(GuestEagain);
        done = FirstDone(nullptr);
    }
    if (completed != nullptr) *completed = done->guest;
    return Collect(done, lock);
}

}
