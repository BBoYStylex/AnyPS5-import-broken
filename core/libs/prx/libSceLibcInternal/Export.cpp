#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/HeapDiagnostics.hpp"
#include "prx/libc/include/HostThreadLocal.hpp"
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

extern "C" void APS5_VABI sceKernelSetThreadDtors(thread_dtors_func_t dtors);
extern "C" int APS5_VABI sceKernelGetModuleInfoFromAddr(std::uint64_t address, int flags, ModuleInfoEx* info);
extern "C" const char* __progname_nid_postfix;

namespace {

using ThreadDestructorFunction = void (APS5_VABI*)(void*);

struct ThreadDestructor {
    ThreadDestructorFunction function;
    void* object;
    void* dsoSymbol;
};

struct ThreadDestructorsTag {};

std::vector<ThreadDestructor>& ThreadDestructors() {
    return HostThreadLocal<std::vector<ThreadDestructor>, ThreadDestructorsTag>();
}

bool IsInLoadedImage(const void* address) {
    if (address == nullptr)
        return false;
#ifdef _WIN32
    HMODULE module = nullptr;
    return GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, static_cast<LPCSTR>(address), &module) != 0;
#else
    Dl_info info{};
    return dladdr(address, &info) != 0;
#endif
}

void CallThreadDestructor(const ThreadDestructor& destructor) {
    const auto* function = reinterpret_cast<const void*>(destructor.function);
    if (!IsInLoadedImage(function)) {
        std::ostringstream message;
        message << "thread_local destructor " << function << " of dso " << destructor.dsoSymbol << " is not in a loaded image";
        throw std::runtime_error(message.str());
    }
    destructor.function(destructor.object);
}

void APS5_VABI RunThreadDestructors_nid_no_patch() {
    auto& destructors = ThreadDestructors();
    while (!destructors.empty()) {
        const ThreadDestructor destructor = destructors.back();
        destructors.pop_back();
        CallThreadDestructor(destructor);
    }
}

bool FindDsoModule(const void* dsoSymbol, KernelModule& handle) {
    ModuleInfoEx info{};
    info.st_size = sizeof(ModuleInfoEx);
    if (sceKernelGetModuleInfoFromAddr(reinterpret_cast<std::uintptr_t>(dsoSymbol), 2, &info) != 0) {
        handle = 0;
        return false;
    }
    handle = info.id;
    return true;
}

bool ForceThreadDestructorPass(KernelModule handle) {
    auto& destructors = ThreadDestructors();
    bool found = false;
    for (std::size_t index = destructors.size(); index-- > 0;) {
        const ThreadDestructor destructor = destructors[index];
        KernelModule module = 0;
        const bool loaded = FindDsoModule(destructor.dsoSymbol, module);
        if (module != handle)
            continue;
        found = true;
        destructors.erase(destructors.begin() + static_cast<std::ptrdiff_t>(index));
        if (loaded && *static_cast<void* const*>(destructor.dsoSymbol) == destructor.dsoSymbol)
            CallThreadDestructor(destructor);
    }
    return found;
}

void RegisterThreadExitHook() {
    [[maybe_unused]] static const bool registered = [] {
        sceKernelSetThreadDtors(RunThreadDestructors_nid_no_patch);
        return true;
    }();
}

}

extern "C" {

int Need_sceLibcInternal_nid_postfix = 1;

const char* APS5_VABI getprogname_nid_postfix() {
    return __progname_nid_postfix;
}

void APS5_VABI __cxa_finalize_nid_postfix(void* dsoHandle) {
    CxaFinalize_nid_no_patch(dsoHandle);
}

void APS5_VABI sceLibcHeapGetTraceInfo_nid_postfix(Info* info) {
    LibcHeapTraceInfo_nid_no_patch(info);
}

int APS5_VABI _sceLibcInternalThreadAtexit_nid_postfix(ThreadDestructorFunction destructor, void* object, void* dsoSymbol) {
    RegisterThreadExitHook();
    ThreadDestructors().push_back({destructor, object, dsoSymbol});
    return 0;
}

void APS5_VABI _sceLibcInternalThreadDtors_nid_postfix() {
    RunThreadDestructors_nid_no_patch();
}

int APS5_VABI _sceLibcInternalForceTlsDestructor_nid_postfix(KernelModule handle) {
    for (int pass = 0; pass < 4; ++pass) {
        if (!ForceThreadDestructorPass(handle))
            break;
    }
    return 0;
}

unsigned int APS5_VABI GuestAlarm_nid_no_patch(unsigned int seconds);

unsigned int APS5_VABI alarm_nid_postfix(unsigned int seconds) {
    return GuestAlarm_nid_no_patch(seconds);
}


int APS5_VABI sceLibcInternalUnknown_FHErahnajkw_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Atan", sceLibcInternalUnknown_FHErahnajkw_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_M6nCy6H8Hs4_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Atqexit", sceLibcInternalUnknown_M6nCy6H8Hs4_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_IHiK3lL7CvI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Atthreadexit", sceLibcInternalUnknown_IHiK3lL7CvI_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_aMucxariNg8_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Btowc", sceLibcInternalUnknown_aMucxariNg8_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_ykNF6P3ZsdA_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_CStrftime", sceLibcInternalUnknown_ykNF6P3ZsdA_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_we_vQBAugV8_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_CStrxfrm", sceLibcInternalUnknown_we_vQBAugV8_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_6aEXAPYpaEA_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_CWcsxfrm", sceLibcInternalUnknown_6aEXAPYpaEA_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_G1kDk_5L6dU_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Call_onceEx", sceLibcInternalUnknown_G1kDk_5L6dU_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_mgNGxmJltOY_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Closreg", sceLibcInternalUnknown_mgNGxmJltOY_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_vyLotuB6AS4_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Cnd_do_broadcast_at_thread_exit", sceLibcInternalUnknown_vyLotuB6AS4_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_DV2AdZFFEh8_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Cnd_register_at_thread_exit", sceLibcInternalUnknown_DV2AdZFFEh8_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_wpuIiVoCWcM_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Cnd_unregister_at_thread_exit", sceLibcInternalUnknown_wpuIiVoCWcM_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_ZlsoRa7pcuI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Daysto", sceLibcInternalUnknown_ZlsoRa7pcuI_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_H0FQnSWp1es_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Deletegloballocale", sceLibcInternalUnknown_H0FQnSWp1es_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_VGhcd0QwhhY_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Divide", sceLibcInternalUnknown_VGhcd0QwhhY_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_4QwxZ3U0OK0_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Do_call", sceLibcInternalUnknown_4QwxZ3U0OK0_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_vCQLavj_3CM_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Dtento", sceLibcInternalUnknown_vCQLavj_3CM_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_aQURHgjHo_w_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Erfc", sceLibcInternalUnknown_aQURHgjHo_w_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_yL91YD_WTBc_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Exp", sceLibcInternalUnknown_yL91YD_WTBc_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_us3bDnDzd70_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FAtan", sceLibcInternalUnknown_us3bDnDzd70_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_fuzzBVdpRG0_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FDivide", sceLibcInternalUnknown_fuzzBVdpRG0_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_SoNnx4Ejxw8_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FDtento", sceLibcInternalUnknown_SoNnx4Ejxw8_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_qwR1gtp_WS0_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FErfc", sceLibcInternalUnknown_qwR1gtp_WS0_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown__3pU5y1utmI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FExp", sceLibcInternalUnknown__3pU5y1utmI_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_DS03EjPDtFo_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FHypot", sceLibcInternalUnknown_DS03EjPDtFo_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_OAE3YU396YQ_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FLogpoly", sceLibcInternalUnknown_OAE3YU396YQ_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_cfpRP3h9F6o_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FPlsw", sceLibcInternalUnknown_cfpRP3h9F6o_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_IdWhZ0SM7JA_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FPmsw", sceLibcInternalUnknown_IdWhZ0SM7JA_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_A98W3Iad6xE_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FPow", sceLibcInternalUnknown_A98W3Iad6xE_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_y9Ur6T0A0p8_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FQuad", sceLibcInternalUnknown_y9Ur6T0A0p8_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_PDQbEFcV4h0_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FQuadph", sceLibcInternalUnknown_PDQbEFcV4h0_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_lP9zfrhtpBc_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FRecip", sceLibcInternalUnknown_lP9zfrhtpBc_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_TLvAYmLtdVw_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FRint", sceLibcInternalUnknown_TLvAYmLtdVw_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_fQ_SWrQUQBg_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FSincos", sceLibcInternalUnknown_fQ_SWrQUQBg_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_iBrTJkDlQv8_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FTan", sceLibcInternalUnknown_iBrTJkDlQv8_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_odPHnVL_rFg_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FTgamma", sceLibcInternalUnknown_odPHnVL_rFg_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_1EyHxzcz6AM_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FXp_addh", sceLibcInternalUnknown_1EyHxzcz6AM_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_1b_IhPTX0nk_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FXp_addx", sceLibcInternalUnknown_1b_IhPTX0nk_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_e1y7KVAySrY_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FXp_getw", sceLibcInternalUnknown_e1y7KVAySrY_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_OVqW4uElSrc_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FXp_invx", sceLibcInternalUnknown_OVqW4uElSrc_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_7GgGIxmwA6I_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FXp_ldexpx", sceLibcInternalUnknown_7GgGIxmwA6I_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_DVZmEd0ipSg_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FXp_movx", sceLibcInternalUnknown_DVZmEd0ipSg_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_W_lrIwAQVUk_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FXp_mulh", sceLibcInternalUnknown_W_lrIwAQVUk_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_o1B4dkvesMc_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FXp_mulx", sceLibcInternalUnknown_o1B4dkvesMc_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_ikHTMeh60B0_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FXp_setn", sceLibcInternalUnknown_ikHTMeh60B0_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_5zWUVRtR8xg_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FXp_setw", sceLibcInternalUnknown_5zWUVRtR8xg_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_pNWIpeE5Wv4_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FXp_sqrtx", sceLibcInternalUnknown_pNWIpeE5Wv4_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_HD9vSXqj6zI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_FXp_subx", sceLibcInternalUnknown_HD9vSXqj6zI_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_chzmnjxxVtk_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fac_tidy", sceLibcInternalUnknown_chzmnjxxVtk_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_D_fkILS7EK0_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fail_s", sceLibcInternalUnknown_D_fkILS7EK0_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_JbQw6W62UwI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fetch_add_8", sceLibcInternalUnknown_JbQw6W62UwI_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_pxFnS1okTFE_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fetch_and_8", sceLibcInternalUnknown_pxFnS1okTFE_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_zQQIrnpCoFA_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fetch_and_seq_cst_1", sceLibcInternalUnknown_zQQIrnpCoFA_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_xROUuk7ItMM_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fetch_and_seq_cst_2", sceLibcInternalUnknown_xROUuk7ItMM_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_jQuruQuMlyo_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fetch_and_seq_cst_4", sceLibcInternalUnknown_jQuruQuMlyo_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_ixWEOmOBavk_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fetch_or_8", sceLibcInternalUnknown_ixWEOmOBavk_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_2_6K_2tWaok_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fetch_or_seq_cst_1", sceLibcInternalUnknown_2_6K_2tWaok_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown__egu08GJ0lw_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fetch_or_seq_cst_2", sceLibcInternalUnknown__egu08GJ0lw_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_gI9un1H_fZk_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fetch_or_seq_cst_4", sceLibcInternalUnknown_gI9un1H_fZk_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_YhaOeniKcoA_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fetch_xor_8", sceLibcInternalUnknown_YhaOeniKcoA_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_E2YhT7m79kM_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fetch_xor_seq_cst_1", sceLibcInternalUnknown_E2YhT7m79kM_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_fgXJvOSrqfg_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fetch_xor_seq_cst_2", sceLibcInternalUnknown_fgXJvOSrqfg_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_cqcY17uV3dI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fetch_xor_seq_cst_4", sceLibcInternalUnknown_cqcY17uV3dI_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_dYJJbxnyb74_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fgpos", sceLibcInternalUnknown_dYJJbxnyb74_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_xGT4Mc55ViQ_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fofind", sceLibcInternalUnknown_xGT4Mc55ViQ_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_jVDuvE3s5Bs_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fofree", sceLibcInternalUnknown_jVDuvE3s5Bs_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_sQL8D_jio7U_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fopen", sceLibcInternalUnknown_sQL8D_jio7U_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_dREVnZkAKRE_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Foprep", sceLibcInternalUnknown_dREVnZkAKRE_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_9s3P_LCvWP8_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Frprep", sceLibcInternalUnknown_9s3P_LCvWP8_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_A_Y3xfrWLLo_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fspos", sceLibcInternalUnknown_A_Y3xfrWLLo_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_4F9pQjbh8R8_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Fwprep", sceLibcInternalUnknown_4F9pQjbh8R8_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_vakoyx9nkqo_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Genld", sceLibcInternalUnknown_vakoyx9nkqo_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_bRN9BzEkm4o_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Gentime", sceLibcInternalUnknown_bRN9BzEkm4o_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_2MfMa3456FI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Getcloc", sceLibcInternalUnknown_2MfMa3456FI_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_i1N28hWcD_4_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Getctyptab", sceLibcInternalUnknown_i1N28hWcD_4_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_Jcu0Wl1_XbE_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Getdst", sceLibcInternalUnknown_Jcu0Wl1_XbE_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_M1xC101lsIU_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Geterrno", sceLibcInternalUnknown_M1xC101lsIU_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_bItEABINEm0_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Getfld", sceLibcInternalUnknown_bItEABINEm0_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_7iFNNuNyXxw_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Getfloat", sceLibcInternalUnknown_7iFNNuNyXxw_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_8Jr4cvRM6EM_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Getgloballocale", sceLibcInternalUnknown_8Jr4cvRM6EM_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_PWmDp8ZTS9k_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Getint", sceLibcInternalUnknown_PWmDp8ZTS9k_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_bF4eWOM5ouo_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Getpcostate", sceLibcInternalUnknown_bF4eWOM5ouo_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_8xXiEPby8h8_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Getptimes", sceLibcInternalUnknown_8xXiEPby8h8_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_hzsdjKbFD7g_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Getpwcostate", sceLibcInternalUnknown_hzsdjKbFD7g_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_RLdcWoBjmT4_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Getpwctrtab", sceLibcInternalUnknown_RLdcWoBjmT4_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_uF8hDs1CqWI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Getpwctytab", sceLibcInternalUnknown_uF8hDs1CqWI_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_i2yN6xBwooo_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Getstr", sceLibcInternalUnknown_i2yN6xBwooo_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_g8ozp2Zrsj8_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Gettime", sceLibcInternalUnknown_g8ozp2Zrsj8_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_Wz9CvcF5jn4_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Getzone", sceLibcInternalUnknown_Wz9CvcF5jn4_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_wUa_oPQvFZ0_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Hypot", sceLibcInternalUnknown_wUa_oPQvFZ0_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_Sb26PiOiFtE_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Isdst", sceLibcInternalUnknown_Sb26PiOiFtE_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_2Aw366Jn07s_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LAtan", sceLibcInternalUnknown_2Aw366Jn07s_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_moDSeLQGJFQ_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LCosh", sceLibcInternalUnknown_moDSeLQGJFQ_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_HPGLb8Qo6as_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LDivide", sceLibcInternalUnknown_HPGLb8Qo6as_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_PEU_SAfo5_0_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LDtento", sceLibcInternalUnknown_PEU_SAfo5_0_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_PG4HVe4X_Oc_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LErfc", sceLibcInternalUnknown_PG4HVe4X_Oc_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_se3uU7lRMHY_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LExp", sceLibcInternalUnknown_se3uU7lRMHY_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_IRUFZywbTT0_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LHypot", sceLibcInternalUnknown_IRUFZywbTT0_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_4iFgTDd9NFs_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LLog", sceLibcInternalUnknown_4iFgTDd9NFs_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_niPixjs0l2w_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LLogpoly", sceLibcInternalUnknown_niPixjs0l2w_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_W_T_amhWrkA_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LPlsw", sceLibcInternalUnknown_W_T_amhWrkA_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_gso5X06CFkI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LPmsw", sceLibcInternalUnknown_gso5X06CFkI_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_3Z8jD_HTKr8_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LPow", sceLibcInternalUnknown_3Z8jD_HTKr8_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_fXCPTDS0tD0_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LQuad", sceLibcInternalUnknown_fXCPTDS0tD0_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_CbRhl_RoYEM_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LQuadph", sceLibcInternalUnknown_CbRhl_RoYEM_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_XrXTtRA7U08_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LRecip", sceLibcInternalUnknown_XrXTtRA7U08_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_fJmOr59K8QY_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LRint", sceLibcInternalUnknown_fJmOr59K8QY_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_g3dK2qlzwnM_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LSincos", sceLibcInternalUnknown_g3dK2qlzwnM_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_ZWy6IcBqs1c_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LSinh", sceLibcInternalUnknown_ZWy6IcBqs1c_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_mM4OblD9xWM_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LTan", sceLibcInternalUnknown_mM4OblD9xWM_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_jq4Srfnz9K8_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LTgamma", sceLibcInternalUnknown_jq4Srfnz9K8_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_QEr_PxGUoic_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LXp_addh", sceLibcInternalUnknown_QEr_PxGUoic_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_BuP7bDPGEcQ_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LXp_addx", sceLibcInternalUnknown_BuP7bDPGEcQ_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_TnublTBYXTI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LXp_getw", sceLibcInternalUnknown_TnublTBYXTI_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_FAreWopdBvE_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LXp_invx", sceLibcInternalUnknown_FAreWopdBvE_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_7O_vjsHecbY_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LXp_ldexpx", sceLibcInternalUnknown_7O_vjsHecbY_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_wlEdSSqaz_E_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LXp_movx", sceLibcInternalUnknown_wlEdSSqaz_E_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_riets0BFHMY_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LXp_mulh", sceLibcInternalUnknown_riets0BFHMY_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_LbLiLA2biaI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LXp_mulx", sceLibcInternalUnknown_LbLiLA2biaI_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_hjDoZ6qbkmQ_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LXp_setn", sceLibcInternalUnknown_hjDoZ6qbkmQ_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_kHVXc_AWbMU_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LXp_setw", sceLibcInternalUnknown_kHVXc_AWbMU_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_IPjwywTNR8w_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LXp_sqrtx", sceLibcInternalUnknown_IPjwywTNR8w_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_YCVxOE0lHOI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_LXp_subx", sceLibcInternalUnknown_YCVxOE0lHOI_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_3BbBNPjfkYI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Ldtob", sceLibcInternalUnknown_3BbBNPjfkYI_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown__GxvfSLhoAo_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Litob", sceLibcInternalUnknown__GxvfSLhoAo_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_Cv_8x__GS9A_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Lock_spin_lock", sceLibcInternalUnknown_Cv_8x__GS9A_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_sYz_SxY8H6M_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Locsum", sceLibcInternalUnknown_sYz_SxY8H6M_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_4ifo9QGrO5w_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Locterm", sceLibcInternalUnknown_4ifo9QGrO5w_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_ElU3kEY8q_I_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Locvar", sceLibcInternalUnknown_ElU3kEY8q_I_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_bqcStLRGIXw_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Logpoly", sceLibcInternalUnknown_bqcStLRGIXw_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_tTDqwhYbUUo_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Makeloc", sceLibcInternalUnknown_tTDqwhYbUUo_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_AnTtuRUE1cE_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Makestab", sceLibcInternalUnknown_AnTtuRUE1cE_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_QalEczzSNqc_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Makewct", sceLibcInternalUnknown_QalEczzSNqc_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_sij3OtJpHFc_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Mbtowc", sceLibcInternalUnknown_sij3OtJpHFc_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_hPzYSd5Nasc_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Mtx_timedlock", sceLibcInternalUnknown_hPzYSd5Nasc_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_LaPaA6mYA38_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Mtxdst", sceLibcInternalUnknown_LaPaA6mYA38_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_z7STeF6abuU_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Mtxinit", sceLibcInternalUnknown_z7STeF6abuU_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_pE4Ot3CffW0_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Mtxlock", sceLibcInternalUnknown_pE4Ot3CffW0_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_cMwgSSmpE5o_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Mtxunlock", sceLibcInternalUnknown_cMwgSSmpE5o_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_Ss3108pBuZY_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Nnl", sceLibcInternalUnknown_Ss3108pBuZY_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_1hGmBh83dL8_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Plsw", sceLibcInternalUnknown_1hGmBh83dL8_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_hrv1BgM68kU_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Pmsw", sceLibcInternalUnknown_hrv1BgM68kU_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_ECh_p_LRG6Y_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Pow", sceLibcInternalUnknown_ECh_p_LRG6Y_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_FModQzwn1_4_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Printf", sceLibcInternalUnknown_FModQzwn1_4_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_rnxaQ_2hSMI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Putfld", sceLibcInternalUnknown_rnxaQ_2hSMI_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_Fot_m6M2oKE_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Putstr", sceLibcInternalUnknown_Fot_m6M2oKE_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_ijAqq39g4dI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Puttxt", sceLibcInternalUnknown_ijAqq39g4dI_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_TQPr_IeNIS0_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Quad", sceLibcInternalUnknown_TQPr_IeNIS0_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_VecRKuKdXdo_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Quadph", sceLibcInternalUnknown_VecRKuKdXdo_nid_no_patch);

int APS5_VABI sceLibcInternalUnknown_MxZ4Lc35Rig_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("_Readloc", sceLibcInternalUnknown_MxZ4Lc35Rig_nid_no_patch);
}
