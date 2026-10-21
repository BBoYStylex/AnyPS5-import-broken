#ifndef CORE_LIBS_PRX_LIBMDBG_SYSCORE_MDBG_HPP
#define CORE_LIBS_PRX_LIBMDBG_SYSCORE_MDBG_HPP

#include <cstddef>
#include <cstdint>
#include "prx/libc/include/general/VabiMacros.hpp"

extern "C" {

struct SceDebugAppInfo;
struct SceDebugModuleInfo;
struct SceDebugProcessInfo;
struct SceDebugThreadInfo;
struct SceDebugVirtualQueryInfo;
struct SceKernelEvent;

using SceDebugModuleId = std::uint64_t;

struct SceDebugRegister {
    std::uint64_t data[4];
};

static_assert(sizeof(SceDebugRegister) == 32);

enum class SceDebugRegisterId : std::int32_t {
    SCE_DBGP_REG_ID_RAX = 0x10000,
    SCE_DBGP_REG_ID_RCX,
    SCE_DBGP_REG_ID_RDX,
    SCE_DBGP_REG_ID_RBX,
    SCE_DBGP_REG_ID_RSP,
    SCE_DBGP_REG_ID_RBP,
    SCE_DBGP_REG_ID_RSI,
    SCE_DBGP_REG_ID_RDI,
    SCE_DBGP_REG_ID_R8,
    SCE_DBGP_REG_ID_R9,
    SCE_DBGP_REG_ID_R10,
    SCE_DBGP_REG_ID_R11,
    SCE_DBGP_REG_ID_R12,
    SCE_DBGP_REG_ID_R13,
    SCE_DBGP_REG_ID_R14,
    SCE_DBGP_REG_ID_R15,
    SCE_DBGP_REG_ID_RIP = 0x10012,
    SCE_DBGP_REG_ID_RFLAGS = 0x10015,
    SCE_DBGP_REG_ID_cw = 0x20000,
    SCE_DBGP_REG_ID_sw,
    SCE_DBGP_REG_ID_tw,
    SCE_DBGP_REG_ID_opcode = 0x20004,
    SCE_DBGP_REG_ID_FPURIP,
    SCE_DBGP_REG_ID_FPURDP,
    SCE_DBGP_REG_ID_mxcsr,
    SCE_DBGP_REG_ID_mxcsr_mask,
    SCE_DBGP_REG_ID_acc0 = 0x30000,
    SCE_DBGP_REG_ID_acc1,
    SCE_DBGP_REG_ID_acc2,
    SCE_DBGP_REG_ID_acc3,
    SCE_DBGP_REG_ID_acc4,
    SCE_DBGP_REG_ID_acc5,
    SCE_DBGP_REG_ID_acc6,
    SCE_DBGP_REG_ID_acc7,
    SCE_DBGP_REG_ID_ymm0 = 0x40000,
    SCE_DBGP_REG_ID_ymm1,
    SCE_DBGP_REG_ID_ymm2,
    SCE_DBGP_REG_ID_ymm3,
    SCE_DBGP_REG_ID_ymm4,
    SCE_DBGP_REG_ID_ymm5,
    SCE_DBGP_REG_ID_ymm6,
    SCE_DBGP_REG_ID_ymm7,
    SCE_DBGP_REG_ID_ymm8,
    SCE_DBGP_REG_ID_ymm9,
    SCE_DBGP_REG_ID_ymm10,
    SCE_DBGP_REG_ID_ymm11,
    SCE_DBGP_REG_ID_ymm12,
    SCE_DBGP_REG_ID_ymm13,
    SCE_DBGP_REG_ID_ymm14,
    SCE_DBGP_REG_ID_ymm15,
    SCE_DBGP_REG_ID_DR0 = 0x50000,
    SCE_DBGP_REG_ID_DR1,
    SCE_DBGP_REG_ID_DR2,
    SCE_DBGP_REG_ID_DR3,
    SCE_DBGP_REG_ID_DR4,
    SCE_DBGP_REG_ID_DR5,
    SCE_DBGP_REG_ID_DR6,
    SCE_DBGP_REG_ID_DR7,
    SCE_DBGP_REG_ID_FSBASE = 0x60000,
};

int APS5_VABI sceDebugInit(int a1);
int APS5_VABI sceDebugAttachProcess(int pid);
int APS5_VABI sceDebugClearStepThread(int tid);
int APS5_VABI sceDebugCreateScratchExecutableArea(int pid, std::size_t size, std::uint64_t* addressOut);
int APS5_VABI sceDebugCreateScratchExecutableAreaForPrx(int pid, std::size_t size, const char* name, std::uint64_t* addressOut);
int APS5_VABI sceDebugDestroyScratchExecutableArea(int pid, std::uint64_t address, std::size_t size);
int APS5_VABI sceDebugDetachProcess(int pid);
int APS5_VABI sceDebugGetApplicationIdByTitleId(char* titleId, int* appId);
int APS5_VABI sceDebugGetApplicationInfo(int appId, SceDebugAppInfo* info);
int APS5_VABI sceDebugGetApplicationList(int* appIds, int maxAppIds, int* actualAppIds);
int APS5_VABI sceDebugGetEventList(int pid, std::uint64_t sid, SceKernelEvent* dataPtr, std::uint64_t a4, std::size_t** actualEvents);
int APS5_VABI sceDebugGetModuleInfo(int pid, SceDebugModuleId moduleId, SceDebugModuleInfo* moduleInfo);
int APS5_VABI sceDebugGetModuleList(int pid, SceDebugModuleId* moduleIdList, int moduleIdListSize, int* moduleIdCount);
int APS5_VABI sceDebugGetModuleMetaData(int pid, SceDebugModuleId moduleId, void* metaData, std::size_t metaDataSize, std::size_t* actualMetaData);
int APS5_VABI sceDebugGetProcessInfo(int pid, SceDebugProcessInfo* processInfo);
int APS5_VABI sceDebugGetProcessList(int* pidList, int pidListSize, int* pidCount);
int APS5_VABI sceDebugGetThreadInfo(int pid, int tid, SceDebugThreadInfo* info);
int APS5_VABI sceDebugGetThreadList(int pid, int* tidList, int tidListSize, int* tidCount);
int APS5_VABI sceDebugGetVirtualMemoryInfo(int pid, std::uint64_t address, SceDebugVirtualQueryInfo* info);
int APS5_VABI sceDebugKillApplication(int context, int appid);
int APS5_VABI sceDebugKillProcess(int pid);
int APS5_VABI sceDebugNoStopChildProcesses(int pid);
int APS5_VABI sceDebugNoStopOnDLLoad(int pid);
int APS5_VABI sceDebugProcessSpawn(int context, void* argv, int flags, std::size_t stackSize, const char* workdir, const char* pathFile);
int APS5_VABI sceDebugReadProcessMemory(int pid, std::uint64_t addrss, std::size_t size, std::uint8_t* data, std::size_t* sizeRead);
int APS5_VABI sceDebugReadThreadRegister(int tid, SceDebugRegisterId reg, SceDebugRegister* value);
int APS5_VABI sceDebugResumeApplication(int context, int appId);
int APS5_VABI sceDebugResumeProcess(int pid, int a2);
int APS5_VABI sceDebugResumeThread(int tid);
int APS5_VABI sceDebugSetStepThread(int tid);
int APS5_VABI sceDebugSpawnApplication(int context, char* titleId, void* argv, int flags, int mountFlags);
int APS5_VABI sceDebugStopChildProcesses(int pid);
int APS5_VABI sceDebugStopOnDLLoad(int pid);
int APS5_VABI sceDebugSuspendApplication(int context, int appid);
int APS5_VABI sceDebugSuspendProcess(int pid);
int APS5_VABI sceDebugSuspendThread(int tid);
int APS5_VABI sceDebugWriteProcessMemory(int pid, std::uint64_t address, std::size_t size, std::uint8_t* data, std::size_t* sizeWritten);
int APS5_VABI sceDebugWriteThreadRegister(int tid, SceDebugRegisterId reg, SceDebugRegister value);

}

#endif
