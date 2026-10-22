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

    bool protOk = memory.Protect(addr, 16384, Aot::Runtime::MemoryProtection::Read);
    assert(protOk);
    queryOk = memory.QueryMemoryBlock(addr, block);
    assert(queryOk);
    assert(block.Protection == Aot::Runtime::MemoryProtection::Read);

    int32_t sysProtRes = aps5_sys_mprotect(addr, 16384, 3);
    assert(sysProtRes == 0);
    queryOk = memory.QueryMemoryBlock(addr, block);
    assert(queryOk);
    assert(block.Protection == Aot::Runtime::MemoryProtection::ReadWrite);

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

    uint64_t monoUs = Aot::Runtime::OsSubsystemStubs::GetMonotonicClockMicroseconds();
    assert(monoUs > 0);

    const char* testFileName = "test_aot_runtime_tmp.bin";
    const char* testFileNameRenamed = "test_aot_runtime_tmp_renamed.bin";
    int32_t fd = aps5_sys_open(testFileName, 0x0102, 0666);
    if (fd < 0) {
        fd = aps5_sys_open(testFileName, 2, 0666);
    }
    if (fd >= 0) {
        const char msg[] = "AnyPS5_AOT_Runtime_Test_Data";
        int64_t written = aps5_sys_write(fd, msg, sizeof(msg));
        assert(written == static_cast<int64_t>(sizeof(msg)));

        uint64_t fileSize = 0;
        uint32_t fileMode = 0;
        int32_t statRes = aps5_sys_fstat(fd, &fileSize, &fileMode);
        assert(statRes == 0);
        assert(fileSize == sizeof(msg));

        char pbuf[8] = {};
        int64_t preadRes = aps5_sys_pread(fd, pbuf, 6, 0);
        assert(preadRes == 6);
        assert(pbuf[0] == 'A' && pbuf[5] == '5');

        int32_t truncRes = aps5_sys_ftruncate(fd, 6);
        assert(truncRes == 0);

        statRes = aps5_sys_fstat(fd, &fileSize, &fileMode);
        assert(statRes == 0);
        assert(fileSize == 6);

        aps5_sys_close(fd);
        aps5_sys_rename(testFileName, testFileNameRenamed);
        aps5_sys_unlink(testFileNameRenamed);
    }

    uint64_t evtHandle = 0;
    int32_t evtRes = aps5_sys_named_event_create("TestEventSync", 0, &evtHandle);
    assert(evtRes == 0);
    assert(evtHandle != 0);

    uint64_t openedEvtHandle = 0;
    int32_t openEvtRes = aps5_sys_named_event_open("TestEventSync", 0, &openedEvtHandle);
    assert(openEvtRes == 0);

    int32_t waitRes = aps5_sys_named_event_wait(evtHandle, 0);
    assert(waitRes == -1);

    int32_t setRes = aps5_sys_named_event_set(openedEvtHandle);
    assert(setRes == 0);

    waitRes = aps5_sys_named_event_wait(evtHandle, 10000);
    assert(waitRes == 0);

    waitRes = aps5_sys_named_event_wait(evtHandle, 0);
    assert(waitRes == -1);

    aps5_sys_named_event_close(openedEvtHandle);
    aps5_sys_named_event_close(evtHandle);
    aps5_sys_named_event_delete("TestEventSync");

    return 0;
}
