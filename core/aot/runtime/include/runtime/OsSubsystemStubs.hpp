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
