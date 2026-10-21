#include "prx/libmdbg_syscore/Mdbg.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceDebugAcquireAndUpdateDebugRegister() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugAttachProcess(int pid) {
    (void)pid;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugCancelCoredump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugClearStepThread(int tid) {
    (void)tid;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugCreatePerfScratchDataArea() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugCreatePerfScratchExecutableArea() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugCreateScratchDataArea() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugCreateScratchDataAreaForPrx() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugCreateScratchExecutableArea(int pid, std::size_t size, std::uint64_t* addressOut) {
    (void)pid;
    (void)size;
    (void)addressOut;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugCreateScratchExecutableAreaForPrx(int pid, std::size_t size, const char* name, std::uint64_t* addressOut) {
    (void)pid;
    (void)size;
    (void)name;
    (void)addressOut;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugDestroyPerfScratchDataArea() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugDestroyPerfScratchExecutableArea() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugDestroyScratchDataArea() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugDestroyScratchExecutableArea(int pid, std::uint64_t address, std::size_t size) {
    (void)pid;
    (void)address;
    (void)size;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugDetachProcess(int pid) {
    (void)pid;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetApplicationIdByTitleId(char* titleId, int* appId) {
    (void)titleId;
    (void)appId;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetApplicationInfo(int appId, SceDebugAppInfo* info) {
    (void)appId;
    (void)info;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetApplicationList(int* appIds, int maxAppIds, int* actualAppIds) {
    (void)appIds;
    (void)maxAppIds;
    (void)actualAppIds;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetCrashInfoDetailForCoredump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetCrashInfoForCoredump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetDLLoadFlag() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetDataListOfUltQueue() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetDebugRegisterStatusMap() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetEventList(int pid, std::uint64_t sid, SceKernelEvent* dataPtr, std::uint64_t a4, std::size_t** actualEvents) {
    (void)pid;
    (void)sid;
    (void)dataPtr;
    (void)a4;
    (void)actualEvents;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetEventListForEQueueFd() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetEventSubscriptionList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetEventSubscriptionListForEQueueFd() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetFiberInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetFileInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetFileInfoForCoredump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetFileList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetFileListForCoredump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetGpuInfoArea() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetJobManagerInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetJobManagerSequenceInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetJobManagerSequenceList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetModuleInfo(int pid, SceDebugModuleId moduleId, SceDebugModuleInfo* moduleInfo) {
    (void)pid;
    (void)moduleId;
    (void)moduleInfo;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetModuleList(int pid, SceDebugModuleId* moduleIdList, int moduleIdListSize, int* moduleIdCount) {
    (void)pid;
    (void)moduleIdList;
    (void)moduleIdListSize;
    (void)moduleIdCount;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetModuleMetaData(int pid, SceDebugModuleId moduleId, void* metaData, std::size_t metaDataSize, std::size_t* actualMetaData) {
    (void)pid;
    (void)moduleId;
    (void)metaData;
    (void)metaDataSize;
    (void)actualMetaData;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetMonoVMInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetMonoVMList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetProcessCoredumpHandlerEventBlob() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetProcessCoredumpHandlerEventInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetProcessCoredumpHandlerResult() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetProcessEventCntlFlag() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetProcessInfo(int pid, SceDebugProcessInfo* processInfo) {
    (void)pid;
    (void)processInfo;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetProcessList(int* pidList, int pidListSize, int* pidCount) {
    (void)pidList;
    (void)pidListSize;
    (void)pidCount;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetProcessPropertyForCoredump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetProcessResourceStatCount() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetProcessResourceStatData() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetProcessTime() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetProcessTimeCounter() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetSyncExclusiveWaiterList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetSyncObjectData() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetSyncObjectList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetSyncWaiterList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetSyncWaiterListForEQueueFd() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetSystemStatusBlob() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetSystemStatusCount() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetThreadInfo(int pid, int tid, SceDebugThreadInfo* info) {
    (void)pid;
    (void)tid;
    (void)info;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetThreadInfoByIdent() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetThreadList(int pid, int* tidList, int tidListSize, int* tidCount) {
    (void)pid;
    (void)tidList;
    (void)tidListSize;
    (void)tidCount;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetThreadListAsIdent() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetUlObjectList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetUltCondvarInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetUltInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetUltListOfUltRuntime() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetUltMutexInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetUltQueueDataResourcePoolInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetUltQueueInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetUltRuntimeInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetUltRwlockInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetUltSemaphoreInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetUltWaitingQueueResourcePoolInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetVirtualMemoryDetailInfoForCoredump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetVirtualMemoryInfo(int pid, std::uint64_t address, SceDebugVirtualQueryInfo* info) {
    (void)pid;
    (void)address;
    (void)info;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetVirtualMemoryInfoForCoredump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetWaitingListOfUltCondvar() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetWaitingListOfUltMutex() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetWaitingListOfUltQueue() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetWaitingListOfUltRwlock() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetWaitingListOfUltSemaphore() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugGetWorkerThreadListOfUltRuntime() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugInit(int a1) {
    (void)a1;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugInitForCoredump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugInitForTest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetBlockTimeInfoList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetBlockedIpcInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetChannelInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetChannelKidList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetChannelWaitingThreadList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetClientInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetClientKidList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetClientKidListByDump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetClientKidListByServerKid() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetConnectRequestInfoList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetConnectionInfoList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetConnectionWaitingThreadListByClientKid() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetConnectionWaitingThreadListBySessionKid() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetDump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetEncryptedInfoAllForCoredump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetKidInfoListForCoredump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetServerDispatchInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetServerInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetServerKidList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetServerKidListByDump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetServerWaitingThreadList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetSessionInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetSessionKidList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetSessionKidListByServerKid() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugIpmiGetTidListByDump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugKillApplication(int context, int appid) {
    (void)context;
    (void)appid;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugKillProcess(int pid) {
    (void)pid;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugNoStopChildProcesses(int pid) {
    (void)pid;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugNoStopOnDLLoad(int pid) {
    (void)pid;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugProcessSpawn(int context, void* argv, int flags, std::size_t stackSize, const char* workdir, const char* pathFile) {
    (void)context;
    (void)argv;
    (void)flags;
    (void)stackSize;
    (void)workdir;
    (void)pathFile;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugReadEvent() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugReadEventForTest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugReadProcessMemory(int pid, std::uint64_t addrss, std::size_t size, std::uint8_t* data, std::size_t* sizeRead) {
    (void)pid;
    (void)addrss;
    (void)size;
    (void)data;
    (void)sizeRead;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugReadProcessMemoryForSDBGP() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugReadProcessRegister() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugReadProcessRegisterForSDBGP() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugReadProcessResourceStatData() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugReadThreadRegister(int tid, SceDebugRegisterId reg, SceDebugRegister* value) {
    (void)tid;
    (void)reg;
    (void)value;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugReadThreadRegisterForSDBGP() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugReleaseDebugRegister() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugResumeApplication(int context, int appId) {
    (void)context;
    (void)appId;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugResumeProcess(int pid, int a2) {
    (void)pid;
    (void)a2;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugResumeThread(int tid) {
    (void)tid;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugSetProcessEventCntlFlag() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugSetStepThread(int tid) {
    (void)tid;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugSpawnApplication(int context, char* titleId, void* argv, int flags, int mountFlags) {
    (void)context;
    (void)titleId;
    (void)argv;
    (void)flags;
    (void)mountFlags;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugStopChildProcesses(int pid) {
    (void)pid;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugStopOnDLLoad(int pid) {
    (void)pid;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugSuspendApplication(int context, int appid) {
    (void)context;
    (void)appid;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugSuspendProcess(int pid) {
    (void)pid;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugSuspendThread(int tid) {
    (void)tid;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugTriggerCoredump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugTriggerCoredumpForSystem() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugTriggerVrCaptureDump() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugWriteProcessMemory(int pid, std::uint64_t address, std::size_t size, std::uint8_t* data, std::size_t* sizeWritten) {
    (void)pid;
    (void)address;
    (void)size;
    (void)data;
    (void)sizeWritten;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugWriteProcessRegister() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDebugWriteThreadRegister(int tid, SceDebugRegisterId reg, SceDebugRegister value) {
    (void)tid;
    (void)reg;
    (void)value;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
