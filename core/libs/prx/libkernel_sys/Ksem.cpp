#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <cerrno>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

extern "C" int* APS5_VABI __error_nid_postfix();
extern "C" int APS5_VABI close_nid_postfix(int descriptor);
extern "C" void RegisterDescriptorCloseObserver_nid_no_patch(void (*observer)(int descriptor));

namespace {

constexpr int GuestEnoent = 2;
constexpr int GuestEfault = 14;
constexpr int GuestEbusy = 16;
constexpr int GuestEexist = 17;
constexpr int GuestEinval = 22;
constexpr int GuestEnfile = 23;
constexpr int GuestEmfile = 24;
constexpr int GuestEagain = 35;
constexpr int GuestEtimedout = 60;
constexpr int GuestEnametoolong = 63;
constexpr int GuestEoverflow = 84;
constexpr int GuestOCreat = 0x0200;
constexpr int GuestOExcl = 0x0800;
constexpr std::size_t GuestPathMax = 1024;
constexpr unsigned SemValueMax = 0x7fffffffu;

struct GuestTimespec {
    std::int64_t seconds;
    std::int64_t nanoseconds;
};

struct Ksem {
    std::mutex lock;
    std::condition_variable available;
    unsigned count = 0;
    unsigned waiters = 0;
    bool named = false;
};

std::mutex g_ksemLock;
std::map<int, std::shared_ptr<Ksem>> g_ksems;
std::map<std::string, std::shared_ptr<Ksem>> g_named;

int Fail(int error) {
    *__error_nid_postfix() = error;
    return -1;
}

std::shared_ptr<Ksem> Find(std::intptr_t id) {
    if (id < 0 || id > INT_MAX) return {};
    std::lock_guard lock(g_ksemLock);
    const auto found = g_ksems.find(static_cast<int>(id));
    return found != g_ksems.end() ? found->second : std::shared_ptr<Ksem>{};
}

int ReserveDescriptor() {
#ifdef _WIN32
    return ::_open("NUL", _O_RDONLY | _O_BINARY);
#else
    return ::open("/dev/null", O_RDONLY | O_CLOEXEC);
#endif
}

void DescriptorClosed(int descriptor) {
    std::lock_guard lock(g_ksemLock);
    g_ksems.erase(descriptor);
}

const bool g_observerRegistered = [] {
    RegisterDescriptorCloseObserver_nid_no_patch(DescriptorClosed);
    return true;
}();

int Wait(std::intptr_t id, bool once, const GuestTimespec* abstime) {
    const auto ksem = Find(id);
    if (!ksem) return Fail(GuestEinval);
    std::unique_lock lock(ksem->lock);
    if (ksem->count == 0) {
        if (once) return Fail(GuestEagain);
        ++ksem->waiters;
        bool ready = true;
        if (abstime == nullptr) {
            ksem->available.wait(lock, [&] { return ksem->count > 0; });
        } else {
            const auto deadline = std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::system_clock::duration>(
                std::chrono::seconds(abstime->seconds) + std::chrono::nanoseconds(abstime->nanoseconds)));
            ready = ksem->available.wait_until(lock, deadline, [&] { return ksem->count > 0; });
        }
        --ksem->waiters;
        if (!ready) return Fail(GuestEtimedout);
    }
    --ksem->count;
    return 0;
}

}

extern "C" {

int APS5_VABI ksem_init_nid_postfix(std::intptr_t* idp, unsigned int value) {
    if (value > SemValueMax) return Fail(GuestEinval);
    if (idp == nullptr) return Fail(GuestEfault);
    const int descriptor = ReserveDescriptor();
    if (descriptor < 0) return Fail(errno == ENFILE ? GuestEnfile : GuestEmfile);
    auto ksem = std::make_shared<Ksem>();
    ksem->count = value;
    {
        std::lock_guard lock(g_ksemLock);
        g_ksems[descriptor] = std::move(ksem);
    }
    *idp = descriptor;
    return 0;
}

int APS5_VABI ksem_post_nid_postfix(std::intptr_t id) {
    const auto ksem = Find(id);
    if (!ksem) return Fail(GuestEinval);
    std::lock_guard lock(ksem->lock);
    if (ksem->count == SemValueMax) return Fail(GuestEoverflow);
    ++ksem->count;
    ksem->available.notify_one();
    return 0;
}

int APS5_VABI ksem_wait_nid_postfix(std::intptr_t id) {
    return Wait(id, false, nullptr);
}

int APS5_VABI ksem_trywait_nid_postfix(std::intptr_t id) {
    return Wait(id, true, nullptr);
}

int APS5_VABI ksem_timedwait_nid_postfix(std::intptr_t id, const GuestTimespec* abstime) {
    if (abstime != nullptr && (abstime->nanoseconds < 0 || abstime->nanoseconds >= 1000000000)) return Fail(GuestEinval);
    return Wait(id, false, abstime);
}

int APS5_VABI ksem_getvalue_nid_postfix(std::intptr_t id, int* value) {
    const auto ksem = Find(id);
    if (!ksem) return Fail(GuestEinval);
    if (value == nullptr) return Fail(GuestEfault);
    std::lock_guard lock(ksem->lock);
    *value = static_cast<int>(ksem->count);
    return 0;
}

int APS5_VABI ksem_destroy_nid_postfix(std::intptr_t id) {
    const auto ksem = Find(id);
    if (!ksem || ksem->named) return Fail(GuestEinval);
    {
        std::lock_guard lock(ksem->lock);
        if (ksem->waiters != 0) return Fail(GuestEbusy);
    }
    return close_nid_postfix(static_cast<int>(id));
}

int APS5_VABI ksem_open_nid_postfix(std::intptr_t* idp, const char* name, int oflag, unsigned int mode, unsigned int value) {
    (void)mode;
    if (value > SemValueMax) return Fail(GuestEinval);
    if (name == nullptr) return Fail(GuestEfault);
    const std::string path(name, strnlen(name, GuestPathMax));
    if (path.size() >= GuestPathMax) return Fail(GuestEnametoolong);
    if (path.empty() || path[0] != '/') return Fail(GuestEinval);
    if ((oflag & ~(GuestOCreat | GuestOExcl)) != 0) return Fail(GuestEinval);
    if (idp == nullptr) return Fail(GuestEfault);
    std::lock_guard lock(g_ksemLock);
    std::shared_ptr<Ksem> ksem;
    const auto found = g_named.find(path);
    if (found == g_named.end()) {
        if ((oflag & GuestOCreat) == 0) return Fail(GuestEnoent);
        ksem = std::make_shared<Ksem>();
        ksem->count = value;
        ksem->named = true;
    } else {
        if ((oflag & (GuestOCreat | GuestOExcl)) == (GuestOCreat | GuestOExcl)) return Fail(GuestEexist);
        ksem = found->second;
    }
    const int descriptor = ReserveDescriptor();
    if (descriptor < 0) return Fail(errno == ENFILE ? GuestEnfile : GuestEmfile);
    if (found == g_named.end()) g_named[path] = ksem;
    g_ksems[descriptor] = std::move(ksem);
    *idp = descriptor;
    return 0;
}

int APS5_VABI ksem_close_nid_postfix(std::intptr_t id) {
    const auto ksem = Find(id);
    if (!ksem || !ksem->named) return Fail(GuestEinval);
    return close_nid_postfix(static_cast<int>(id));
}

int APS5_VABI ksem_unlink_nid_postfix(const char* name) {
    if (name == nullptr) return Fail(GuestEfault);
    const std::string path(name, strnlen(name, GuestPathMax));
    if (path.size() >= GuestPathMax) return Fail(GuestEnametoolong);
    std::lock_guard lock(g_ksemLock);
    if (g_named.erase(path) == 0) return Fail(GuestEnoent);
    return 0;
}

}
