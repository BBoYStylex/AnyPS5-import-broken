#include <cstdint>
#include <cstddef>
#include <type_traits>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI _accept(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_accept), int (APS5_VABI*)(void)>);

int APS5_VABI _bind(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_bind), int (APS5_VABI*)(void)>);

int APS5_VABI _connect(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_connect), int (APS5_VABI*)(void)>);

int APS5_VABI _fpathconf(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_fpathconf), int (APS5_VABI*)(void)>);

int APS5_VABI _getdirentries(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_getdirentries), int (APS5_VABI*)(void)>);

int APS5_VABI _getpeername(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_getpeername), int (APS5_VABI*)(void)>);

int APS5_VABI _getsockname(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_getsockname), int (APS5_VABI*)(void)>);

int APS5_VABI _getsockopt(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_getsockopt), int (APS5_VABI*)(void)>);

int APS5_VABI _listen(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_listen), int (APS5_VABI*)(void)>);

int APS5_VABI _openat(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_openat), int (APS5_VABI*)(void)>);

int APS5_VABI _recvfrom(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_recvfrom), int (APS5_VABI*)(void)>);

int APS5_VABI _recvmsg(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_recvmsg), int (APS5_VABI*)(void)>);

int APS5_VABI _sendmsg(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_sendmsg), int (APS5_VABI*)(void)>);

int APS5_VABI _sendto(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_sendto), int (APS5_VABI*)(void)>);

int APS5_VABI _setsockopt(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_setsockopt), int (APS5_VABI*)(void)>);

int APS5_VABI _sigaction(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_sigaction), int (APS5_VABI*)(void)>);

int APS5_VABI _sigsuspend(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_sigsuspend), int (APS5_VABI*)(void)>);

int APS5_VABI pthread_cond_signalto_np(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&pthread_cond_signalto_np), int (APS5_VABI*)(void)>);

int APS5_VABI pthread_condattr_getclock_nid_postfix(void) {
    NotImplemented_nid_no_patch("pthread_condattr_getclock");
    return 0;
}
static_assert(std::is_same_v<decltype(&pthread_condattr_getclock_nid_postfix), int (APS5_VABI*)(void)>);

int APS5_VABI pthread_mutexattr_getkind_np(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&pthread_mutexattr_getkind_np), int (APS5_VABI*)(void)>);

int APS5_VABI pthread_mutexattr_setkind_np(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&pthread_mutexattr_setkind_np), int (APS5_VABI*)(void)>);

int APS5_VABI pthread_rwlockattr_gettype_np(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&pthread_rwlockattr_gettype_np), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelCallIndirectBuffer3(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelCallIndirectBuffer3), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelGetCpuCoreClock(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelGetCpuCoreClock), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelGetGenuineProsperoSystemSwVersion(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelGetGenuineProsperoSystemSwVersion), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelGetProsperoCompiledSdkVersion(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelGetProsperoCompiledSdkVersion), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelGetProsperoCompiledSdkVersionByPid(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelGetProsperoCompiledSdkVersionByPid), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelGetPs4CompiledSdkVersion(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelGetPs4CompiledSdkVersion), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelGetPs4CompiledSdkVersionByPid(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelGetPs4CompiledSdkVersionByPid), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelGetToolMemoryRange(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelGetToolMemoryRange), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelIccGetBDDriveChuckingState(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelIccGetBDDriveChuckingState), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelIccSetPriorityBuzzer(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelIccSetPriorityBuzzer), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelMapperGetUsageStatsData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelMapperGetUsageStatsData), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelMapperSetPageTablePoolOccupancyNotificationThreshold(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelMapperSetPageTablePoolOccupancyNotificationThreshold), int (APS5_VABI*)(void)>);

int APS5_VABI scePthreadRegisterThreadDeathHook(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&scePthreadRegisterThreadDeathHook), int (APS5_VABI*)(void)>);

}
