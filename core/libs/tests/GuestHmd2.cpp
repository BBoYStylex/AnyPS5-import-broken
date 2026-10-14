#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" {
std::int32_t APS5_VABI sceHmd2Initialize(const void* param);
int APS5_VABI sceHmd2Open();
std::int32_t APS5_VABI sceHmd2GetDeviceInformation(void* info);
std::int32_t APS5_VABI sceHmd2GetFieldOfViewWithoutHandle(void* fieldOfView);
std::int32_t APS5_VABI sceHmd2ReprojectionEnableVrMode(std::uint64_t outputMode);
std::int32_t APS5_VABI sceHmd2ReprojectionSetTiming(std::int32_t timing);
}

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "HMD2: %s\n", message);
        std::abort();
    }
}

}

int main() {
    const std::uint8_t param[16]{};
    Require(sceHmd2Initialize(param) == static_cast<std::int32_t>(0x81110016), "initialization did not report the unsupported feature");
    Require(sceHmd2Initialize(nullptr) == static_cast<std::int32_t>(0x81110016), "initialization without a param did not report the unsupported feature");
    bool threw = false;
    try {
        sceHmd2Open();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "open after failed initialization did not throw");

    constexpr auto notInitialized = static_cast<std::int32_t>(0x8A720002);
    std::uint8_t information[28];
    for (auto& byte : information) byte = 0x5a;
    Require(sceHmd2GetDeviceInformation(information) == notInitialized, "device information did not report the uninitialized runtime");
    Require(sceHmd2GetDeviceInformation(nullptr) == notInitialized, "device information without an output did not report the uninitialized runtime");
    for (const auto byte : information) Require(byte == 0x5a, "device information wrote its output");
    float fieldOfView[4] = {-1.0f, -1.0f, -1.0f, -1.0f};
    Require(sceHmd2GetFieldOfViewWithoutHandle(fieldOfView) == notInitialized, "field of view did not report the uninitialized runtime");
    for (const float tangent : fieldOfView) Require(tangent == -1.0f, "field of view wrote its output");

    constexpr auto reprojectionNotInitialized = static_cast<std::int32_t>(0x8A72000B);
    Require(sceHmd2ReprojectionEnableVrMode(0x2000c) == reprojectionNotInitialized, "VR mode did not report the uninitialized reprojection");
    Require(sceHmd2ReprojectionEnableVrMode(0) == reprojectionNotInitialized, "an invalid VR mode did not report the uninitialized reprojection first");
    Require(sceHmd2ReprojectionSetTiming(3000) == reprojectionNotInitialized, "timing did not report the uninitialized reprojection");
    std::puts("HMD2 tests passed");
    return 0;
}
