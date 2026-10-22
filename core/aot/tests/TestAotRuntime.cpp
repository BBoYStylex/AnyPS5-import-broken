#include <runtime/MemoryBridge.hpp>
#include <runtime/HostLinkerBridge.hpp>
#include <runtime/OsSubsystemStubs.hpp>
#include <runtime/RuntimeRegistry.hpp>
#include <cassert>
#include <atomic>

static void DummyExportFunction() {}

int main() {
    Aot::Runtime::MemoryBridge memory;
    bool initOk = memory.Initialize();
    assert(initOk);

    uint64_t addr = memory.Allocate(32768, Aot::Runtime::MemoryPoolType::HostRam);
    assert(addr != 0);
    assert(memory.IsValidGuestAddress(addr));

    void* hostPtr = memory.TranslateGuestToHost(addr);
    assert(hostPtr != nullptr);

    uint64_t backToGuest = memory.TranslateHostToGuest(hostPtr);
    assert(backToGuest == addr);

    Aot::Runtime::MemoryBlock block{};
    bool queryOk = memory.QueryMemoryBlock(addr, block);
    assert(queryOk);
    assert(block.GuestVirtualAddress == addr);
    assert(block.PoolType == Aot::Runtime::MemoryPoolType::HostRam);
    assert(memory.GetTotalCommittedBytes() >= 32768);
    assert(memory.GetCommittedBytesByPool(Aot::Runtime::MemoryPoolType::HostRam) >= 32768);

    bool freeOk = memory.Deallocate(addr);
    assert(freeOk);

    memory.Shutdown();

    auto& linker = Aot::Runtime::HostLinkerBridge::Instance();
    linker.RegisterExport("libkernel", "sceKernelExitProcess", DummyExportFunction);
    auto resolvedFunc = linker.ResolveSymbol("libkernel", "sceKernelExitProcess");
    assert(resolvedFunc == DummyExportFunction);

    linker.RegisterNid("libkernel", "AC1FtjqMCL0", DummyExportFunction);
    auto resolvedNid = linker.ResolveNid("libkernel", "AC1FtjqMCL0");
    assert(resolvedNid == DummyExportFunction);

    std::atomic<int64_t> testAtomic{10};
    int64_t oldVal = Aot::Runtime::OsSubsystemStubs::AtomicAdd64(&testAtomic, 5);
    assert(oldVal == 10);
    assert(testAtomic.load() == 15);

    int64_t expected = 15;
    bool casOk = Aot::Runtime::OsSubsystemStubs::AtomicCompareExchange64(&testAtomic, &expected, 42);
    assert(casOk);
    assert(testAtomic.load() == 42);

    uint64_t dmemAddr = 0;
    uint64_t dmemRes = aps5_sys_dmem_allocate(65536, 16384, 0, &dmemAddr);
    assert(dmemRes == 0);
    assert(dmemAddr != 0);

    int32_t relRes = aps5_sys_dmem_release(dmemAddr, 65536);
    assert(relRes == 0);

    uint64_t jitAddr = 0;
    int32_t jitRes = aps5_sys_jit_allocate(65536, 0, &jitAddr);
    assert(jitRes == 0);
    assert(jitAddr != 0);

    int32_t jitFreeRes = aps5_sys_munmap(jitAddr, 65536);
    assert(jitFreeRes == 0);

    uint64_t selfTid = aps5_sys_thr_self();
    assert(selfTid != 0);

    uint32_t regVal = 0;
    int32_t regRes = aps5_sys_regmgr_call(1, 1, &regVal, sizeof(regVal), 0);
    assert(regRes == 0);

    int32_t pid = aps5_sys_getpid();
    assert(pid > 0);

    int32_t yieldRes = aps5_sys_sched_yield();
    assert(yieldRes == 0);

    uint64_t ts[2] = {0, 1000};
    int32_t sleepRes = aps5_sys_nanosleep(ts, nullptr);
    assert(sleepRes == 0);

    uint64_t clockTs[2] = {0, 0};
    int32_t clockRes = aps5_sys_clock_gettime(0, clockTs);
    assert(clockRes == 0);
    assert(clockTs[0] > 0);

    return 0;
}
