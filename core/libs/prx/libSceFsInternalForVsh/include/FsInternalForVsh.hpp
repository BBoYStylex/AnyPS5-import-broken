#ifndef CORE_LIBS_PRX_LIBSCEFSINTERNALFORVSH_INCLUDE_FSINTERNALFORVSH_HPP
#define CORE_LIBS_PRX_LIBSCEFSINTERNALFORVSH_INCLUDE_FSINTERNALFORVSH_HPP

#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>

extern "C" {

int APS5_VABI sceFsInitMountSaveDataOpt(void* opt);
int APS5_VABI sceFsInitUmountSaveDataOpt(void* opt);
int APS5_VABI sceFsMountSaveData(void* opt, const char* volumePath, const char* mountPath, std::uint8_t* key);
int APS5_VABI sceFsUmountSaveData(void* opt, const char* mountPath, int handle, int ignoreErrors);

}

#endif
