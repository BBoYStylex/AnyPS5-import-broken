#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include "prx/libc/include/PreciseWait.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

namespace {

constexpr int MutexTypeMask = 0x107;
constexpr int MutexRecursive = 0x100;
constexpr int ThreadSuccess = 0;

class GuestMutex {
public:
    explicit GuestMutex(bool recursive) : recursive(recursive) {}

    void Lock() {
        const auto self = std::this_thread::get_id();
        if (owner.load() == self) {
            if (!recursive) throw std::runtime_error("libc: non-recursive _Mtx_t locked again by its owner");
            ++count;
            return;
        }
#ifdef _WIN32
        while (!mutex.try_lock()) PreciseSleepUs(1000);
#else
        mutex.lock();
#endif
        owner.store(self);
        count = 1;
    }

    void Unlock() {
        requireOwner();
        if (--count != 0) return;
        owner.store(std::thread::id{});
        mutex.unlock();
    }

    void RequireSingleLock() const {
        requireOwner();
        if (count != 1) throw std::runtime_error("libc: _Cnd_wait with a recursive _Mtx_t locked more than once");
    }

    bool Locked() const {
        return owner.load() != std::thread::id{};
    }

private:
    void requireOwner() const {
        if (owner.load() != std::this_thread::get_id()) throw std::runtime_error("libc: _Mtx_t used by a thread that does not own it");
    }

    std::mutex mutex;
    std::atomic<std::thread::id> owner;
    std::uint32_t count = 0;
    const bool recursive;
};

class WaitLock {
public:
    explicit WaitLock(GuestMutex& mutex) : mutex(mutex) {}
    void lock() { mutex.Lock(); }
    void unlock() { mutex.Unlock(); }

private:
    GuestMutex& mutex;
};

class GuestCondition {
public:
    void Wait(GuestMutex& mutex) {
        const auto observed = generation.load();
        WaitLock lock(mutex);
#ifdef _WIN32
        while (generation.load() == observed) condition.wait_for(lock, std::chrono::milliseconds(1));
#else
        while (generation.load() == observed) condition.wait(lock);
#endif
    }

    void Broadcast() {
        ++generation;
        condition.notify_all();
    }

private:
    std::condition_variable_any condition;
    std::atomic<std::uint64_t> generation{0};
};

template <typename TValue>
TValue& resolve(TValue** slot, const char* name) {
    if (slot == nullptr || *slot == nullptr) throw std::runtime_error(std::string("libc: uninitialized ") + name);
    return **slot;
}

}

extern "C" {

int APS5_VABI _Mtx_init_nid_postfix(GuestMutex** mtx, int type) {
    if (mtx == nullptr) throw std::runtime_error("libc: _Mtx_init with a null _Mtx_t");
    if (type == 0 || (type & ~MutexTypeMask) != 0) throw std::runtime_error("libc: unsupported _Mtx_init type " + std::to_string(type));
    *mtx = new GuestMutex((type & MutexRecursive) != 0);
    return ThreadSuccess;
}

void APS5_VABI _Mtx_destroy_nid_postfix(GuestMutex** mtx) {
    if (resolve(mtx, "_Mtx_t").Locked()) throw std::runtime_error("libc: _Mtx_destroy on a locked _Mtx_t");
    delete *mtx;
    *mtx = nullptr;
}

int APS5_VABI _Mtx_lock_nid_postfix(GuestMutex** mtx) {
    resolve(mtx, "_Mtx_t").Lock();
    return ThreadSuccess;
}

int APS5_VABI _Mtx_unlock_nid_postfix(GuestMutex** mtx) {
    resolve(mtx, "_Mtx_t").Unlock();
    return ThreadSuccess;
}

int APS5_VABI _Cnd_init_nid_postfix(GuestCondition** cnd) {
    if (cnd == nullptr) throw std::runtime_error("libc: _Cnd_init with a null _Cnd_t");
    *cnd = new GuestCondition();
    return ThreadSuccess;
}

void APS5_VABI _Cnd_destroy_nid_postfix(GuestCondition** cnd) {
    resolve(cnd, "_Cnd_t");
    delete *cnd;
    *cnd = nullptr;
}

int APS5_VABI _Cnd_wait_nid_postfix(GuestCondition** cnd, GuestMutex** mtx) {
    auto& condition = resolve(cnd, "_Cnd_t");
    auto& mutex = resolve(mtx, "_Mtx_t");
    mutex.RequireSingleLock();
    condition.Wait(mutex);
    return ThreadSuccess;
}

int APS5_VABI _Cnd_broadcast_nid_postfix(GuestCondition** cnd) {
    resolve(cnd, "_Cnd_t").Broadcast();
    return ThreadSuccess;
}

}
