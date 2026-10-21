#include <runtime/OsSubsystemStubs.hpp>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <iostream>
#include <string>

#if defined(__GNUC__) || defined(__clang__)
extern "C" __attribute__((weak)) void NotImplemented_nid_no_patch(const char* name) {
    static_cast<void>(name);
}
#else
extern "C" void NotImplemented_nid_no_patch(const char* name) {
    static_cast<void>(name);
}
#endif

namespace Aot::Runtime {

std::atomic<bool> OsSubsystemStubs::fallbackSuccess{false};

void OsSubsystemStubs::SetFallbackSuccessEnabled(bool enabled) {
    fallbackSuccess.store(enabled, std::memory_order_release);
}

bool OsSubsystemStubs::IsFallbackSuccessEnabled() {
    return fallbackSuccess.load(std::memory_order_acquire);
}

int64_t OsSubsystemStubs::HandleUnimplementedSyscall(uint32_t syscallId) {
    if (fallbackSuccess.load(std::memory_order_acquire)) {
        return 0;
    }

    std::string identifier = "syscall_" + std::to_string(syscallId);
    NotImplemented_nid_no_patch(identifier.c_str());
    return -1;
}

#if defined(_WIN32)
struct Win32ThreadContext {
    ThreadEntryPoint EntryPoint;
    void* Argument;
};

static DWORD WINAPI Win32ThreadTrampoline(LPVOID parameter) {
    auto* context = static_cast<Win32ThreadContext*>(parameter);
    ThreadEntryPoint entry = context->EntryPoint;
    void* arg = context->Argument;
    delete context;

    void* result = entry(arg);
    return static_cast<DWORD>(reinterpret_cast<uintptr_t>(result));
}
#endif

int32_t OsSubsystemStubs::CreateGuestThread(
    uint64_t* outThreadId,
    const ThreadCreationAttributes* attributes,
    ThreadEntryPoint entryPoint,
    void* argument) {
#if defined(_WIN32)
    auto* context = new Win32ThreadContext{entryPoint, argument};
    size_t stackSize = (attributes != nullptr) ? attributes->StackSize : 0;

    DWORD threadId = 0;
    HANDLE handle = CreateThread(
        nullptr,
        stackSize,
        Win32ThreadTrampoline,
        context,
        0,
        &threadId
    );

    if (handle == nullptr) {
        delete context;
        return -1;
    }

    if (attributes != nullptr && attributes->CpuAffinityMask != 0) {
        SetThreadAffinityMask(handle, static_cast<DWORD_PTR>(attributes->CpuAffinityMask));
    }

    if (outThreadId != nullptr) {
        *outThreadId = reinterpret_cast<uint64_t>(handle);
    } else {
        CloseHandle(handle);
    }

    return 0;
#else
    pthread_t thread{};
    pthread_attr_t attr{};
    pthread_attr_init(&attr);

    if (attributes != nullptr && attributes->StackSize != 0) {
        pthread_attr_setstacksize(&attr, attributes->StackSize);
    }

    int result = pthread_create(&thread, &attr, entryPoint, argument);
    pthread_attr_destroy(&attr);

    if (result != 0) {
        return result;
    }

    if (outThreadId != nullptr) {
        *outThreadId = static_cast<uint64_t>(thread);
    }

    return 0;
#endif
}

int32_t OsSubsystemStubs::JoinGuestThread(uint64_t threadId, void** outReturnValue) {
#if defined(_WIN32)
    HANDLE handle = reinterpret_cast<HANDLE>(threadId);
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
        return -1;
    }

    DWORD waitResult = WaitForSingleObject(handle, INFINITE);
    if (waitResult != WAIT_OBJECT_0) {
        CloseHandle(handle);
        return -1;
    }

    if (outReturnValue != nullptr) {
        DWORD exitCode = 0;
        GetExitCodeThread(handle, &exitCode);
        *outReturnValue = reinterpret_cast<void*>(static_cast<uintptr_t>(exitCode));
    }

    CloseHandle(handle);
    return 0;
#else
    void* returnValue = nullptr;
    int result = pthread_join(static_cast<pthread_t>(threadId), &returnValue);
    if (outReturnValue != nullptr) {
        *outReturnValue = returnValue;
    }
    return result;
#endif
}

int32_t OsSubsystemStubs::FileOpen(const char* path, int32_t flags, int32_t mode) {
#if defined(_WIN32)
    int fd = -1;
    _sopen_s(&fd, path, flags, _SH_DENYNO, mode);
    return fd;
#else
    return open(path, flags, static_cast<mode_t>(mode));
#endif
}

int64_t OsSubsystemStubs::FileRead(int32_t fileDescriptor, void* buffer, size_t byteCount) {
#if defined(_WIN32)
    return _read(fileDescriptor, buffer, static_cast<unsigned int>(byteCount));
#else
    return read(fileDescriptor, buffer, byteCount);
#endif
}

int64_t OsSubsystemStubs::FileWrite(int32_t fileDescriptor, const void* buffer, size_t byteCount) {
#if defined(_WIN32)
    return _write(fileDescriptor, buffer, static_cast<unsigned int>(byteCount));
#else
    return write(fileDescriptor, buffer, byteCount);
#endif
}

int64_t OsSubsystemStubs::FileSeek(int32_t fileDescriptor, int64_t offset, int32_t whence) {
#if defined(_WIN32)
    return _lseeki64(fileDescriptor, offset, whence);
#else
    return lseek(fileDescriptor, offset, whence);
#endif
}

int32_t OsSubsystemStubs::FileClose(int32_t fileDescriptor) {
#if defined(_WIN32)
    return _close(fileDescriptor);
#else
    return close(fileDescriptor);
#endif
}

bool OsSubsystemStubs::AtomicCompareExchange64(std::atomic<int64_t>* target, int64_t* expected, int64_t desired) {
    return target->compare_exchange_strong(*expected, desired, std::memory_order_seq_cst);
}

int64_t OsSubsystemStubs::AtomicAdd64(std::atomic<int64_t>* target, int64_t value) {
    return target->fetch_add(value, std::memory_order_seq_cst);
}

int64_t OsSubsystemStubs::AtomicExchange64(std::atomic<int64_t>* target, int64_t desired) {
    return target->exchange(desired, std::memory_order_seq_cst);
}

} // namespace Aot::Runtime

extern "C" {

int64_t aps5_syscall_unimplemented_fallback(uint32_t syscallId) {
    return Aot::Runtime::OsSubsystemStubs::HandleUnimplementedSyscall(syscallId);
}

}
