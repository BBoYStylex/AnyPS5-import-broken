#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr int ULOBJMGR_EINVAL = 22;
constexpr std::uint32_t ULOBJMGR_ID_LIMIT = 0x4000;

struct PendingModuleArgs {
    std::size_t args = 0;
    const void* argp = nullptr;
};
thread_local PendingModuleArgs pendingModuleArgs;
thread_local int pendingModuleInitResult = 0;

}

extern "C" {

int APS5_VABI _sceUlobjmgrRegisterObject(std::uint64_t object, std::int32_t kind, std::uint32_t* id) {
 if (object == 0 || kind == 0 || id == nullptr) return ULOBJMGR_EINVAL;
 *id = 0;
 return 0;
}

int APS5_VABI _sceUlobjmgrUnregisterObject(std::uint32_t id) {
 if (id >= ULOBJMGR_ID_LIMIT) return ULOBJMGR_EINVAL;
 return 0;
}

const void* __aps5_get_pending_module_args_nid_no_patch() {
    return &pendingModuleArgs;
}

void __aps5_set_module_init_result_nid_no_patch(int result) {
    pendingModuleInitResult = result;
}

}
