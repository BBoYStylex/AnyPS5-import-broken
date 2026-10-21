#ifndef CORE_AOT_RUNTIME_RUNTIMEREGISTRY_HPP
#define CORE_AOT_RUNTIME_RUNTIMEREGISTRY_HPP

#include <cstdint>
#include <cstddef>

namespace Aot::Runtime {

class RuntimeRegistry {
public:
    static bool Initialize();
    static void Shutdown();
    static bool IsInitialized();
};

} // namespace Aot::Runtime

extern "C" {
void aps5_sys_exit(int32_t exitCode);
int64_t aps5_sys_read(int32_t fileDescriptor, void* buffer, size_t count);
int64_t aps5_sys_write(int32_t fileDescriptor, const void* buffer, size_t count);
int32_t aps5_sys_open(const char* path, int32_t flags, int32_t mode);
int32_t aps5_sys_close(int32_t fileDescriptor);
uint64_t aps5_sys_mmap(uint64_t address, size_t length, int32_t protection, int32_t flags, int32_t fileDescriptor, int64_t offset);
int32_t aps5_sys_munmap(uint64_t address, size_t length);
int32_t aps5_sys_thr_create(uint64_t* outThreadId, void* attributes, void* (*entryPoint)(void*), void* argument);
void aps5_sys_thr_exit(void* exitValue);
}

#endif
