#include <runtime/RuntimeRegistry.hpp>
#include <runtime/MemoryBridge.hpp>
#include <runtime/OsSubsystemStubs.hpp>
#include <cstdlib>

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

}
