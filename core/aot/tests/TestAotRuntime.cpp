#include <runtime/MemoryBridge.hpp>
#include <runtime/HostLinkerBridge.hpp>
#include <runtime/OsSubsystemStubs.hpp>
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

    return 0;
}
