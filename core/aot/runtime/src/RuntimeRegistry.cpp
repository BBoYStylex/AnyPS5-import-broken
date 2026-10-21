#include <runtime/RuntimeRegistry.hpp>
#include <runtime/MemoryBridge.hpp>
#include <runtime/OsSubsystemStubs.hpp>
#include <cstdlib>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#else
#include <pthread.h>
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
    static_cast<void>(name);
    static_cast<void>(attributes);
    static uint64_t nextEventId = 1000;
    if (outEventHandle != nullptr) {
        *outEventHandle = ++nextEventId;
    }
    return 0;
}

}
