#include <cstdint>
#include <cstddef>
#include <type_traits>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI __freeze(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__freeze), int (APS5_VABI*)(void)>);

int APS5_VABI __sceKernelGetGPI(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sceKernelGetGPI), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_debug_init(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_debug_init), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_dl_get_info(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_dl_get_info), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_dl_get_info_2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_dl_get_info_2), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_dl_get_list(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_dl_get_list), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_dl_get_metadata(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_dl_get_metadata), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_dynlib_get_info2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_dynlib_get_info2), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_dynlib_get_info_for_libdbg(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_dynlib_get_info_for_libdbg), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_dynlib_get_list2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_dynlib_get_list2), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_dynlib_get_list_for_libdbg(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_dynlib_get_list_for_libdbg), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_dynlib_load_prx(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_dynlib_load_prx), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_get_proc_type_info(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_get_proc_type_info), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_is_development_mode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_is_development_mode), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_kqueueex(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_kqueueex), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_namedobj_create(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_namedobj_create), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_namedobj_delete(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_namedobj_delete), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_netabort(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_netabort), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_netcontrol(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_netcontrol), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_netgetiflist(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_netgetiflist), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_netgetsockinfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_netgetsockinfo), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_opmc_disable(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_opmc_disable), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_opmc_enable(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_opmc_enable), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_opmc_get_ctr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_opmc_get_ctr), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_opmc_get_hw(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_opmc_get_hw), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_opmc_set_ctl(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_opmc_set_ctl), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_opmc_set_ctr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_opmc_set_ctr), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_opmc_set_hw(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_opmc_set_hw), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_osem_close(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_osem_close), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_osem_open(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_osem_open), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_randomized_path(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_randomized_path), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_rdup(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_rdup), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_regmgr_call(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_regmgr_call), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_set_uevt(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_set_uevt), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_socketclose(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_socketclose), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_test_debug_rwmem(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_test_debug_rwmem), int (APS5_VABI*)(void)>);

int APS5_VABI __sys_workaround8849(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&__sys_workaround8849), int (APS5_VABI*)(void)>);

int APS5_VABI _dup2_nid_postfix(void) {
    NotImplemented_nid_no_patch("_dup2");
    return 0;
}
static_assert(std::is_same_v<decltype(&_dup2_nid_postfix), int (APS5_VABI*)(void)>);

int APS5_VABI _execve_nid_postfix(void) {
    NotImplemented_nid_no_patch("_execve");
    return 0;
}
static_assert(std::is_same_v<decltype(&_execve_nid_postfix), int (APS5_VABI*)(void)>);

int APS5_VABI _execvpe_nid_postfix(void) {
    NotImplemented_nid_no_patch("_execvpe");
    return 0;
}
static_assert(std::is_same_v<decltype(&_execvpe_nid_postfix), int (APS5_VABI*)(void)>);

int APS5_VABI _wait4(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&_wait4), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelGetPrefixVersion(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelGetPrefixVersion), int (APS5_VABI*)(void)>);

int APS5_VABI sceKernelIccGetThermalAlert(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
static_assert(std::is_same_v<decltype(&sceKernelIccGetThermalAlert), int (APS5_VABI*)(void)>);

}
