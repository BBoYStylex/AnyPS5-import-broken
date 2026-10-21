#include "Program.hpp"
#include "Thread.hpp"
#include "prx/libc/include/GuestArena.hpp"
#include "wasm-runtime/Guest.hpp"
#include "wasm-runtime/NidThunk.hpp"
#include <emscripten.h>
#include <emscripten/threading.h>
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <new>
#include <pthread.h>
#include <unordered_map>

namespace WasmRuntime {
namespace {

constexpr std::size_t DefaultStackSize = 2u << 20;
constexpr std::size_t StackAlignment = 0x10000;

constexpr double SignalPollMs = 50;

struct ThreadState {
    GuestCpu* cpu = nullptr;
    std::uint64_t stack = 0;
    std::size_t stackSize = 0;
    ThreadTls tls;
    pthread_t host{};
    std::atomic<void*> waitAddress{nullptr};
    std::atomic<unsigned> hostCalls{0};
    std::atomic<const char*> hostCall{nullptr};
};

thread_local ThreadState* t_thread = nullptr;
thread_local std::size_t t_stackSize = DefaultStackSize;
thread_local unsigned t_hostCalls = 0;
thread_local bool t_signalBlocked = false;
std::atomic<SignalDelivery> g_signalDelivery{nullptr};

std::mutex& RegistryLock() {
    static std::mutex lock;
    return lock;
}

std::unordered_map<pthread_t, ThreadState*>& Registry() {
    static std::unordered_map<pthread_t, ThreadState*> threads;
    return threads;
}

bool TraceSignals() {
    static const bool enabled = std::getenv("APS5_TRACE_SIGNALS") != nullptr;
    return enabled;
}

bool SignalPending(GuestCpu& cpu) {
    return std::atomic_ref(cpu.pendingSignal).load() != 0;
}

void DispatchPlaceholder(GuestCpu*) {
    GuestFault("the game module is not bound to this thread");
}

using DispatchFunction = void (*)(GuestCpu*);
DispatchFunction volatile g_dispatch = DispatchPlaceholder;

pthread_key_t g_threadKey;
pthread_once_t g_threadKeyOnce = PTHREAD_ONCE_INIT;

void ReleaseThread(void* value) {
    auto* state = static_cast<ThreadState*>(value);
    {
        std::lock_guard lock(RegistryLock());
        Registry().erase(state->host);
    }
    GuestArena::GuestArenaRelease_nid_postfix(reinterpret_cast<void*>(state->stack), state->stackSize);
    GuestArena::GuestArenaRelease_nid_postfix(reinterpret_cast<void*>(state->tls.block), state->tls.bytes);
    ::operator delete(state->cpu, std::align_val_t{alignof(GuestCpu)});
    delete state;
}

void CreateThreadKey() {
    pthread_key_create(&g_threadKey, ReleaseThread);
}

EM_JS(void, aps5_bind_game, (void* slot, const std::uint64_t* bases, std::uint32_t count, std::uint64_t stubBase, std::uint64_t addrMap), {
    if (!globalThis.aps5GameInstance) {
        const module = globalThis.aps5GameModule;
        if (!module) throw new Error('the game module did not reach this worker');
        const memory = wasmMemory;
        const view = new BigUint64Array(memory.buffer, Number(bases), count);
        const constant = (value) => new WebAssembly.Global({value: 'i64', mutable: false}, BigInt(value));
        const imports = {env: {memory}, nid: {}, aps5: {}};
        for (const entry of WebAssembly.Module.imports(module)) {
            if (entry.module === 'env' && entry.name.startsWith('__aps5_image_base_')) {
                imports.env[entry.name] = constant(view[Number(entry.name.slice(18))]);
            } else if (entry.module === 'env' && entry.name === '__aps5_stub_base') {
                imports.env[entry.name] = constant(stubBase);
            } else if (entry.module === 'env' && entry.name === '__aps5_addr_map') {
                imports.env[entry.name] = constant(addrMap);
            } else if (entry.module === 'nid') {
                const exported = wasmExports['nid.' + entry.name];
                imports.nid[entry.name] = exported || ((ctx) => {
                    const rsp = new BigUint64Array(memory.buffer, Number(ctx) + 32, 1)[0];
                    const caller = new BigUint64Array(memory.buffer, Number(rsp), 1)[0];
                    abort('the runtime does not provide NID ' + entry.name + ' (called from 0x' + caller.toString(16) + ')');
                });
            } else if (entry.module === 'aps5') {
                const exported = wasmExports['aps5.' + entry.name];
                if (!exported) throw new Error('the runtime does not provide aps5.' + entry.name);
                imports.aps5[entry.name] = exported;
            }
        }
        const instance = new WebAssembly.Instance(module, imports);
        if (instance.exports.__aps5_abi_version() !== 1) throw new Error('the game module uses another ABI version');
        globalThis.aps5GameInstance = instance;
    }
    wasmTable.set(slot, globalThis.aps5GameInstance.exports.__aps5_dispatch);
});

ThreadState* CreateThreadState() {
    pthread_once(&g_threadKeyOnce, CreateThreadKey);
    const auto& program = CurrentProgram();
    aps5_bind_game(reinterpret_cast<void*>(DispatchPlaceholder), program.bases.data(), static_cast<std::uint32_t>(program.bases.size()), program.stubBase, program.addrMap);
    auto* state = new ThreadState;
    state->cpu = new (std::align_val_t{alignof(GuestCpu)}) GuestCpu{};
    state->stackSize = (t_stackSize + StackAlignment - 1) & ~(StackAlignment - 1);
    state->stack = reinterpret_cast<std::uintptr_t>(GuestArena::GuestArenaAllocate_nid_postfix(state->stackSize, StackAlignment));
    state->tls = CreateThreadTls(program);
    auto& cpu = *state->cpu;
    cpu.gpr[Rsp] = state->stack + state->stackSize - 64;
    cpu.fsBase = state->tls.threadPointer;
    cpu.mxcsr = InitialMxcsr;
    cpu.x87Fcw = InitialFcw;
    state->host = pthread_self();
    pthread_setspecific(g_threadKey, state);
    std::lock_guard lock(RegistryLock());
    Registry()[state->host] = state;
    return state;
}

void DeliverWhileWaiting(GuestCpu& cpu) {
    const GuestCpu before = cpu;
    std::uint64_t rip = 0;
    std::memcpy(&rip, reinterpret_cast<const void*>(static_cast<std::uintptr_t>(cpu.gpr[Rsp])), sizeof(rip));
    const std::uint64_t interruptedRip = rip;
    std::uint64_t rsp = cpu.gpr[Rsp] + 8;
    DeliverPendingSignals(cpu, rip, rsp);
    if (rip != interruptedRip || rsp != before.gpr[Rsp] + 8 || std::memcmp(cpu.gpr, before.gpr, sizeof(cpu.gpr)) != 0)
        GuestFault("a signal handler changed the context of a thread waiting in a runtime call");
}

}  // namespace

void SetSignalDelivery(SignalDelivery delivery) {
    g_signalDelivery.store(delivery);
}

bool RaiseSignal(pthread_t thread) {
    std::lock_guard lock(RegistryLock());
    const auto found = Registry().find(thread);
    if (found == Registry().end()) {
        if (TraceSignals()) std::fprintf(stderr, "[signal] %lx raises on %lx, which has no guest context\n", static_cast<unsigned long>(pthread_self()), static_cast<unsigned long>(thread));
        return false;
    }
    ThreadState& state = *found->second;
    if (TraceSignals()) {
        std::uint64_t caller = 0;
        std::memcpy(&caller, reinterpret_cast<const void*>(static_cast<std::uintptr_t>(state.cpu->gpr[Rsp])), sizeof(caller));
        const char* call = state.hostCall.load();
        std::fprintf(stderr, "[signal] %lx raises on %lx: %u runtime calls deep (last %s), waiting on %p, [rsp] %llx\n", static_cast<unsigned long>(pthread_self()), static_cast<unsigned long>(thread),
            state.hostCalls.load(), call ? call : "none", state.waitAddress.load(), static_cast<unsigned long long>(caller));
    }
    std::atomic_ref(state.cpu->pendingSignal).store(1);
    if (void* address = state.waitAddress.load()) emscripten_futex_wake(address, INT_MAX);
    return true;
}

void DeliverPendingSignals(GuestCpu& cpu, std::uint64_t& rip, std::uint64_t& rsp) {
    const SignalDelivery delivery = g_signalDelivery.load();
    if (delivery == nullptr || t_signalBlocked) return;
    while (std::atomic_ref(cpu.pendingSignal).exchange(0) != 0) {
        t_signalBlocked = true;
        struct Unblock {
            ~Unblock() { t_signalBlocked = false; }
        } unblock;
        if (TraceSignals()) std::fprintf(stderr, "[signal] %lx runs the handler at %llx%s\n", static_cast<unsigned long>(pthread_self()), static_cast<unsigned long long>(rip), t_hostCalls != 0 ? " in a runtime call" : "");
        delivery(cpu, rip, rsp);
        if (TraceSignals()) std::fprintf(stderr, "[signal] %lx returns from the handler\n", static_cast<unsigned long>(pthread_self()));
    }
}

namespace Thunk {

HostCall::HostCall(const char* name) {
    ++t_hostCalls;
    if (t_thread) {
        t_thread->hostCalls.fetch_add(1, std::memory_order_relaxed);
        t_thread->hostCall.store(name, std::memory_order_relaxed);
    }
}

HostCall::~HostCall() {
    --t_hostCalls;
    if (t_thread) t_thread->hostCalls.fetch_sub(1, std::memory_order_relaxed);
}

}  // namespace Thunk

GuestCpu& CurrentCpu() {
    if (!t_thread) t_thread = CreateThreadState();
    return *t_thread->cpu;
}

void SetNextThreadStackSize(std::size_t bytes) {
    if (bytes != 0) t_stackSize = bytes;
}

void CurrentGuestStack(std::uint64_t& low, std::uint64_t& high) {
    CurrentCpu();
    low = t_thread->stack;
    high = t_thread->stack + t_thread->stackSize;
}

void Dispatch(GuestCpu& cpu) {
    g_dispatch(&cpu);
}

std::uint64_t ThreadPointer() {
    CurrentCpu();
    return t_thread->tls.threadPointer;
}

}  // namespace WasmRuntime

extern "C" int __real_emscripten_futex_wait(volatile void* address, std::uint32_t value, double maxWaitMs);

extern "C" int __wrap_emscripten_futex_wait(volatile void* address, std::uint32_t value, double maxWaitMs) {
    using namespace WasmRuntime;
    ThreadState* state = t_thread;
    if (state == nullptr || t_hostCalls == 0 || t_signalBlocked)
        return __real_emscripten_futex_wait(address, value, maxWaitMs);
    const double deadline = emscripten_get_now() + maxWaitMs;
    while (true) {
        state->waitAddress.store(const_cast<void*>(address));
        int result = -ETIMEDOUT;
        if (!SignalPending(*state->cpu)) result = __real_emscripten_futex_wait(address, value, std::clamp(deadline - emscripten_get_now(), 0.0, SignalPollMs));
        state->waitAddress.store(nullptr);
        if (SignalPending(*state->cpu)) {
            DeliverWhileWaiting(*state->cpu);
            return 0;
        }
        if (result != -ETIMEDOUT || emscripten_get_now() >= deadline) return result;
    }
}
