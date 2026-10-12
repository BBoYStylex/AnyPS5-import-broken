#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>

extern "C" {
int APS5_VABI sceCoredumpRegisterCoredumpHandler(uint64_t handler, size_t stack_size, uint64_t context);
int APS5_VABI sceCoredumpUnregisterCoredumpHandler(void);
int APS5_VABI sceKernelDebugWriteCppExceptionInfo(const void* exception, uint64_t unknown, const char* typeName, const char* what);
int APS5_VABI sceCoredumpAttachUserFile(const char* path);
int APS5_VABI sceCoredumpGetStopInfoGpu_Agc(void* stopInfoGpu);
int APS5_VABI sceCoredumpAttachMemoryRegionAsUserFile(const void* address, size_t size);
int APS5_VABI sceCoredumpSetUserDataType(uint32_t type);
int APS5_VABI sceCoredumpDebugTextOut(const char* text);
int APS5_VABI sceCoredumpGetStopInfoCpu(void* stopInfoCpu);
int APS5_VABI sceCoredumpWriteUserString(const char* str);
int APS5_VABI sceCoredumpAttachUserMemoryFile(const void* address, size_t size);
int APS5_VABI sceCoredumpAttachMemoryRegion(const void* address, size_t size);
int APS5_VABI sceCoredumpWriteUserData(const void* data, size_t size);
}

static constexpr int SCE_OK = 0;
static constexpr int SCE_KERNEL_ERROR_EFAULT = static_cast<int>(0x8002000e);
static constexpr int SCE_KERNEL_ERROR_EINVAL = static_cast<int>(0x80020016);

static void Require(bool value) {
    if (!value) std::abort();
}

int main() {
    Require(sceCoredumpRegisterCoredumpHandler(0, 0x1000, 0) == SCE_KERNEL_ERROR_EINVAL);
    Require(sceCoredumpRegisterCoredumpHandler(0x12345678, 0x4000, 0x87654321) == SCE_OK);

    Require(sceCoredumpUnregisterCoredumpHandler() == SCE_OK);

    Require(sceKernelDebugWriteCppExceptionInfo(nullptr, 0, nullptr, nullptr) == SCE_OK);
    Require(sceKernelDebugWriteCppExceptionInfo(reinterpret_cast<const void*>(0x1000), 0, "GuestException", "crash") == SCE_OK);

    Require(sceCoredumpSetUserDataType(1) == SCE_OK);
    Require(sceCoredumpSetUserDataType(0xFFFFFFFF) == SCE_OK);

    Require(sceCoredumpWriteUserData(nullptr, 128) == SCE_KERNEL_ERROR_EFAULT);
    Require(sceCoredumpWriteUserData(nullptr, 0) == SCE_OK);
    const char dummy[] = "coredump_test";
    Require(sceCoredumpWriteUserData(dummy, sizeof(dummy)) == SCE_OK);

    Require(sceCoredumpWriteUserString(nullptr) == SCE_KERNEL_ERROR_EFAULT);
    Require(sceCoredumpWriteUserString("Error: Assertion failed in core engine") == SCE_OK);

    Require(sceCoredumpDebugTextOut(nullptr) == SCE_KERNEL_ERROR_EFAULT);
    Require(sceCoredumpDebugTextOut("Debug: Submitting crash trace to buffer") == SCE_OK);

    Require(sceCoredumpGetStopInfoCpu(nullptr) == SCE_KERNEL_ERROR_EFAULT);
    char cpuStop[128];
    std::memset(cpuStop, 0xFF, sizeof(cpuStop));
    Require(sceCoredumpGetStopInfoCpu(cpuStop) == SCE_OK);
    for (int i = 0; i < 128; ++i) {
        Require(cpuStop[i] == 0);
    }

    Require(sceCoredumpGetStopInfoGpu_Agc(nullptr) == SCE_KERNEL_ERROR_EFAULT);
    char gpuStop[128];
    std::memset(gpuStop, 0xFF, sizeof(gpuStop));
    Require(sceCoredumpGetStopInfoGpu_Agc(gpuStop) == SCE_OK);
    for (int i = 0; i < 128; ++i) {
        Require(gpuStop[i] == 0);
    }

    Require(sceCoredumpAttachUserFile(nullptr) == SCE_KERNEL_ERROR_EFAULT);
    Require(sceCoredumpAttachUserFile("/app0/crash_report.log") == SCE_OK);

    Require(sceCoredumpAttachUserMemoryFile(nullptr, 256) == SCE_KERNEL_ERROR_EFAULT);
    Require(sceCoredumpAttachUserMemoryFile(nullptr, 0) == SCE_OK);
    Require(sceCoredumpAttachUserMemoryFile(dummy, sizeof(dummy)) == SCE_OK);

    Require(sceCoredumpAttachMemoryRegionAsUserFile(nullptr, 256) == SCE_KERNEL_ERROR_EFAULT);
    Require(sceCoredumpAttachMemoryRegionAsUserFile(nullptr, 0) == SCE_OK);
    Require(sceCoredumpAttachMemoryRegionAsUserFile(dummy, sizeof(dummy)) == SCE_OK);

    Require(sceCoredumpAttachMemoryRegion(nullptr, 256) == SCE_KERNEL_ERROR_EFAULT);
    Require(sceCoredumpAttachMemoryRegion(nullptr, 0) == SCE_OK);
    Require(sceCoredumpAttachMemoryRegion(dummy, sizeof(dummy)) == SCE_OK);

    return 0;
}
