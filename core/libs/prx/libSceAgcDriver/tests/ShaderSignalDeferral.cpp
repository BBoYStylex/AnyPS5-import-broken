#include "SceTypes.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Shaders/ShaderRegistry.hpp"
#include "prx/libSceAgcDriver/Execution/include/ShaderPreparationScope.hpp"
#include "prx/libSceAgcDriver/Execution/include/ShaderPreparation.hpp"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <source_location>
#include <memory>
#include <thread>

extern "C" {
int APS5_VABI scePthreadCreate(Pthread*, const PthreadAttr*, PthreadEntry, void*, const char*);
int APS5_VABI scePthreadJoin(Pthread, void**);
int APS5_VABI sceKernelInstallExceptionHandler(int, void*);
int APS5_VABI sceKernelRemoveExceptionHandler(int);
int APS5_VABI sceKernelRaiseException(Pthread, int);
Pthread APS5_VABI scePthreadSelf();
}

using Transaction = AgcDriver::DriverDetail::ShaderPreparationTransaction;
using Scope = std::unique_ptr<Transaction, decltype(&AgcDriverEndShaderPreparation_nid_postfix)>;
static void Require(bool value, std::source_location at = std::source_location::current()) {
    if (value) return;
    std::fprintf(stderr, "shader signal deferral assertion at line %u\n", at.line());
    std::abort();
}
static AgcDriver::DriverDetail::ShaderSnapshot snapshot{};
static std::atomic<int> startedCalls{0}, completedCalls{0};
static Scope Begin() { return {AgcDriverBeginShaderPreparation_nid_postfix(nullptr, 0), &AgcDriverEndShaderPreparation_nid_postfix}; }
static void APS5_VABI Handler(int signal, void*) {
    Require(signal == 30);
    startedCalls.fetch_add(1);
    auto transaction = Begin();
    transaction->Read(snapshot);
    transaction->Commit();
    transaction.reset();
    completedCalls.fetch_add(1);
}

struct Context {
    std::atomic<bool> ready{false}, release{false};
    bool hold = true;
    int expected = 1;
};
static void* APS5_VABI Worker(void* opaque) {
    auto& context = *static_cast<Context*>(opaque);
    {
        auto transaction = Begin();
        if (!context.hold) context.ready.store(true);
        transaction->Read(snapshot);
        if (context.hold) {
            context.ready.store(true);
            while (!context.release.load()) std::atomic_signal_fence(std::memory_order_seq_cst);
        }
        transaction->Commit();
    }
    while (completedCalls.load() < context.expected) std::this_thread::yield();
    return nullptr;
}

int main() {
    Require(sceKernelInstallExceptionHandler(30, reinterpret_cast<void*>(&Handler)) == 0);
    Context owner;
    Pthread worker = nullptr;
    Require(scePthreadCreate(&worker, nullptr, Worker, &owner, "shader owner") == 0);
    while (!owner.ready.load()) std::this_thread::yield();
    std::thread release([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        Require(startedCalls.load() == 0);
        owner.release.store(true);
    });
    Require(sceKernelRaiseException(worker, 30) == 0);
    release.join();
    Require(scePthreadJoin(worker, nullptr) == 0);
    Require(completedCalls.load() == 1);

    auto transaction = Begin();
    transaction->Read(snapshot);
    Context waiting;
    waiting.hold = false;
    waiting.expected = 2;
    Require(scePthreadCreate(&worker, nullptr, Worker, &waiting, "shader waiter") == 0);
    while (!waiting.ready.load()) std::this_thread::yield();
    std::thread raising([&] { Require(sceKernelRaiseException(worker, 30) == 0); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    Require(startedCalls.load() == 1);
    transaction->Commit();
    transaction.reset();
    raising.join();
    Require(scePthreadJoin(worker, nullptr) == 0);
    Require(completedCalls.load() == 2);
    Context adopted;
    std::atomic<Pthread> adoptedThread{nullptr};
    std::thread native([&] {
        {
            auto scope = Begin();
            scope->Read(snapshot);
            adoptedThread.store(scePthreadSelf());
            while (!adopted.release.load()) std::atomic_signal_fence(std::memory_order_seq_cst);
            scope->Commit();
        }
        while (completedCalls.load() < 3) std::this_thread::yield();
    });
    while (adoptedThread.load() == nullptr) std::this_thread::yield();
    std::thread releaseAdopted([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        Require(startedCalls.load() == 2);
        adopted.release.store(true);
    });
    Require(sceKernelRaiseException(adoptedThread.load(), 30) == 0);
    releaseAdopted.join();
    native.join();
    Require(completedCalls.load() == 3);
    Require(sceKernelRemoveExceptionHandler(30) == 0);
}
