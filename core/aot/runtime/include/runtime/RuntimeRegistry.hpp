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
uint64_t aps5_sys_thr_self();
int32_t aps5_sys_thr_kill(uint64_t threadId, int32_t signal);
int32_t aps5_sys_thr_wake(uint64_t threadId);
int32_t aps5_sys_ioctl(int32_t fileDescriptor, uint64_t request, void* argument);
int32_t aps5_sys_mprotect(uint64_t address, size_t length, int32_t protection);
int32_t aps5_sys_regmgr_call(uint32_t category, uint32_t propertyId, void* outBuffer, size_t bufferSize, uint32_t type);
int32_t aps5_sys_jit_allocate(size_t size, int32_t flags, uint64_t* outAddress);
uint64_t aps5_sys_dmem_allocate(size_t size, uint64_t alignment, uint32_t memoryType, uint64_t* outAddress);
int32_t aps5_sys_dmem_release(uint64_t address, size_t size);
int32_t aps5_sys_named_event_create(const char* name, uint32_t attributes, uint64_t* outEventHandle);
int32_t aps5_sys_sched_yield();
int32_t aps5_sys_nanosleep(const void* rqtp, void* rmtp);
int32_t aps5_sys_getpid();
int32_t aps5_sys_clock_gettime(int32_t clockId, void* timeSpec);
int64_t aps5_sys_pread(int32_t fileDescriptor, void* buffer, size_t count, int64_t offset);
int64_t aps5_sys_pwrite(int32_t fileDescriptor, const void* buffer, size_t count, int64_t offset);
int64_t aps5_sys_lseek(int32_t fileDescriptor, int64_t offset, int32_t whence);
int32_t aps5_sys_fstat(int32_t fileDescriptor, uint64_t* outSize, uint32_t* outMode);
int32_t aps5_sys_ftruncate(int32_t fileDescriptor, int64_t length);
int32_t aps5_sys_unlink(const char* path);
int32_t aps5_sys_rename(const char* oldPath, const char* newPath);
int32_t aps5_sys_mkdir(const char* path, int32_t mode);
int32_t aps5_sys_named_event_open(const char* name, uint32_t flags, uint64_t* outEventHandle);
int32_t aps5_sys_named_event_delete(const char* name);
int32_t aps5_sys_named_event_set(uint64_t eventHandle);
int32_t aps5_sys_named_event_reset(uint64_t eventHandle);
int32_t aps5_sys_named_event_wait(uint64_t eventHandle, uint64_t timeoutMicroseconds);
int32_t aps5_sys_named_event_close(uint64_t eventHandle);
int32_t aps5_sys_named_sema_create(const char* name, uint32_t attributes, int32_t initCount, int32_t maxCount, uint64_t* outSemaHandle);
int32_t aps5_sys_named_sema_open(const char* name, uint32_t flags, uint64_t* outSemaHandle);
int32_t aps5_sys_named_sema_delete(const char* name);
int32_t aps5_sys_named_sema_wait(uint64_t semaHandle, uint64_t timeoutMicroseconds);
int32_t aps5_sys_named_sema_signal(uint64_t semaHandle, int32_t count);
int32_t aps5_sys_named_sema_close(uint64_t semaHandle);
int32_t aps5_sys_umtx_op(void* obj, int32_t op, uint64_t val, void* uaddr1, void* uaddr2);
}

#endif
