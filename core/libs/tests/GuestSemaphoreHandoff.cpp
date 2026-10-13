#include "SceTypes.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <thread>

extern "C" {
int APS5_VABI scePthreadCreate(Pthread* thread, const PthreadAttr* attr, PthreadEntry entry, void* arg, const char* name);
int APS5_VABI scePthreadJoin(Pthread thread, void** retval);
int APS5_VABI sceKernelCreateSema(KernelSema* sem, const char* name, uint32_t attr, int init, int max, void* opt);
int APS5_VABI sceKernelDeleteSema(KernelSema sem);
int APS5_VABI sceKernelPollSema(KernelSema sem, int need);
int APS5_VABI sceKernelSignalSema(KernelSema sem, int count);
int APS5_VABI sceKernelWaitSema(KernelSema sem, int need, KernelUseconds* time);
}

static constexpr int SCE_OK = 0;
static constexpr int SCE_KERNEL_ERROR_EBUSY = static_cast<int>(0x80020010);

static void Require(bool value) { if (!value) std::abort(); }

struct Waiter {
    KernelSema sem = nullptr;
    int result = -1;
    std::atomic<bool> started{false};
};

static void* APS5_VABI Wait(void* arg) {
    auto& waiter = *static_cast<Waiter*>(arg);
    KernelUseconds timeout = 1000000;
    waiter.started = true;
    waiter.result = sceKernelWaitSema(waiter.sem, 1, &timeout);
    return nullptr;
}

int main() {
    for (int i = 0; i < 20; ++i) {
        KernelSema sem = nullptr;
        Require(sceKernelCreateSema(&sem, "handoff", 1, 0, 1, nullptr) == SCE_OK);
        Waiter waiter{sem};
        Pthread thread = nullptr;
        Require(scePthreadCreate(&thread, nullptr, Wait, &waiter, nullptr) == SCE_OK);
        while (!waiter.started) std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        Require(sceKernelSignalSema(sem, 1) == SCE_OK);
        Require(sceKernelPollSema(sem, 1) == SCE_KERNEL_ERROR_EBUSY);
        Require(scePthreadJoin(thread, nullptr) == SCE_OK);
        Require(waiter.result == SCE_OK);
        Require(sceKernelPollSema(sem, 1) == SCE_KERNEL_ERROR_EBUSY);
        Require(sceKernelDeleteSema(sem) == SCE_OK);
    }
    return 0;
}
