#include "prx/libmdbg_syscore/Mdbg.hpp"
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>

static_assert(std::is_same_v<decltype(&sceDebugAttachProcess), int (APS5_VABI*)(int)>);
static_assert(std::is_same_v<decltype(&sceDebugClearStepThread), int (APS5_VABI*)(int)>);
static_assert(std::is_same_v<decltype(&sceDebugCreateScratchExecutableArea), int (APS5_VABI*)(int, std::size_t, std::uint64_t*)>);
static_assert(std::is_same_v<decltype(&sceDebugCreateScratchExecutableAreaForPrx), int (APS5_VABI*)(int, std::size_t, const char*, std::uint64_t*)>);
static_assert(std::is_same_v<decltype(&sceDebugDestroyScratchExecutableArea), int (APS5_VABI*)(int, std::uint64_t, std::size_t)>);
static_assert(std::is_same_v<decltype(&sceDebugDetachProcess), int (APS5_VABI*)(int)>);
static_assert(std::is_same_v<decltype(&sceDebugGetApplicationIdByTitleId), int (APS5_VABI*)(char*, int*)>);
static_assert(std::is_same_v<decltype(&sceDebugGetApplicationInfo), int (APS5_VABI*)(int, SceDebugAppInfo*)>);
static_assert(std::is_same_v<decltype(&sceDebugGetApplicationList), int (APS5_VABI*)(int*, int, int*)>);
static_assert(std::is_same_v<decltype(&sceDebugGetEventList), int (APS5_VABI*)(int, std::uint64_t, SceKernelEvent*, std::uint64_t, std::size_t**)>);
static_assert(std::is_same_v<decltype(&sceDebugGetModuleInfo), int (APS5_VABI*)(int, SceDebugModuleId, SceDebugModuleInfo*)>);
static_assert(std::is_same_v<decltype(&sceDebugGetModuleList), int (APS5_VABI*)(int, SceDebugModuleId*, int, int*)>);
static_assert(std::is_same_v<decltype(&sceDebugGetModuleMetaData), int (APS5_VABI*)(int, SceDebugModuleId, void*, std::size_t, std::size_t*)>);
static_assert(std::is_same_v<decltype(&sceDebugGetProcessInfo), int (APS5_VABI*)(int, SceDebugProcessInfo*)>);
static_assert(std::is_same_v<decltype(&sceDebugGetProcessList), int (APS5_VABI*)(int*, int, int*)>);
static_assert(std::is_same_v<decltype(&sceDebugGetThreadInfo), int (APS5_VABI*)(int, int, SceDebugThreadInfo*)>);
static_assert(std::is_same_v<decltype(&sceDebugGetThreadList), int (APS5_VABI*)(int, int*, int, int*)>);
static_assert(std::is_same_v<decltype(&sceDebugGetVirtualMemoryInfo), int (APS5_VABI*)(int, std::uint64_t, SceDebugVirtualQueryInfo*)>);
static_assert(std::is_same_v<decltype(&sceDebugInit), int (APS5_VABI*)(int)>);
static_assert(std::is_same_v<decltype(&sceDebugKillApplication), int (APS5_VABI*)(int, int)>);
static_assert(std::is_same_v<decltype(&sceDebugKillProcess), int (APS5_VABI*)(int)>);
static_assert(std::is_same_v<decltype(&sceDebugNoStopChildProcesses), int (APS5_VABI*)(int)>);
static_assert(std::is_same_v<decltype(&sceDebugNoStopOnDLLoad), int (APS5_VABI*)(int)>);
static_assert(std::is_same_v<decltype(&sceDebugProcessSpawn), int (APS5_VABI*)(int, void*, int, std::size_t, const char*, const char*)>);
static_assert(std::is_same_v<decltype(&sceDebugReadProcessMemory), int (APS5_VABI*)(int, std::uint64_t, std::size_t, std::uint8_t*, std::size_t*)>);
static_assert(std::is_same_v<decltype(&sceDebugReadThreadRegister), int (APS5_VABI*)(int, SceDebugRegisterId, SceDebugRegister*)>);
static_assert(std::is_same_v<decltype(&sceDebugResumeApplication), int (APS5_VABI*)(int, int)>);
static_assert(std::is_same_v<decltype(&sceDebugResumeProcess), int (APS5_VABI*)(int, int)>);
static_assert(std::is_same_v<decltype(&sceDebugResumeThread), int (APS5_VABI*)(int)>);
static_assert(std::is_same_v<decltype(&sceDebugSetStepThread), int (APS5_VABI*)(int)>);
static_assert(std::is_same_v<decltype(&sceDebugSpawnApplication), int (APS5_VABI*)(int, char*, void*, int, int)>);
static_assert(std::is_same_v<decltype(&sceDebugStopChildProcesses), int (APS5_VABI*)(int)>);
static_assert(std::is_same_v<decltype(&sceDebugStopOnDLLoad), int (APS5_VABI*)(int)>);
static_assert(std::is_same_v<decltype(&sceDebugSuspendApplication), int (APS5_VABI*)(int, int)>);
static_assert(std::is_same_v<decltype(&sceDebugSuspendProcess), int (APS5_VABI*)(int)>);
static_assert(std::is_same_v<decltype(&sceDebugSuspendThread), int (APS5_VABI*)(int)>);
static_assert(std::is_same_v<decltype(&sceDebugWriteProcessMemory), int (APS5_VABI*)(int, std::uint64_t, std::size_t, std::uint8_t*, std::size_t*)>);
static_assert(std::is_same_v<decltype(&sceDebugWriteThreadRegister), int (APS5_VABI*)(int, SceDebugRegisterId, SceDebugRegister)>);

namespace {

template <typename Call>
void RequireThrows(const char* name, Call&& call) {
    try {
        call();
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()).rfind(name, 0) == 0) return;
        std::fprintf(stderr, "%s threw %s\n", name, error.what());
        std::exit(1);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s threw a non-runtime error: %s\n", name, error.what());
        std::exit(1);
    }
    std::fprintf(stderr, "%s returned instead of throwing\n", name);
    std::exit(1);
}

}

int main() {
    RequireThrows("sceDebugAttachProcess", [] { sceDebugAttachProcess(0); });
    RequireThrows("sceDebugClearStepThread", [] { sceDebugClearStepThread(0); });
    RequireThrows("sceDebugCreateScratchExecutableArea", [] { sceDebugCreateScratchExecutableArea(1, 0, nullptr); });
    RequireThrows("sceDebugCreateScratchExecutableAreaForPrx", [] { sceDebugCreateScratchExecutableAreaForPrx(1, 0, nullptr, nullptr); });
    RequireThrows("sceDebugDestroyScratchExecutableArea", [] { sceDebugDestroyScratchExecutableArea(1, 0, 0); });
    RequireThrows("sceDebugDetachProcess", [] { sceDebugDetachProcess(0); });
    RequireThrows("sceDebugGetApplicationIdByTitleId", [] { sceDebugGetApplicationIdByTitleId(nullptr, nullptr); });
    RequireThrows("sceDebugGetApplicationInfo", [] { sceDebugGetApplicationInfo(1, nullptr); });
    RequireThrows("sceDebugGetApplicationList", [] { sceDebugGetApplicationList(nullptr, 0, nullptr); });
    RequireThrows("sceDebugGetEventList", [] { sceDebugGetEventList(1, 0, nullptr, 0, nullptr); });
    RequireThrows("sceDebugGetModuleInfo", [] { sceDebugGetModuleInfo(1, 0, nullptr); });
    RequireThrows("sceDebugGetModuleList", [] { sceDebugGetModuleList(1, nullptr, 0, nullptr); });
    RequireThrows("sceDebugGetModuleMetaData", [] { sceDebugGetModuleMetaData(1, 0, nullptr, 0, nullptr); });
    RequireThrows("sceDebugGetProcessInfo", [] { sceDebugGetProcessInfo(1, nullptr); });
    RequireThrows("sceDebugGetProcessList", [] { sceDebugGetProcessList(nullptr, 0, nullptr); });
    RequireThrows("sceDebugGetThreadInfo", [] { sceDebugGetThreadInfo(1, 2, nullptr); });
    RequireThrows("sceDebugGetThreadList", [] { sceDebugGetThreadList(1, nullptr, 0, nullptr); });
    RequireThrows("sceDebugGetVirtualMemoryInfo", [] { sceDebugGetVirtualMemoryInfo(1, 0, nullptr); });
    RequireThrows("sceDebugInit", [] { sceDebugInit(1); });
    RequireThrows("sceDebugKillApplication", [] { sceDebugKillApplication(1, 2); });
    RequireThrows("sceDebugKillProcess", [] { sceDebugKillProcess(0); });
    RequireThrows("sceDebugNoStopChildProcesses", [] { sceDebugNoStopChildProcesses(0); });
    RequireThrows("sceDebugNoStopOnDLLoad", [] { sceDebugNoStopOnDLLoad(0); });
    RequireThrows("sceDebugProcessSpawn", [] { sceDebugProcessSpawn(1, nullptr, 0, 0, nullptr, nullptr); });
    RequireThrows("sceDebugReadProcessMemory", [] { sceDebugReadProcessMemory(1, 0, 0, nullptr, nullptr); });
    RequireThrows("sceDebugReadThreadRegister", [] { sceDebugReadThreadRegister(1, SceDebugRegisterId::SCE_DBGP_REG_ID_RAX, nullptr); });
    RequireThrows("sceDebugResumeApplication", [] { sceDebugResumeApplication(1, 2); });
    RequireThrows("sceDebugResumeProcess", [] { sceDebugResumeProcess(0, 0); });
    RequireThrows("sceDebugResumeThread", [] { sceDebugResumeThread(0); });
    RequireThrows("sceDebugSetStepThread", [] { sceDebugSetStepThread(0); });
    RequireThrows("sceDebugSpawnApplication", [] { sceDebugSpawnApplication(1, nullptr, nullptr, 0, 0); });
    RequireThrows("sceDebugStopChildProcesses", [] { sceDebugStopChildProcesses(0); });
    RequireThrows("sceDebugStopOnDLLoad", [] { sceDebugStopOnDLLoad(0); });
    RequireThrows("sceDebugSuspendApplication", [] { sceDebugSuspendApplication(1, 2); });
    RequireThrows("sceDebugSuspendProcess", [] { sceDebugSuspendProcess(0); });
    RequireThrows("sceDebugSuspendThread", [] { sceDebugSuspendThread(0); });
    RequireThrows("sceDebugWriteProcessMemory", [] { sceDebugWriteProcessMemory(1, 0, 0, nullptr, nullptr); });
    RequireThrows("sceDebugWriteThreadRegister", [] { sceDebugWriteThreadRegister(1, SceDebugRegisterId::SCE_DBGP_REG_ID_RAX, SceDebugRegister{}); });
    std::puts("libmdbg_syscore pending exports throw");
    return 0;
}
