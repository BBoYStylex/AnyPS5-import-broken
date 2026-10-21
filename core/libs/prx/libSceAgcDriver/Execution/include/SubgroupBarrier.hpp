#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_SUBGROUPBARRIER_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_SUBGROUPBARRIER_HPP

#include <vulkan/vulkan.h>

namespace AgcDriver {

inline bool SubgroupControlBarrierUsable(VkDriverId driver) {
#ifdef __APPLE__
    // MoltenVK lowers a subgroup-scope control barrier to threadgroup_barrier, which waves skipping a branch never meet.
    if (driver == VK_DRIVER_ID_MOLTENVK) return false;
#endif
    return driver != VK_DRIVER_ID_AMD_PROPRIETARY;
}

}

#endif
