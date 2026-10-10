#include "prx/libc/include/general/VabiMacros.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

constexpr int THRD_SUCCESS = 0;
constexpr int MTX_PLAIN = 0x1;
constexpr int MTX_TRY = 0x2;
constexpr int MTX_TIMED = 0x4;
constexpr int MTX_RECURSIVE = 0x100;

class GuestMutex {
public:
    explicit GuestMutex(int type) : _recursive((type & MTX_RECURSIVE) != 0) {}

    void lock() {
        if (_owner == std::this_thread::get_id()) {
            if (!_recursive) throw std::runtime_error("_Mtx_lock: non-recursive mutex locked again by its owner");
            ++_count;
            return;
        }
        _mutex.lock();
        _count = 1;
        _owner = std::this_thread::get_id();
    }

    void unlock() {
        if (_owner != std::this_thread::get_id()) throw std::runtime_error("_Mtx_unlock: mutex not owned by the calling thread");
        if (--_count != 0) return;
        _owner = std::thread::id();
        _mutex.unlock();
    }

    void RequireSingleOwnership(const char* funcName) const {
        if (_owner != std::this_thread::get_id() || _count != 1)
            throw std::runtime_error(std::string(funcName) + ": mutex must be locked once by the calling thread");
    }

    bool IsLocked() const { return _owner != std::thread::id(); }

private:
    std::mutex _mutex;
    std::atomic<std::thread::id> _owner;
    int _count = 0;
    bool _recursive;
};

GuestMutex* _toMutex(void* handle, const char* funcName) {
    if (!handle) throw std::runtime_error(std::string(funcName) + ": null mutex");
    return static_cast<GuestMutex*>(handle);
}

std::condition_variable_any* _toCond(void* handle, const char* funcName) {
    if (!handle) throw std::runtime_error(std::string(funcName) + ": null condition variable");
    return static_cast<std::condition_variable_any*>(handle);
}

}

extern "C" {

int APS5_VABI _Mtx_init_nid_postfix(void** mtx, int type) {
    if (!mtx) throw std::runtime_error("_Mtx_init: null mutex");
    const int base = type & ~MTX_RECURSIVE;
    if (base != MTX_PLAIN && base != MTX_TRY && base != MTX_TIMED && base != (MTX_PLAIN | MTX_TIMED) && base != (MTX_TRY | MTX_TIMED))
        throw std::invalid_argument("_Mtx_init: unknown mutex type " + std::to_string(type));
    *mtx = new GuestMutex(type);
    return THRD_SUCCESS;
}

void APS5_VABI _Mtx_destroy_nid_postfix(void* mtx) {
    if (!mtx) return;
    auto* mutex = static_cast<GuestMutex*>(mtx);
    if (mutex->IsLocked()) throw std::runtime_error("_Mtx_destroy: mutex is locked");
    delete mutex;
}

int APS5_VABI _Mtx_lock_nid_postfix(void* mtx) {
    _toMutex(mtx, "_Mtx_lock")->lock();
    return THRD_SUCCESS;
}

int APS5_VABI _Mtx_unlock_nid_postfix(void* mtx) {
    _toMutex(mtx, "_Mtx_unlock")->unlock();
    return THRD_SUCCESS;
}

int APS5_VABI _Cnd_init_nid_postfix(void** cnd) {
    if (!cnd) throw std::runtime_error("_Cnd_init: null condition variable");
    *cnd = new std::condition_variable_any();
    return THRD_SUCCESS;
}

void APS5_VABI _Cnd_destroy_nid_postfix(void* cnd) {
    delete static_cast<std::condition_variable_any*>(cnd);
}

int APS5_VABI _Cnd_wait_nid_postfix(void* cnd, void* mtx) {
    auto* cond = _toCond(cnd, "_Cnd_wait");
    auto* mutex = _toMutex(mtx, "_Cnd_wait");
    mutex->RequireSingleOwnership("_Cnd_wait");
    cond->wait(*mutex);
    return THRD_SUCCESS;
}

int APS5_VABI _Cnd_signal_nid_postfix(void* cnd) {
    _toCond(cnd, "_Cnd_signal")->notify_one();
    return THRD_SUCCESS;
}

int APS5_VABI _Cnd_broadcast_nid_postfix(void* cnd) {
    _toCond(cnd, "_Cnd_broadcast")->notify_all();
    return THRD_SUCCESS;
}

}
