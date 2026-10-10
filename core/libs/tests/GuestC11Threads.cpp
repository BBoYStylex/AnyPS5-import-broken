#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdlib>
#include <stdexcept>
#include <thread>

extern "C" {
int APS5_VABI _Mtx_init_nid_postfix(void** mtx, int type);
void APS5_VABI _Mtx_destroy_nid_postfix(void* mtx);
int APS5_VABI _Mtx_lock_nid_postfix(void* mtx);
int APS5_VABI _Mtx_unlock_nid_postfix(void* mtx);
int APS5_VABI _Cnd_init_nid_postfix(void** cnd);
void APS5_VABI _Cnd_destroy_nid_postfix(void* cnd);
int APS5_VABI _Cnd_wait_nid_postfix(void* cnd, void* mtx);
int APS5_VABI _Cnd_signal_nid_postfix(void* cnd);
int APS5_VABI _Cnd_broadcast_nid_postfix(void* cnd);
}

static constexpr int MTX_PLAIN = 0x1;
static constexpr int MTX_RECURSIVE = 0x100;

static void Require(bool value) { if (!value) std::abort(); }

template <typename TFunc>
static bool Throws(TFunc func) {
    try {
        func();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

static void TestRecursive() {
    void* mtx = nullptr;
    Require(_Mtx_init_nid_postfix(&mtx, MTX_PLAIN | MTX_RECURSIVE) == 0);
    Require(_Mtx_lock_nid_postfix(mtx) == 0);
    Require(_Mtx_lock_nid_postfix(mtx) == 0);
    Require(_Mtx_unlock_nid_postfix(mtx) == 0);
    Require(_Mtx_unlock_nid_postfix(mtx) == 0);
    Require(Throws([&] { _Mtx_unlock_nid_postfix(mtx); }));
    _Mtx_destroy_nid_postfix(mtx);
}

static void TestPlainRelock() {
    void* mtx = nullptr;
    Require(_Mtx_init_nid_postfix(&mtx, MTX_PLAIN) == 0);
    Require(_Mtx_lock_nid_postfix(mtx) == 0);
    Require(Throws([&] { _Mtx_lock_nid_postfix(mtx); }));
    Require(Throws([&] { _Mtx_destroy_nid_postfix(mtx); }));
    Require(_Mtx_unlock_nid_postfix(mtx) == 0);
    _Mtx_destroy_nid_postfix(mtx);
    Require(Throws([&] { _Mtx_init_nid_postfix(&mtx, 0x8); }));
}

static void TestWait(int (APS5_VABI *wake)(void*)) {
    void* mtx = nullptr;
    void* cnd = nullptr;
    Require(_Mtx_init_nid_postfix(&mtx, MTX_PLAIN) == 0);
    Require(_Cnd_init_nid_postfix(&cnd) == 0);
    int ready = 0;
    constexpr int WAITERS = 4;
    int woken = 0;
    std::thread waiters[WAITERS];
    for (auto& waiter : waiters) {
        waiter = std::thread([&] {
            Require(_Mtx_lock_nid_postfix(mtx) == 0);
            while (!ready) Require(_Cnd_wait_nid_postfix(cnd, mtx) == 0);
            ++woken;
            Require(_Mtx_unlock_nid_postfix(mtx) == 0);
        });
    }
    Require(_Mtx_lock_nid_postfix(mtx) == 0);
    ready = 1;
    Require(_Mtx_unlock_nid_postfix(mtx) == 0);
    for (int i = 0; i < WAITERS; ++i) Require(wake(cnd) == 0);
    for (auto& waiter : waiters) waiter.join();
    Require(woken == WAITERS);
    _Cnd_destroy_nid_postfix(cnd);
    _Mtx_destroy_nid_postfix(mtx);
}

static void TestWaitNeedsSingleLock() {
    void* mtx = nullptr;
    void* cnd = nullptr;
    Require(_Mtx_init_nid_postfix(&mtx, MTX_PLAIN | MTX_RECURSIVE) == 0);
    Require(_Cnd_init_nid_postfix(&cnd) == 0);
    Require(Throws([&] { _Cnd_wait_nid_postfix(cnd, mtx); }));
    Require(_Mtx_lock_nid_postfix(mtx) == 0);
    Require(_Mtx_lock_nid_postfix(mtx) == 0);
    Require(Throws([&] { _Cnd_wait_nid_postfix(cnd, mtx); }));
    Require(_Mtx_unlock_nid_postfix(mtx) == 0);
    Require(_Mtx_unlock_nid_postfix(mtx) == 0);
    _Cnd_destroy_nid_postfix(cnd);
    _Mtx_destroy_nid_postfix(mtx);
}

int main() {
    TestRecursive();
    TestPlainRelock();
    TestWait(_Cnd_signal_nid_postfix);
    TestWait(_Cnd_broadcast_nid_postfix);
    TestWaitNeedsSingleLock();
    return 0;
}
