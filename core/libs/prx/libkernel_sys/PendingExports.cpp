#include <cstdint>
#include <cstddef>
#include <type_traits>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI __getcwd(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__getcwd), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_nanosleep(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_nanosleep), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_resume_internal_hdd(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_resume_internal_hdd), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_resume_process(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_resume_process), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_suspend_process(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_suspend_process), int (APS5_VABI*)(void)>);

int APS5_VABI fpathconf_nid_postfix(void) {
    NotImplemented_nid_no_patch("fpathconf");
    return 0;
}
static_assert(std::is_same_v<decltype(&fpathconf_nid_postfix), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelGetAppInfoListForMinisyscore(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelGetAppInfoListForMinisyscore), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelGetNandInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelGetNandInfo), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelGetUpdatemode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelGetUpdatemode), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelIccGetEmcBtDownload(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelIccGetEmcBtDownload), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelIccGetFloydVersion(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelIccGetFloydVersion), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelIccGetWlanBtGpio(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelIccGetWlanBtGpio), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelIccIsThermalAlert(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelIccIsThermalAlert), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelIccSetBootTimestamp(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelIccSetBootTimestamp), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelIccSetEmcBtDownload(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelIccSetEmcBtDownload), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelIccSetWlanBtGpio(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelIccSetWlanBtGpio), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelIccSetWlanBtManuTestMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelIccSetWlanBtManuTestMode), int (APS5_VABI*)(void)>);

int APS5_VABI setpgid_nid_postfix(void) {
    NotImplemented_nid_no_patch("setpgid");
    return 0;
}
static_assert(std::is_same_v<decltype(&setpgid_nid_postfix), int (APS5_VABI*)(void)>);

}
