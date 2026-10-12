#include <cstdint>
#include <cstddef>
#include <atomic>
#include <cstdio>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#endif
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/KernelErrors.hpp"

namespace {

// The handler is recorded but never invoked: host crashes are not turned into guest core dumps.
std::atomic<uint64_t> g_coredumpHandler{0};
std::atomic<size_t> g_coredumpStackSize{0};
std::atomic<uint64_t> g_coredumpContext{0};
std::atomic<uint32_t> g_coredumpUserDataType{0};

#ifdef _WIN32
bool IsReadablePointer(const void* ptr, size_t size) {
    if (ptr == nullptr) return false;
    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) return false;
    if (!(mbi.State & MEM_COMMIT)) return false;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
    return true;
}

bool IsReadableString(const char* str) {
    if (str == nullptr) return false;
    return IsReadablePointer(str, 1);
}
#else
bool IsReadablePointer(const void* ptr, size_t size) {
    (void)size;
    return ptr != nullptr;
}
bool IsReadableString(const char* str) {
    return str != nullptr;
}
#endif

} // namespace

extern "C" {

int APS5_VABI sceCoredumpRegisterCoredumpHandler(uint64_t handler, size_t stack_size, uint64_t context) {
    if (handler == 0) {
        return SCE_KERNEL_ERROR_EINVAL;
    }
    g_coredumpHandler.store(handler, std::memory_order_release);
    g_coredumpStackSize.store(stack_size, std::memory_order_release);
    g_coredumpContext.store(context, std::memory_order_release);
    return 0;
}

int APS5_VABI sceCoredumpUnregisterCoredumpHandler(void) {
    g_coredumpHandler.store(0, std::memory_order_release);
    g_coredumpStackSize.store(0, std::memory_order_release);
    g_coredumpContext.store(0, std::memory_order_release);
    return 0;
}

int APS5_VABI sceKernelDebugWriteCppExceptionInfo(const void* exception, uint64_t unknown, const char* typeName, const char* what) {
    (void)unknown;
    std::fprintf(stderr, "[coredump] uncaught C++ exception %p of type %s%s%s\n",
                 exception,
                 (typeName && IsReadableString(typeName)) ? typeName : "(unknown)",
                 what ? ", what(): " : "",
                 (what && IsReadableString(what)) ? what : "");
    return 0;
}

int APS5_VABI sceCoredumpAttachUserFile(const char* path) {
    if (path == nullptr) {
        return SCE_KERNEL_ERROR_EFAULT;
    }
    if (IsReadableString(path)) {
        std::fprintf(stderr, "[coredump] attached user file: %s\n", path);
    }
    return 0;
}

int APS5_VABI sceCoredumpGetStopInfoGpu_Agc(void* stopInfoGpu) {
    if (stopInfoGpu == nullptr) {
        return SCE_KERNEL_ERROR_EFAULT;
    }
    std::memset(stopInfoGpu, 0, 128);
    return 0;
}

int APS5_VABI sceCoredumpAttachMemoryRegionAsUserFile(const void* address, size_t size) {
    if (address == nullptr && size > 0) {
        return SCE_KERNEL_ERROR_EFAULT;
    }
    return 0;
}

int APS5_VABI sceCoredumpSetUserDataType(uint32_t type) {
    g_coredumpUserDataType.store(type, std::memory_order_relaxed);
    return 0;
}

int APS5_VABI sceCoredumpDebugTextOut(const char* text) {
    if (text == nullptr) {
        return SCE_KERNEL_ERROR_EFAULT;
    }
    if (IsReadableString(text)) {
        std::fprintf(stderr, "[coredump debug] %s\n", text);
    }
    return 0;
}

int APS5_VABI sceCoredumpGetStopInfoCpu(void* stopInfoCpu) {
    if (stopInfoCpu == nullptr) {
        return SCE_KERNEL_ERROR_EFAULT;
    }
    std::memset(stopInfoCpu, 0, 128);
    return 0;
}

int APS5_VABI sceCoredumpGetThreadContextInfo(void* threadContextInfo, size_t threadContextInfoSize) {
    (void)threadContextInfo;
    (void)threadContextInfoSize;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCoredumpWriteUserString(const char* str) {
    if (str == nullptr) {
        return SCE_KERNEL_ERROR_EFAULT;
    }
    if (IsReadableString(str)) {
        std::fprintf(stderr, "[coredump string] %s\n", str);
    }
    return 0;
}

int APS5_VABI sceCoredumpAttachUserMemoryFile(const void* address, size_t size) {
    if (address == nullptr && size > 0) {
        return SCE_KERNEL_ERROR_EFAULT;
    }
    return 0;
}

int APS5_VABI sceCoredumpAttachMemoryRegion(const void* address, size_t size) {
    if (address == nullptr && size > 0) {
        return SCE_KERNEL_ERROR_EFAULT;
    }
    return 0;
}

int APS5_VABI sceCoredumpWriteUserData(const void* data, size_t size) {
    if (data == nullptr && size > 0) {
        return SCE_KERNEL_ERROR_EFAULT;
    }
    return 0;
}

}
