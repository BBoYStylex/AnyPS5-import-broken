#include <runtime/RuntimeRegistry.hpp>
#include <runtime/MemoryBridge.hpp>
#include <runtime/OsSubsystemStubs.hpp>
#include <cstdlib>
#include <chrono>
#include <thread>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

namespace Aot::Runtime {

static MemoryBridge gMemoryBridge;
static bool gInitialized = false;

bool RuntimeRegistry::Initialize() {
    if (gInitialized) {
        return true;
    }
    gInitialized = gMemoryBridge.Initialize();
    return gInitialized;
}

void RuntimeRegistry::Shutdown() {
    if (!gInitialized) {
        return;
    }
    gMemoryBridge.Shutdown();
    gInitialized = false;
}

bool RuntimeRegistry::IsInitialized() {
    return gInitialized;
}

} // namespace Aot::Runtime

extern "C" {

void aps5_sys_exit(int32_t exitCode) {
    std::exit(exitCode);
}

int64_t aps5_sys_read(int32_t fileDescriptor, void* buffer, size_t count) {
    return Aot::Runtime::OsSubsystemStubs::FileRead(fileDescriptor, buffer, count);
}

int64_t aps5_sys_write(int32_t fileDescriptor, const void* buffer, size_t count) {
    return Aot::Runtime::OsSubsystemStubs::FileWrite(fileDescriptor, buffer, count);
}

int32_t aps5_sys_open(const char* path, int32_t flags, int32_t mode) {
    return Aot::Runtime::OsSubsystemStubs::FileOpen(path, flags, mode);
}

int32_t aps5_sys_close(int32_t fileDescriptor) {
    return Aot::Runtime::OsSubsystemStubs::FileClose(fileDescriptor);
}

uint64_t aps5_sys_mmap(uint64_t address, size_t length, int32_t protection, int32_t flags, int32_t fileDescriptor, int64_t offset) {
    static_cast<void>(address);
    static_cast<void>(protection);
    static_cast<void>(flags);
    static_cast<void>(fileDescriptor);
    static_cast<void>(offset);

    if (!Aot::Runtime::gInitialized) {
        Aot::Runtime::RuntimeRegistry::Initialize();
    }
    return Aot::Runtime::gMemoryBridge.Allocate(length, Aot::Runtime::MemoryPoolType::HostRam);
}

int32_t aps5_sys_munmap(uint64_t address, size_t length) {
    static_cast<void>(length);
    if (!Aot::Runtime::gInitialized) {
        return -1;
    }
    return Aot::Runtime::gMemoryBridge.Deallocate(address) ? 0 : -1;
}

int32_t aps5_sys_thr_create(uint64_t* outThreadId, void* attributes, void* (*entryPoint)(void*), void* argument) {
    auto* attr = static_cast<const Aot::Runtime::ThreadCreationAttributes*>(attributes);
    return Aot::Runtime::OsSubsystemStubs::CreateGuestThread(outThreadId, attr, entryPoint, argument);
}

void aps5_sys_thr_exit(void* exitValue) {
    static_cast<void>(exitValue);
}

uint64_t aps5_sys_thr_self() {
#if defined(_WIN32)
    return static_cast<uint64_t>(GetCurrentThreadId());
#else
    return static_cast<uint64_t>(pthread_self());
#endif
}

int32_t aps5_sys_thr_kill(uint64_t threadId, int32_t signal) {
    static_cast<void>(threadId);
    static_cast<void>(signal);
    return 0;
}

int32_t aps5_sys_thr_wake(uint64_t threadId) {
    static_cast<void>(threadId);
    return 0;
}

int32_t aps5_sys_ioctl(int32_t fileDescriptor, uint64_t request, void* argument) {
    static_cast<void>(fileDescriptor);
    static_cast<void>(request);
    static_cast<void>(argument);
    return 0;
}

int32_t aps5_sys_mprotect(uint64_t address, size_t length, int32_t protection) {
    static_cast<void>(address);
    static_cast<void>(length);
    static_cast<void>(protection);
    return 0;
}

int32_t aps5_sys_regmgr_call(uint32_t category, uint32_t propertyId, void* outBuffer, size_t bufferSize, uint32_t type) {
    static_cast<void>(category);
    static_cast<void>(propertyId);
    static_cast<void>(type);
    if (outBuffer != nullptr && bufferSize >= sizeof(uint32_t)) {
        *static_cast<uint32_t*>(outBuffer) = 0;
    }
    return 0;
}

int32_t aps5_sys_jit_allocate(size_t size, int32_t flags, uint64_t* outAddress) {
    static_cast<void>(flags);
    if (!Aot::Runtime::gInitialized) {
        Aot::Runtime::RuntimeRegistry::Initialize();
    }
    uint64_t allocated = Aot::Runtime::gMemoryBridge.Allocate(size, Aot::Runtime::MemoryPoolType::HostRam, Aot::Runtime::MemoryBridge::StandardPageSize);
    if (outAddress != nullptr) {
        *outAddress = allocated;
    }
    return (allocated != 0) ? 0 : -1;
}

uint64_t aps5_sys_dmem_allocate(size_t size, uint64_t alignment, uint32_t memoryType, uint64_t* outAddress) {
    static_cast<void>(memoryType);
    if (!Aot::Runtime::gInitialized) {
        Aot::Runtime::RuntimeRegistry::Initialize();
    }
    size_t align = (alignment != 0) ? alignment : Aot::Runtime::MemoryBridge::StandardPageSize;
    uint64_t allocated = Aot::Runtime::gMemoryBridge.Allocate(size, Aot::Runtime::MemoryPoolType::SharedVisibleVram, align);
    if (outAddress != nullptr) {
        *outAddress = allocated;
    }
    return (allocated != 0) ? 0 : 0x8002000C;
}

int32_t aps5_sys_dmem_release(uint64_t address, size_t size) {
    static_cast<void>(size);
    if (!Aot::Runtime::gInitialized) {
        return -1;
    }
    return Aot::Runtime::gMemoryBridge.Deallocate(address) ? 0 : -1;
}

int32_t aps5_sys_named_event_create(const char* name, uint32_t attributes, uint64_t* outEventHandle) {
    return Aot::Runtime::OsSubsystemStubs::NamedEventCreate(name, attributes, outEventHandle);
}

int32_t aps5_sys_sched_yield() {
    std::this_thread::yield();
    return 0;
}

int32_t aps5_sys_nanosleep(const void* rqtp, void* rmtp) {
    static_cast<void>(rmtp);
    if (rqtp == nullptr) {
        return -1;
    }
    const auto* ts = static_cast<const uint64_t*>(rqtp);
    uint64_t sec = ts[0];
    uint64_t nsec = ts[1];
    std::this_thread::sleep_for(std::chrono::seconds(sec) + std::chrono::nanoseconds(nsec));
    return 0;
}

int32_t aps5_sys_getpid() {
#if defined(_WIN32)
    return static_cast<int32_t>(GetCurrentProcessId());
#else
    return static_cast<int32_t>(getpid());
#endif
}

int32_t aps5_sys_clock_gettime(int32_t clockId, void* timeSpec) {
    static_cast<void>(clockId);
    if (timeSpec == nullptr) {
        return -1;
    }
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - sec);
    auto* ts = static_cast<uint64_t*>(timeSpec);
    ts[0] = static_cast<uint64_t>(sec.count());
    ts[1] = static_cast<uint64_t>(nsec.count());
    return 0;
}

int64_t aps5_sys_pread(int32_t fileDescriptor, void* buffer, size_t count, int64_t offset) {
    return Aot::Runtime::OsSubsystemStubs::FilePread(fileDescriptor, buffer, count, offset);
}

int64_t aps5_sys_pwrite(int32_t fileDescriptor, const void* buffer, size_t count, int64_t offset) {
    return Aot::Runtime::OsSubsystemStubs::FilePwrite(fileDescriptor, buffer, count, offset);
}

int64_t aps5_sys_lseek(int32_t fileDescriptor, int64_t offset, int32_t whence) {
    return Aot::Runtime::OsSubsystemStubs::FileSeek(fileDescriptor, offset, whence);
}

int32_t aps5_sys_fstat(int32_t fileDescriptor, uint64_t* outSize, uint32_t* outMode) {
    return Aot::Runtime::OsSubsystemStubs::FileStat(fileDescriptor, outSize, outMode);
}

int32_t aps5_sys_ftruncate(int32_t fileDescriptor, int64_t length) {
    return Aot::Runtime::OsSubsystemStubs::FileTruncate(fileDescriptor, length);
}

int32_t aps5_sys_unlink(const char* path) {
    return Aot::Runtime::OsSubsystemStubs::FileUnlink(path);
}

int32_t aps5_sys_rename(const char* oldPath, const char* newPath) {
    return Aot::Runtime::OsSubsystemStubs::FileRename(oldPath, newPath);
}

int32_t aps5_sys_mkdir(const char* path, int32_t mode) {
    return Aot::Runtime::OsSubsystemStubs::FileMkdir(path, mode);
}

int32_t aps5_sys_named_event_open(const char* name, uint32_t flags, uint64_t* outEventHandle) {
    return Aot::Runtime::OsSubsystemStubs::NamedEventOpen(name, flags, outEventHandle);
}

int32_t aps5_sys_named_event_delete(const char* name) {
    return Aot::Runtime::OsSubsystemStubs::NamedEventDelete(name);
}

int32_t aps5_sys_named_event_set(uint64_t eventHandle) {
    return Aot::Runtime::OsSubsystemStubs::NamedEventSet(eventHandle);
}

int32_t aps5_sys_named_event_reset(uint64_t eventHandle) {
    return Aot::Runtime::OsSubsystemStubs::NamedEventReset(eventHandle);
}

int32_t aps5_sys_named_event_wait(uint64_t eventHandle, uint64_t timeoutMicroseconds) {
    return Aot::Runtime::OsSubsystemStubs::NamedEventWait(eventHandle, timeoutMicroseconds);
}

int32_t aps5_sys_named_event_close(uint64_t eventHandle) {
    return Aot::Runtime::OsSubsystemStubs::NamedEventClose(eventHandle);
}

}
