#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceRemoteplay/include/Remoteplay.hpp"
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <type_traits>

static void Require(bool value) { if (!value) std::abort(); }

static_assert(std::is_same_v<decltype(&sceRemoteplayConfirmDeviceRegist), int (APS5_VABI *)(int*, int*)>);
static_assert(std::is_same_v<decltype(&sceRemoteplayGeneratePinCode), int (APS5_VABI *)(std::uint32_t*)>);
static_assert(std::is_same_v<decltype(&sceRemoteplayNotifyPinCodeError), int (APS5_VABI *)(int)>);

template <typename TCall>
static bool ReportsNotImplemented(const char* name, TCall call) {
    try {
        call();
    } catch (const std::runtime_error& error) {
        return std::string(error.what()) == std::string(name) + " not implemented";
    }
    return false;
}

int main() {
    std::uint32_t pinCode = 0;
    int status = 0;
    int errorCode = 0;
    Require(ReportsNotImplemented("sceRemoteplayGeneratePinCode", [&] { sceRemoteplayGeneratePinCode(&pinCode); }));
    Require(ReportsNotImplemented("sceRemoteplayConfirmDeviceRegist", [&] { sceRemoteplayConfirmDeviceRegist(&status, &errorCode); }));
    Require(ReportsNotImplemented("sceRemoteplayNotifyPinCodeError", [&] { sceRemoteplayNotifyPinCodeError(1); }));
    Require(pinCode == 0 && status == 0 && errorCode == 0);
    return 0;
}
