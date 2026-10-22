#include <runtime/OsSubsystemStubs.hpp>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <direct.h>
#include <sys/stat.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif

#include <iostream>
#include <string>
#include <cstdio>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <memory>

extern "C" void NotImplemented_nid_no_patch(const char* name) {
    static_cast<void>(name);
}

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

int32_t OsSubsystemStubs::FileStat(int32_t fileDescriptor, uint64_t* outSize, uint32_t* outMode) {
#if defined(_WIN32)
    struct _stat64 st{};
    int res = _fstat64(fileDescriptor, &st);
    if (res != 0) {
        return -1;
    }
    if (outSize != nullptr) {
        *outSize = static_cast<uint64_t>(st.st_size);
    }
    if (outMode != nullptr) {
        *outMode = static_cast<uint32_t>(st.st_mode);
    }
    return 0;
#else
    struct stat st{};
    int res = fstat(fileDescriptor, &st);
    if (res != 0) {
        return -1;
    }
    if (outSize != nullptr) {
        *outSize = static_cast<uint64_t>(st.st_size);
    }
    if (outMode != nullptr) {
        *outMode = static_cast<uint32_t>(st.st_mode);
    }
    return 0;
#endif
}

int32_t OsSubsystemStubs::FileTruncate(int32_t fileDescriptor, int64_t length) {
#if defined(_WIN32)
    return (_chsize_s(fileDescriptor, length) == 0) ? 0 : -1;
#else
    return ftruncate(fileDescriptor, length);
#endif
}

int32_t OsSubsystemStubs::FileUnlink(const char* path) {
#if defined(_WIN32)
    return _unlink(path);
#else
    return unlink(path);
#endif
}

int32_t OsSubsystemStubs::FileRename(const char* oldPath, const char* newPath) {
    return std::rename(oldPath, newPath);
}

int32_t OsSubsystemStubs::FileMkdir(const char* path, int32_t mode) {
#if defined(_WIN32)
    static_cast<void>(mode);
    return _mkdir(path);
#else
    return mkdir(path, static_cast<mode_t>(mode));
#endif
}

int64_t OsSubsystemStubs::FilePread(int32_t fileDescriptor, void* buffer, size_t byteCount, int64_t offset) {
#if defined(_WIN32)
    int64_t original = _lseeki64(fileDescriptor, 0, SEEK_CUR);
    if (original < 0) {
        return -1;
    }
    if (_lseeki64(fileDescriptor, offset, SEEK_SET) < 0) {
        return -1;
    }
    int64_t bytesRead = _read(fileDescriptor, buffer, static_cast<unsigned int>(byteCount));
    _lseeki64(fileDescriptor, original, SEEK_SET);
    return bytesRead;
#else
    return pread(fileDescriptor, buffer, byteCount, offset);
#endif
}

int64_t OsSubsystemStubs::FilePwrite(int32_t fileDescriptor, const void* buffer, size_t byteCount, int64_t offset) {
#if defined(_WIN32)
    int64_t original = _lseeki64(fileDescriptor, 0, SEEK_CUR);
    if (original < 0) {
        return -1;
    }
    if (_lseeki64(fileDescriptor, offset, SEEK_SET) < 0) {
        return -1;
    }
    int64_t bytesWritten = _write(fileDescriptor, buffer, static_cast<unsigned int>(byteCount));
    _lseeki64(fileDescriptor, original, SEEK_SET);
    return bytesWritten;
#else
    return pwrite(fileDescriptor, buffer, byteCount, offset);
#endif
}

struct NamedEventObject {
    std::mutex Mutex;
    std::condition_variable Cv;
    bool Signaled{false};
    bool ManualReset{false};
    std::string Name;
};

static std::unordered_map<std::string, std::shared_ptr<NamedEventObject>> gNamedEvents;
static std::unordered_map<uint64_t, std::shared_ptr<NamedEventObject>> gEventHandles;
static std::mutex gNamedEventMutex;
static std::atomic<uint64_t> gNextEventHandle{2000};

int32_t OsSubsystemStubs::NamedEventCreate(const char* name, uint32_t attributes, uint64_t* outHandle) {
    auto eventObj = std::make_shared<NamedEventObject>();
    eventObj->ManualReset = (attributes & 1) != 0;
    if (name != nullptr) {
        eventObj->Name = name;
    }

    uint64_t handle = gNextEventHandle.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(gNamedEventMutex);
        gEventHandles[handle] = eventObj;
        if (name != nullptr && name[0] != '\0') {
            gNamedEvents[eventObj->Name] = eventObj;
        }
    }

    if (outHandle != nullptr) {
        *outHandle = handle;
    }
    return 0;
}

int32_t OsSubsystemStubs::NamedEventOpen(const char* name, uint32_t flags, uint64_t* outHandle) {
    static_cast<void>(flags);
    if (name == nullptr) {
        return -1;
    }

    std::shared_ptr<NamedEventObject> eventObj;
    {
        std::lock_guard<std::mutex> lock(gNamedEventMutex);
        auto it = gNamedEvents.find(name);
        if (it == gNamedEvents.end()) {
            return -1;
        }
        eventObj = it->second;
        uint64_t handle = gNextEventHandle.fetch_add(1, std::memory_order_relaxed);
        gEventHandles[handle] = eventObj;
        if (outHandle != nullptr) {
            *outHandle = handle;
        }
    }
    return 0;
}

int32_t OsSubsystemStubs::NamedEventDelete(const char* name) {
    if (name == nullptr) {
        return -1;
    }
    std::lock_guard<std::mutex> lock(gNamedEventMutex);
    gNamedEvents.erase(name);
    return 0;
}

int32_t OsSubsystemStubs::NamedEventSet(uint64_t handle) {
    std::shared_ptr<NamedEventObject> eventObj;
    {
        std::lock_guard<std::mutex> lock(gNamedEventMutex);
        auto it = gEventHandles.find(handle);
        if (it == gEventHandles.end()) {
            return -1;
        }
        eventObj = it->second;
    }

    {
        std::lock_guard<std::mutex> lock(eventObj->Mutex);
        eventObj->Signaled = true;
    }
    eventObj->Cv.notify_all();
    return 0;
}

int32_t OsSubsystemStubs::NamedEventReset(uint64_t handle) {
    std::shared_ptr<NamedEventObject> eventObj;
    {
        std::lock_guard<std::mutex> lock(gNamedEventMutex);
        auto it = gEventHandles.find(handle);
        if (it == gEventHandles.end()) {
            return -1;
        }
        eventObj = it->second;
    }

    std::lock_guard<std::mutex> lock(eventObj->Mutex);
    eventObj->Signaled = false;
    return 0;
}

int32_t OsSubsystemStubs::NamedEventWait(uint64_t handle, uint64_t timeoutMicroseconds) {
    std::shared_ptr<NamedEventObject> eventObj;
    {
        std::lock_guard<std::mutex> lock(gNamedEventMutex);
        auto it = gEventHandles.find(handle);
        if (it == gEventHandles.end()) {
            return -1;
        }
        eventObj = it->second;
    }

    std::unique_lock<std::mutex> lock(eventObj->Mutex);
    if (timeoutMicroseconds == 0) {
        if (eventObj->Signaled) {
            if (!eventObj->ManualReset) {
                eventObj->Signaled = false;
            }
            return 0;
        }
        return -1;
    }

    if (timeoutMicroseconds == UINT64_MAX) {
        eventObj->Cv.wait(lock, [&] { return eventObj->Signaled; });
        if (!eventObj->ManualReset) {
            eventObj->Signaled = false;
        }
        return 0;
    }

    bool success = eventObj->Cv.wait_for(lock, std::chrono::microseconds(timeoutMicroseconds), [&] {
        return eventObj->Signaled;
    });
    if (success) {
        if (!eventObj->ManualReset) {
            eventObj->Signaled = false;
        }
        return 0;
    }
    return -1;
}

int32_t OsSubsystemStubs::NamedEventClose(uint64_t handle) {
    std::lock_guard<std::mutex> lock(gNamedEventMutex);
    return gEventHandles.erase(handle) > 0 ? 0 : -1;
}

struct NamedSemaObject {
    std::mutex Mutex;
    std::condition_variable Cv;
    int32_t Count{0};
    int32_t MaxCount{1};
    std::string Name;
};

static std::unordered_map<std::string, std::shared_ptr<NamedSemaObject>> gNamedSemas;
static std::unordered_map<uint64_t, std::shared_ptr<NamedSemaObject>> gSemaHandles;
static std::mutex gNamedSemaMutex;
static std::atomic<uint64_t> gNextSemaHandle{3000};

int32_t OsSubsystemStubs::NamedSemaCreate(const char* name, uint32_t attributes, int32_t initCount, int32_t maxCount, uint64_t* outHandle) {
    static_cast<void>(attributes);
    if (initCount < 0 || maxCount <= 0 || initCount > maxCount) {
        return -1;
    }

    auto semaObj = std::make_shared<NamedSemaObject>();
    semaObj->Count = initCount;
    semaObj->MaxCount = maxCount;
    if (name != nullptr) {
        semaObj->Name = name;
    }

    uint64_t handle = gNextSemaHandle.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(gNamedSemaMutex);
        gSemaHandles[handle] = semaObj;
        if (name != nullptr && name[0] != '\0') {
            gNamedSemas[semaObj->Name] = semaObj;
        }
    }

    if (outHandle != nullptr) {
        *outHandle = handle;
    }
    return 0;
}

int32_t OsSubsystemStubs::NamedSemaOpen(const char* name, uint32_t flags, uint64_t* outHandle) {
    static_cast<void>(flags);
    if (name == nullptr) {
        return -1;
    }

    std::shared_ptr<NamedSemaObject> semaObj;
    {
        std::lock_guard<std::mutex> lock(gNamedSemaMutex);
        auto it = gNamedSemas.find(name);
        if (it == gNamedSemas.end()) {
            return -1;
        }
        semaObj = it->second;
        uint64_t handle = gNextSemaHandle.fetch_add(1, std::memory_order_relaxed);
        gSemaHandles[handle] = semaObj;
        if (outHandle != nullptr) {
            *outHandle = handle;
        }
    }
    return 0;
}

int32_t OsSubsystemStubs::NamedSemaDelete(const char* name) {
    if (name == nullptr) {
        return -1;
    }
    std::lock_guard<std::mutex> lock(gNamedSemaMutex);
    gNamedSemas.erase(name);
    return 0;
}

int32_t OsSubsystemStubs::NamedSemaWait(uint64_t handle, uint64_t timeoutMicroseconds) {
    std::shared_ptr<NamedSemaObject> semaObj;
    {
        std::lock_guard<std::mutex> lock(gNamedSemaMutex);
        auto it = gSemaHandles.find(handle);
        if (it == gSemaHandles.end()) {
            return -1;
        }
        semaObj = it->second;
    }

    std::unique_lock<std::mutex> lock(semaObj->Mutex);
    if (timeoutMicroseconds == 0) {
        if (semaObj->Count > 0) {
            --semaObj->Count;
            return 0;
        }
        return -1;
    }

    if (timeoutMicroseconds == UINT64_MAX) {
        semaObj->Cv.wait(lock, [&] { return semaObj->Count > 0; });
        --semaObj->Count;
        return 0;
    }

    bool success = semaObj->Cv.wait_for(lock, std::chrono::microseconds(timeoutMicroseconds), [&] {
        return semaObj->Count > 0;
    });
    if (success) {
        --semaObj->Count;
        return 0;
    }
    return -1;
}

int32_t OsSubsystemStubs::NamedSemaSignal(uint64_t handle, int32_t count) {
    if (count <= 0) {
        return -1;
    }

    std::shared_ptr<NamedSemaObject> semaObj;
    {
        std::lock_guard<std::mutex> lock(gNamedSemaMutex);
        auto it = gSemaHandles.find(handle);
        if (it == gSemaHandles.end()) {
            return -1;
        }
        semaObj = it->second;
    }

    {
        std::lock_guard<std::mutex> lock(semaObj->Mutex);
        semaObj->Count = std::min(semaObj->MaxCount, semaObj->Count + count);
    }
    semaObj->Cv.notify_all();
    return 0;
}

int32_t OsSubsystemStubs::NamedSemaClose(uint64_t handle) {
    std::lock_guard<std::mutex> lock(gNamedSemaMutex);
    return gSemaHandles.erase(handle) > 0 ? 0 : -1;
}

uint64_t OsSubsystemStubs::GetMonotonicClockMicroseconds() {
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count());
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
