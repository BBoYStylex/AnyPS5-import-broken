#ifndef CORE_AOT_RUNTIME_OSSUBSYSTEMSTUBS_HPP
#define CORE_AOT_RUNTIME_OSSUBSYSTEMSTUBS_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <functional>
#include <atomic>
#include <span>

namespace Aot::Runtime {

using ThreadEntryPoint = void* (*)(void*);

struct ThreadCreationAttributes {
    const char* Name;
    size_t StackSize;
    int32_t Priority;
    uint64_t CpuAffinityMask;
};

class OsSubsystemStubs {
public:
    static void SetFallbackSuccessEnabled(bool enabled);
    static bool IsFallbackSuccessEnabled();

    static int64_t HandleUnimplementedSyscall(uint32_t syscallId);

    static int32_t CreateGuestThread(
        uint64_t* outThreadId,
        const ThreadCreationAttributes* attributes,
        ThreadEntryPoint entryPoint,
        void* argument
    );

    static int32_t JoinGuestThread(uint64_t threadId, void** outReturnValue);

    static int32_t FileOpen(const char* path, int32_t flags, int32_t mode);
    static int64_t FileRead(int32_t fileDescriptor, void* buffer, size_t byteCount);
    static int64_t FileWrite(int32_t fileDescriptor, const void* buffer, size_t byteCount);
    static int64_t FileSeek(int32_t fileDescriptor, int64_t offset, int32_t whence);
    static int32_t FileClose(int32_t fileDescriptor);
    static int32_t FileStat(int32_t fileDescriptor, uint64_t* outSize, uint32_t* outMode);
    static int32_t FileTruncate(int32_t fileDescriptor, int64_t length);
    static int32_t FileUnlink(const char* path);
    static int32_t FileRename(const char* oldPath, const char* newPath);
    static int32_t FileMkdir(const char* path, int32_t mode);
    static int64_t FilePread(int32_t fileDescriptor, void* buffer, size_t byteCount, int64_t offset);
    static int64_t FilePwrite(int32_t fileDescriptor, const void* buffer, size_t byteCount, int64_t offset);

    static int32_t NamedEventCreate(const char* name, uint32_t attributes, uint64_t* outHandle);
    static int32_t NamedEventOpen(const char* name, uint32_t flags, uint64_t* outHandle);
    static int32_t NamedEventDelete(const char* name);
    static int32_t NamedEventSet(uint64_t handle);
    static int32_t NamedEventReset(uint64_t handle);
    static int32_t NamedEventWait(uint64_t handle, uint64_t timeoutMicroseconds);
    static int32_t NamedEventClose(uint64_t handle);

    static int32_t NamedSemaCreate(const char* name, uint32_t attributes, int32_t initCount, int32_t maxCount, uint64_t* outHandle);
    static int32_t NamedSemaOpen(const char* name, uint32_t flags, uint64_t* outHandle);
    static int32_t NamedSemaDelete(const char* name);
    static int32_t NamedSemaWait(uint64_t handle, uint64_t timeoutMicroseconds);
    static int32_t NamedSemaSignal(uint64_t handle, int32_t count);
    static int32_t NamedSemaClose(uint64_t handle);

    static uint64_t GetMonotonicClockMicroseconds();

    static bool AtomicCompareExchange64(std::atomic<int64_t>* target, int64_t* expected, int64_t desired);
    static int64_t AtomicAdd64(std::atomic<int64_t>* target, int64_t value);
    static int64_t AtomicExchange64(std::atomic<int64_t>* target, int64_t desired);

private:
    static std::atomic<bool> fallbackSuccess;
};

} // namespace Aot::Runtime

extern "C" {
int64_t aps5_syscall_unimplemented_fallback(uint32_t syscallId);
}

#endif
