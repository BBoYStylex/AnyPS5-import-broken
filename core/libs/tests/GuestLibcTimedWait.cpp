#include "prx/libc/include/general/VabiMacros.hpp"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <thread>

extern "C" {
int APS5_VABI _Mtx_init_nid_postfix(void** handle, int type);
int APS5_VABI _Mtx_init_with_name_nid_postfix(void** handle, int type, const char* name);
void APS5_VABI _Mtx_destroy_nid_postfix(void** handle);
int APS5_VABI _Mtx_lock_nid_postfix(void** handle);
int APS5_VABI _Mtx_unlock_nid_postfix(void** handle);
int APS5_VABI _Cnd_init_nid_postfix(void** handle);
void APS5_VABI _Cnd_destroy_nid_postfix(void** handle);
int APS5_VABI _Cnd_broadcast_nid_postfix(void** handle);
int APS5_VABI _Cnd_timedwait_nid_postfix(void** condition, void** mutex, const void* target);
int APS5_VABI __cxa_thread_atexit_nid_postfix(void (APS5_VABI *func)(void*), void* arg, void* dso);
}

static void Require(bool value) { if (!value) std::abort(); }

template<typename TException, typename TCall>
static void RequireThrows(TCall call) {
    try {
        call();
    } catch (const TException&) {
        return;
    }
    std::abort();
}

constexpr int success = 0;
constexpr int tryable = 0x02;

static void MutexInitWithName() {
    void* mutex = nullptr;
    Require(_Mtx_init_with_name_nid_postfix(&mutex, tryable, "TestMutex") == success && mutex != nullptr);
    Require(_Mtx_lock_nid_postfix(&mutex) == success);
    Require(_Mtx_unlock_nid_postfix(&mutex) == success);
    _Mtx_destroy_nid_postfix(&mutex);
}

static void ConditionTimedwait() {
    struct Xtime {
        std::int64_t sec;
        std::int64_t nsec;
    };
    constexpr int timedout = 2;
    void* condition = nullptr;
    void* mutex = nullptr;
    Require(_Mtx_init_nid_postfix(&mutex, tryable) == success && mutex != nullptr);
    Require(_Cnd_init_nid_postfix(&condition) == success && condition != nullptr);
    Require(_Mtx_lock_nid_postfix(&mutex) == success);

    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(now).count();
    const auto nowNsec = std::chrono::duration_cast<std::chrono::nanoseconds>(now % std::chrono::seconds(1)).count();

    Xtime pastTime{nowSec - 1, nowNsec};
    Require(_Cnd_timedwait_nid_postfix(&condition, &mutex, &pastTime) == timedout);

    Xtime futureTime{nowSec + 5, nowNsec};
    bool ready = false;
    std::thread signaler([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        Require(_Mtx_lock_nid_postfix(&mutex) == success);
        ready = true;
        Require(_Cnd_broadcast_nid_postfix(&condition) == success);
        Require(_Mtx_unlock_nid_postfix(&mutex) == success);
    });
    while (!ready) {
        Require(_Cnd_timedwait_nid_postfix(&condition, &mutex, &futureTime) == success);
    }
    signaler.join();

    Require(_Mtx_unlock_nid_postfix(&mutex) == success);
    RequireThrows<std::invalid_argument>([&] { _Cnd_timedwait_nid_postfix(&condition, &mutex, nullptr); });
    _Cnd_destroy_nid_postfix(&condition);
    _Mtx_destroy_nid_postfix(&mutex);
}

static void APS5_VABI TestThreadDestructor(void* arg) {
    *static_cast<int*>(arg) = 42;
}

static void CxaThreadAtexit() {
    int value = 0;
    std::thread worker([&] {
        Require(__cxa_thread_atexit_nid_postfix(TestThreadDestructor, &value, reinterpret_cast<void*>(0x1234)) == 0);
    });
    worker.join();
    Require(value == 42);
}

int main() {
    MutexInitWithName();
    ConditionTimedwait();
    CxaThreadAtexit();
}
