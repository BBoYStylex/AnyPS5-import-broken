#ifndef CORE_LIBS_PRX_LIBSCEREGMR_INCLUDE_REGMGR_HPP
#define CORE_LIBS_PRX_LIBSCEREGMR_INCLUDE_REGMGR_HPP

#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstddef>
#include <cstdint>

extern "C" {

int APS5_VABI sceRegMgrGetBin(std::int32_t optionId, void* outBinary, std::size_t binaryLength);
int APS5_VABI sceRegMgrSetBin(std::int32_t optionId, const void* inBinary, std::size_t binaryLength);

}

#endif
