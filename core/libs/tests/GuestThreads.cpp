#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <thread>

extern "C" {
int APS5_VABI _Mtx_init_nid_postfix(void**, int);
void APS5_VABI _Mtx_destroy_nid_postfix(void**);
int APS5_VABI _Mtx_lock_nid_postfix(void**);
int APS5_VABI _Mtx_unlock_nid_postfix(void**);
int APS5_VABI _Cnd_init_nid_postfix(void**);
void APS5_VABI _Cnd_destroy_nid_postfix(void**);
int APS5_VABI _Cnd_wait_nid_postfix(void**, void**);
int APS5_VABI _Cnd_broadcast_nid_postfix(void**);
}

static void Require(bool value) { if (!value) std::abort(); }

template <typename TCall>
static bool Throws(TCall call) {
    try { call(); }
    catch (const std::runtime_error&) { return true; }
    return false;
}

constexpr int Plain = 0x01, Try = 0x02, Timed = 0x04, Recursive = 0x100;

static void LifecycleWritesOneSlot() {
    void* sentinel = reinterpret_cast<void*>(0x5a5a5a5a);
    std::array<void*, 3> slots{sentinel, nullptr, sentinel};
    Require(_Mtx_init_nid_postfix(&slots[1], Try) == 0 && slots[1] != nullptr);
    Require(_Mtx_lock_nid_postfix(&slots[1]) == 0);
    Require(_Mtx_unlock_nid_postfix(&slots[1]) == 0);
    _Mtx_destroy_nid_postfix(&slots[1]);
    Require(slots[0] == sentinel && slots[1] == nullptr && slots[2] == sentinel);

    std::array<void*, 3> conditions{sentinel, nullptr, sentinel};
    Require(_Cnd_init_nid_postfix(&conditions[1]) == 0 && conditions[1] != nullptr);
    _Cnd_destroy_nid_postfix(&conditions[1]);
    Require(conditions[0] == sentinel && conditions[1] == nullptr && conditions[2] == sentinel);

    for (int type : {Plain, Plain | Try, Timed | Try, Recursive | Try, Recursive | Timed | Try}) {
        void* mutex = nullptr;
        Require(_Mtx_init_nid_postfix(&mutex, type) == 0);
        _Mtx_destroy_nid_postfix(&mutex);
    }
}

static void InvalidTypesThrow() {
    for (int type : {0, 0x08, 0x80, 0x200, 0x1000, Try | 0x10000}) {
        void* mutex = nullptr;
        Require(Throws([&] { _Mtx_init_nid_postfix(&mutex, type); }) && mutex == nullptr);
    }
    void* empty = nullptr;
    Require(Throws([&] { _Mtx_lock_nid_postfix(&empty); }));
    Require(Throws([&] { _Mtx_destroy_nid_postfix(&empty); }));
    Require(Throws([&] { _Cnd_destroy_nid_postfix(&empty); }));
}

static void RecursiveLocking() {
    void* mutex = nullptr;
    Require(_Mtx_init_nid_postfix(&mutex, Recursive | Try) == 0);
    Require(_Mtx_lock_nid_postfix(&mutex) == 0);
    Require(_Mtx_lock_nid_postfix(&mutex) == 0);
    Require(_Mtx_unlock_nid_postfix(&mutex) == 0);
    Require(_Mtx_unlock_nid_postfix(&mutex) == 0);
    Require(Throws([&] { _Mtx_unlock_nid_postfix(&mutex); }));
    std::thread([&] {
        Require(_Mtx_lock_nid_postfix(&mutex) == 0);
        Require(_Mtx_unlock_nid_postfix(&mutex) == 0);
    }).join();
    _Mtx_destroy_nid_postfix(&mutex);
}

static void OwnershipViolationsThrow() {
    void* mutex = nullptr;
    Require(_Mtx_init_nid_postfix(&mutex, Try) == 0);
    Require(_Mtx_lock_nid_postfix(&mutex) == 0);
    Require(Throws([&] { _Mtx_lock_nid_postfix(&mutex); }));
    bool foreignUnlockThrew = false;
    std::thread([&] { foreignUnlockThrew = Throws([&] { _Mtx_unlock_nid_postfix(&mutex); }); }).join();
    Require(foreignUnlockThrew);
    Require(Throws([&] { _Mtx_destroy_nid_postfix(&mutex); }));
    Require(_Mtx_unlock_nid_postfix(&mutex) == 0);
    _Mtx_destroy_nid_postfix(&mutex);
}

static void ContendedCounter() {
    void* mutex = nullptr;
    Require(_Mtx_init_nid_postfix(&mutex, Try) == 0);
    long counter = 0;
    auto work = [&] {
        for (int i = 0; i < 100000; ++i) {
            Require(_Mtx_lock_nid_postfix(&mutex) == 0);
            ++counter;
            Require(_Mtx_unlock_nid_postfix(&mutex) == 0);
        }
    };
    std::thread first(work), second(work);
    first.join();
    second.join();
    Require(counter == 200000);
    _Mtx_destroy_nid_postfix(&mutex);
}

static void BroadcastWakesWaiters() {
    void* mutex = nullptr;
    void* condition = nullptr;
    Require(_Mtx_init_nid_postfix(&mutex, Try) == 0);
    Require(_Cnd_init_nid_postfix(&condition) == 0);
    bool ready = false;
    std::atomic<int> waiting{0}, woken{0};
    std::array<std::thread, 4> waiters;
    for (auto& waiter : waiters) waiter = std::thread([&] {
        Require(_Mtx_lock_nid_postfix(&mutex) == 0);
        ++waiting;
        while (!ready) Require(_Cnd_wait_nid_postfix(&condition, &mutex) == 0);
        Require(Throws([&] { _Mtx_lock_nid_postfix(&mutex); }));
        ++woken;
        Require(_Mtx_unlock_nid_postfix(&mutex) == 0);
    });
    while (waiting.load() != 4) std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    Require(_Mtx_lock_nid_postfix(&mutex) == 0);
    ready = true;
    Require(_Cnd_broadcast_nid_postfix(&condition) == 0);
    Require(_Mtx_unlock_nid_postfix(&mutex) == 0);
    for (auto& waiter : waiters) waiter.join();
    Require(woken == 4);
    Require(_Mtx_lock_nid_postfix(&mutex) == 0);
    Require(_Mtx_unlock_nid_postfix(&mutex) == 0);
    _Cnd_destroy_nid_postfix(&condition);
    _Mtx_destroy_nid_postfix(&mutex);
}

static void WaitBlocksUntilBroadcast() {
    void* mutex = nullptr;
    void* condition = nullptr;
    Require(_Mtx_init_nid_postfix(&mutex, Try) == 0);
    Require(_Cnd_init_nid_postfix(&condition) == 0);
    std::atomic<bool> waiting{false}, returned{false};
    std::thread waiter([&] {
        Require(_Mtx_lock_nid_postfix(&mutex) == 0);
        waiting = true;
        Require(_Cnd_wait_nid_postfix(&condition, &mutex) == 0);
        returned = true;
        Require(_Mtx_unlock_nid_postfix(&mutex) == 0);
    });
    while (!waiting.load()) std::this_thread::yield();
    Require(_Mtx_lock_nid_postfix(&mutex) == 0);
    Require(_Mtx_unlock_nid_postfix(&mutex) == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    Require(!returned.load());
    Require(_Mtx_lock_nid_postfix(&mutex) == 0);
    Require(_Cnd_broadcast_nid_postfix(&condition) == 0);
    Require(_Mtx_unlock_nid_postfix(&mutex) == 0);
    waiter.join();
    Require(returned.load());
    _Cnd_destroy_nid_postfix(&condition);
    _Mtx_destroy_nid_postfix(&mutex);
}

static void RecursiveWaitThrows() {
    void* mutex = nullptr;
    void* condition = nullptr;
    Require(_Mtx_init_nid_postfix(&mutex, Recursive | Try) == 0);
    Require(_Cnd_init_nid_postfix(&condition) == 0);
    Require(_Mtx_lock_nid_postfix(&mutex) == 0);
    Require(_Mtx_lock_nid_postfix(&mutex) == 0);
    Require(Throws([&] { _Cnd_wait_nid_postfix(&condition, &mutex); }));
    Require(_Mtx_unlock_nid_postfix(&mutex) == 0);
    Require(_Mtx_unlock_nid_postfix(&mutex) == 0);
    Require(Throws([&] { _Cnd_wait_nid_postfix(&condition, &mutex); }));
    _Cnd_destroy_nid_postfix(&condition);
    _Mtx_destroy_nid_postfix(&mutex);
}

int main() {
    LifecycleWritesOneSlot();
    InvalidTypesThrow();
    RecursiveLocking();
    OwnershipViolationsThrow();
    ContendedCounter();
    BroadcastWakesWaiters();
    WaitBlocksUntilBroadcast();
    RecursiveWaitThrows();
}
