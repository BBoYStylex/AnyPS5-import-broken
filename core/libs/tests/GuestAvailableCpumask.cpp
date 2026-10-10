#include "SceTypes.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <bit>
#include <chrono>
#include <future>
#include <thread>

extern "C" {
KernelCpumask APS5_VABI sceKernelGetAvailableCpumask(void);
int APS5_VABI scePthreadCreate(Pthread* thread, const PthreadAttr* attr, PthreadEntry entry, void* arg, const char* name);
int APS5_VABI scePthreadJoin(Pthread thread, void** retval);
int APS5_VABI scePthreadGetaffinity(Pthread thread, KernelCpumask* mask);
int APS5_VABI scePthreadSetaffinity(Pthread thread, KernelCpumask mask);
int APS5_VABI scePthreadAttrInit(PthreadAttr* attr);
int APS5_VABI scePthreadAttrDestroy(PthreadAttr* attr);
int APS5_VABI scePthreadAttrGetaffinity(const PthreadAttr* attr, KernelCpumask* mask);
int APS5_VABI cpuset_getaffinity_nid_postfix(int level, int which, std::int64_t id, std::size_t size, void* mask);
int* APS5_VABI __error_nid_postfix();
int APS5_VABI scePthreadGetthreadid(void);
int APS5_VABI getpid_nid_postfix(void);
Pthread APS5_VABI scePthreadSelf();
}

static constexpr int SCE_OK = 0;
static constexpr int PS5_LOGICAL_CPUS = 16;

static void Require(bool value) { if (!value) std::abort(); }

static void* APS5_VABI Worker(void* arg) {
    static_cast<std::future<void>*>(arg)->get();
    return nullptr;
}

static void* APS5_VABI ReportCpuset(void* arg) {
    auto* result = static_cast<std::uint64_t*>(arg);
    std::uint64_t set[4] = {~0ull, ~0ull, ~0ull, ~0ull};
    result[0] = cpuset_getaffinity_nid_postfix(3, 1, -1, 8, set);
    result[1] = set[0];
    result[2] = set[1];
    result[3] = cpuset_getaffinity_nid_postfix(3, 1, -1, sizeof(set), set);
    result[4] = set[0] | (set[1] != 0 || set[2] != 0 || set[3] != 0 ? 1ull << 63 : 0);
    result[5] = cpuset_getaffinity_nid_postfix(3, 1, -1, 4, set) == -1 && *__error_nid_postfix() == 34;
    result[6] = cpuset_getaffinity_nid_postfix(3, 1, -1, 33, set) == -1 && *__error_nid_postfix() == 34;
    result[7] = cpuset_getaffinity_nid_postfix(3, 1, -1, 8, nullptr) == -1 && *__error_nid_postfix() == 14;
    return nullptr;
}

static std::uint64_t Affinity(int level, int which, std::int64_t id) {
    std::uint64_t set[4] = {~0ull, ~0ull, ~0ull, ~0ull};
    if (cpuset_getaffinity_nid_postfix(level, which, id, sizeof(set), set) != 0) return ~0ull;
    Require(set[1] == 0 && set[2] == 0 && set[3] == 0);
    return set[0];
}

static int Error(int level, int which, std::int64_t id, void* mask) {
    if (cpuset_getaffinity_nid_postfix(level, which, id, 8, mask) != -1) return 0;
    return *__error_nid_postfix();
}

struct Pinned {
    std::future<void>* release;
    std::int64_t tid;
};

static void* APS5_VABI PinnedWorker(void* arg) {
    auto* pinned = static_cast<Pinned*>(arg);
    pinned->tid = scePthreadGetthreadid();
    pinned->release->get();
    return nullptr;
}

static void* APS5_VABI ReportMask(void* arg) {
    *static_cast<KernelCpumask*>(arg) = sceKernelGetAvailableCpumask();
    return nullptr;
}

int main() {
    const KernelCpumask available = sceKernelGetAvailableCpumask();
    Require(available != 0);
    Require((available >> PS5_LOGICAL_CPUS) == 0);
    Require(sceKernelGetAvailableCpumask() == available);

    PthreadAttr attr = nullptr;
    Require(scePthreadAttrInit(&attr) == SCE_OK);
    KernelCpumask defaultAffinity = 0;
    Require(scePthreadAttrGetaffinity(&attr, &defaultAffinity) == SCE_OK);
    Require(defaultAffinity == available);

    std::promise<void> release;
    auto released = release.get_future();
    Pthread thread = nullptr;
    Require(scePthreadCreate(&thread, &attr, Worker, &released, nullptr) == SCE_OK);
    KernelCpumask threadAffinity = 0;
    Require(scePthreadGetaffinity(thread, &threadAffinity) == SCE_OK);
    Require(threadAffinity == available);

    for (KernelCpumask cpu = 1; cpu != 0; cpu <<= 1) {
        if ((available & cpu) == 0) continue;
        Require(scePthreadSetaffinity(thread, cpu) == SCE_OK);
        Require(scePthreadGetaffinity(thread, &threadAffinity) == SCE_OK);
        Require(threadAffinity == cpu);
    }
    release.set_value();
    Require(scePthreadJoin(thread, nullptr) == SCE_OK);

    KernelCpumask fromThread = 0;
    Require(scePthreadCreate(&thread, &attr, ReportMask, &fromThread, nullptr) == SCE_OK);
    Require(scePthreadJoin(thread, nullptr) == SCE_OK);
    Require(fromThread == available);

    std::uint64_t cpuset[8] = {};
    Require(scePthreadCreate(&thread, &attr, ReportCpuset, cpuset, nullptr) == SCE_OK);
    Require(scePthreadJoin(thread, nullptr) == SCE_OK);
    Require(cpuset[0] == 0 && cpuset[1] == available && cpuset[2] == ~0ull);
    Require(cpuset[3] == 0 && cpuset[4] == available);
    Require(cpuset[5] == 1 && cpuset[6] == 1 && cpuset[7] == 1);

    constexpr int LevelRoot = 1, LevelCpuset = 2, LevelWhich = 3, WhichTid = 1, WhichPid = 2;
    constexpr int Esrch = 3, Efault = 14, Einval = 22;
    std::uint64_t set = 0;
    Require(scePthreadSetaffinity(scePthreadSelf(), available) == SCE_OK);
    const std::int64_t self = scePthreadGetthreadid();
    Require(Affinity(LevelWhich, WhichTid, -1) == available);
    Require(Affinity(LevelWhich, WhichTid, self) == available);
    Require(Affinity(LevelWhich, WhichPid, -1) == available);
    Require(Affinity(LevelWhich, WhichPid, getpid_nid_postfix()) == available);

    const KernelCpumask low = available & -available;
    const KernelCpumask high = KernelCpumask{1} << (63 - std::countl_zero(available));
    Require(low != high);
    std::promise<void> unpin;
    auto unpinned = unpin.get_future();
    Pinned pinned{&unpinned, 0};
    Require(scePthreadCreate(&thread, &attr, PinnedWorker, &pinned, nullptr) == SCE_OK);
    Require(scePthreadSetaffinity(thread, high) == SCE_OK);
    Require(scePthreadSetaffinity(scePthreadSelf(), low) == SCE_OK);
    while (__atomic_load_n(&pinned.tid, __ATOMIC_ACQUIRE) == 0) std::this_thread::yield();
    Require(pinned.tid != self);
    Require(Affinity(LevelWhich, WhichTid, pinned.tid) == high);
    Require(Affinity(LevelWhich, WhichTid, -1) == low);
    Require(Affinity(LevelWhich, WhichPid, -1) == (low | high));
    unpin.set_value();
    Require(scePthreadJoin(thread, nullptr) == SCE_OK);
    Require(Affinity(LevelWhich, WhichPid, -1) == low);
    Require(Error(LevelWhich, WhichTid, pinned.tid, nullptr) == Esrch);

    std::promise<void> finish;
    auto finished = finish.get_future();
    finish.set_value();
    Pinned exited{&finished, 0};
    Require(scePthreadCreate(&thread, &attr, PinnedWorker, &exited, nullptr) == SCE_OK);
    Require(scePthreadSetaffinity(thread, high) == SCE_OK);
    while (__atomic_load_n(&exited.tid, __ATOMIC_ACQUIRE) == 0) std::this_thread::yield();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (Error(LevelWhich, WhichTid, exited.tid, &set) != Esrch) {
        Require(std::chrono::steady_clock::now() < deadline);
        std::this_thread::yield();
    }
    Require(Affinity(LevelWhich, WhichPid, -1) == low);
    Require(scePthreadJoin(thread, nullptr) == SCE_OK);
    Require(scePthreadSetaffinity(scePthreadSelf(), available) == SCE_OK);

    Require(Error(LevelWhich, WhichTid, 1, &set) == Esrch);
    Require(Error(LevelWhich, WhichPid, getpid_nid_postfix() + 1, &set) == Esrch);
    Require(Error(LevelWhich, 0, -1, &set) == Einval);
    Require(Error(LevelWhich, 7, -1, &set) == Einval);
    Require(Error(0, WhichTid, -1, &set) == Einval);
    Require(Error(4, WhichPid, -1, &set) == Einval);
    Require(Error(4, WhichTid, 1, &set) == Esrch);
    Require(Error(LevelRoot, WhichTid, 1, &set) == Esrch);
    Require(Error(LevelCpuset, 0, -1, &set) == Einval);
    Require(Error(4, WhichPid, -1, nullptr) == Einval);
    Require(Error(LevelWhich, WhichPid, -1, nullptr) == Efault);
    Require(scePthreadAttrDestroy(&attr) == SCE_OK);
}
