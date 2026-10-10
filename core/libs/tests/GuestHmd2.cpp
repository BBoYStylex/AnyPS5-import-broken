#include "prx/libSceHmd2/Hmd2.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <stdexcept>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "HMD2: %s\n", message);
        std::abort();
    }
}

template <typename TCall>
bool Throws(TCall call) {
    try {
        call();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

}

int main() {
    const std::uint8_t param[16]{};
    Require(sceHmd2Initialize(param) == Hmd2::RuntimeUnavailable, "initialization did not report the unavailable runtime");
    Require(sceHmd2Initialize(nullptr) == Hmd2::RuntimeUnavailable, "initialization without a param did not report the unavailable runtime");

    Hmd2::DeviceInformation info;
    std::memset(&info, 0xa5, sizeof(info));
    const auto untouchedInfo = info;
    Require(sceHmd2GetDeviceInformation(nullptr) == Hmd2::ParameterNull, "null device information");
    Require(sceHmd2GetDeviceInformation(&info) == Hmd2::NotInitialized, "device query after failed initialization");
    Require(std::memcmp(&info, &untouchedInfo, sizeof(info)) == 0, "failed device query modified output");

    Hmd2::FieldOfView fieldOfView;
    std::memset(&fieldOfView, 0xa5, sizeof(fieldOfView));
    const auto untouchedFieldOfView = fieldOfView;
    Require(sceHmd2GetFieldOfViewWithoutHandle(nullptr) == Hmd2::ParameterNull, "null field of view");
    Require(sceHmd2GetFieldOfViewWithoutHandle(&fieldOfView) == Hmd2::NotInitialized, "field of view after failed initialization");
    Require(std::memcmp(&fieldOfView, &untouchedFieldOfView, sizeof(fieldOfView)) == 0, "failed field of view query modified output");

    Require(sceHmd2Open(1, 0, 0, nullptr) == Hmd2::NotInitialized, "open after failed initialization");
    Require(sceHmd2Open(-1, 1, 1, reinterpret_cast<const Hmd2::OpenParam*>(param)) == Hmd2::NotInitialized,
            "open with invalid arguments did not report the uninitialized library");
    for (const auto handle : {0, -1, 1, 0x0f000000}) {
        Require(sceHmd2Close(handle) == Hmd2::NotInitialized, "close after failed initialization");
        Require(sceHmd2SetVibration(handle, 10) == Hmd2::NotInitialized, "vibration after failed initialization");
    }
    Require(sceHmd2SetVibration(0, -1) == Hmd2::NotInitialized, "vibration with an invalid strength did not report the uninitialized library");
    Require(sceHmd2Initialize(param) == Hmd2::RuntimeUnavailable, "failed calls created state");

    Require(Throws([] { sceHmd2ReprojectionInitialize(nullptr, nullptr); }), "reprojection initialization after failed initialization did not throw");
    Require(Throws([] { sceHmd2ReprojectionBeginFrame(1); }), "reprojection frame after failed initialization did not throw");
    std::puts("HMD2 tests passed");
    return 0;
}
