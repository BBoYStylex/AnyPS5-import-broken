#ifndef CORE_LIBS_PRX_LIBSCREMOTEPLAY_INCLUDE_REMOTEPLAY_HPP
#define CORE_LIBS_PRX_LIBSCREMOTEPLAY_INCLUDE_REMOTEPLAY_HPP

#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>

extern "C" {

int APS5_VABI sceRemoteplayConfirmDeviceRegist(int* status, int* errorCode);
int APS5_VABI sceRemoteplayGeneratePinCode(std::uint32_t* pinCode);
int APS5_VABI sceRemoteplayNotifyPinCodeError(int errorCode);

}

#endif
