#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "Pthread/include/Pthread.hpp"
#include "Pthread/Posix/Common.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

extern "C" {
int APS5_VABI scePthreadCondTimedwait(PthreadCond* cond, PthreadMutex* mutex, KernelUseconds usec);
std::filesystem::path ResolvePath_nid_no_patch(const char* path);
}

namespace {

std::mutex spinMutex;
std::map<void*, std::unique_ptr<std::atomic<int>>> spinlocks;

std::atomic<int>* FindSpin(void* lock) {
    const std::lock_guard lockGuard{spinMutex};
    const auto it = spinlocks.find(lock);
    return it == spinlocks.end() ? nullptr : it->second.get();
}

}  // namespace

extern "C" {

int APS5_VABI pthread_attr_getstackaddr_nid_postfix(const PthreadAttr* attr, void** stackaddr) {
    if (!attr || !*attr || !stackaddr) return PosixThread::GUEST_EINVAL;
    *stackaddr = (*attr)->stackAddress;
    return 0;
}

int APS5_VABI pthread_attr_getinheritsched_nid_postfix(const PthreadAttr* attr, int* inheritsched) {
    if (!attr || !*attr || !inheritsched) return PosixThread::GUEST_EINVAL;
    *inheritsched = (*attr)->_inheritsched;
    return 0;
}

namespace {
std::mutex scopeMutex;
std::map<const PthreadAttr*, int> attrScopes;
}

int APS5_VABI pthread_attr_getscope_nid_postfix(const PthreadAttr* attr, int* contentionscope) {
    if (!attr || !*attr || !contentionscope) return PosixThread::GUEST_EINVAL;
    {
        const std::lock_guard lockGuard{scopeMutex};
        const auto it = attrScopes.find(attr);
        *contentionscope = it == attrScopes.end() ? 1 : it->second;
    }
    return 0;
}

int APS5_VABI pthread_attr_setscope_nid_postfix(PthreadAttr* attr, int contentionscope) {
    if (!attr || !*attr) return PosixThread::GUEST_EINVAL;
    if (contentionscope != 0 && contentionscope != 1) return PosixThread::GUEST_EINVAL;
    const std::lock_guard lockGuard{scopeMutex};
    attrScopes[attr] = contentionscope;
    return 0;
}

int APS5_VABI pthread_getaffinity_np_nid_postfix(Pthread thread, std::size_t cpusetsize, void* cpuset) {
    if (!thread || !cpuset || cpusetsize < sizeof(KernelCpumask)) return PosixThread::GUEST_EINVAL;
    const auto affinity = thread->affinity.load(std::memory_order_relaxed);
    std::memcpy(cpuset, &affinity, sizeof(affinity));
    return 0;
}

int APS5_VABI pthread_cond_reltimedwait_np_nid_postfix(PthreadCond* cond, PthreadMutex* mutex,
                                                       const KernelTimespec* reltime) {
    if (!reltime) return PosixThread::GUEST_EINVAL;
    const auto usec = static_cast<KernelUseconds>(reltime->tv_sec) * 1000000u +
                      static_cast<KernelUseconds>(reltime->tv_nsec / 1000);
    return scePthreadCondTimedwait(cond, mutex, usec);
}

int APS5_VABI pthread_spin_init_nid_postfix(void* lock, int /*pshared*/) {
    if (!lock) return PosixThread::GUEST_EINVAL;
    const std::lock_guard lockGuard{spinMutex};
    spinlocks[lock] = std::make_unique<std::atomic<int>>(0);
    return 0;
}

int APS5_VABI pthread_spin_destroy_nid_postfix(void* lock) {
    if (!lock) return PosixThread::GUEST_EINVAL;
    const std::lock_guard lockGuard{spinMutex};
    return spinlocks.erase(lock) == 1 ? 0 : PosixThread::GUEST_EINVAL;
}

int APS5_VABI pthread_spin_trylock_nid_postfix(void* lock) {
    auto* spin = FindSpin(lock);
    if (!spin) return PosixThread::GUEST_EINVAL;
    int expected = 0;
    return spin->compare_exchange_strong(expected, 1, std::memory_order_acquire) ? 0 : 16 /* EBUSY */;
}

int APS5_VABI pthread_spin_lock_nid_postfix(void* lock) {
    auto* spin = FindSpin(lock);
    if (!spin) return PosixThread::GUEST_EINVAL;
    for (;;) {
        int expected = 0;
        if (spin->compare_exchange_strong(expected, 1, std::memory_order_acquire)) return 0;
#ifdef _WIN32
        YieldProcessor();
#else
        sched_yield();
#endif
    }
}

int APS5_VABI pthread_spin_unlock_nid_postfix(void* lock) {
    auto* spin = FindSpin(lock);
    if (!spin) return PosixThread::GUEST_EINVAL;
    spin->store(0, std::memory_order_release);
    return 0;
}

int APS5_VABI pthread_set_name_np_nid_postfix(Pthread thread, const char* name) {
    if (!thread || !name) return PosixThread::GUEST_EINVAL;
#ifdef _WIN32
    if (const auto setDescription = reinterpret_cast<decltype(&SetThreadDescription)>(
            GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetThreadDescription"))) {
        const int wideSize = MultiByteToWideChar(CP_UTF8, 0, name, -1, nullptr, 0);
        std::wstring wide(static_cast<std::size_t>(wideSize), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, name, -1, wide.data(), wideSize);
        setDescription(static_cast<HANDLE>(thread->nativeHandle), wide.c_str());
    }
#else
    (void)thread;
    (void)name;
#endif
    return 0;
}

int APS5_VABI pthread_kill_nid_postfix(Pthread thread, int /*sig*/) {
    return thread ? 0 : 3 /* ESRCH */;
}

int APS5_VABI pause_nid_postfix(void) {
    for (;;) {
#ifdef _WIN32
        Sleep(1000);
#else
        struct timespec request{1, 0};
        nanosleep(&request, nullptr);
#endif
    }
}

int APS5_VABI truncate_nid_postfix(const char* path, std::int64_t length) {
    if (!path || length < 0) {
        errno = 22;
        return -1;
    }
    try {
        const auto native = ResolvePath_nid_no_patch(path).wstring();
#ifdef _WIN32
        const HANDLE file = CreateFileW(native.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            errno = 2;
            return -1;
        }
        LARGE_INTEGER position{};
        position.QuadPart = length;
        if (!SetFilePointerEx(file, position, nullptr, FILE_BEGIN) || !SetEndOfFile(file)) {
            CloseHandle(file);
            errno = 5;
            return -1;
        }
        CloseHandle(file);
        return 0;
#else
        return ::truncate(path, static_cast<off_t>(length));
#endif
    } catch (...) {
        errno = 22;
        return -1;
    }
}

unsigned int APS5_VABI sceKernelGetSystemSwVersion_nid_postfix(void) {
    return 0x04512001u;
}

struct KernelItimerval;

int APS5_VABI setitimer_nid_postfix(int /*which*/, const KernelItimerval* /*newValue*/, KernelItimerval* /*oldValue*/) {
    // Interval timers that deliver signals are not provided; report success.
    return 0;
}

}
