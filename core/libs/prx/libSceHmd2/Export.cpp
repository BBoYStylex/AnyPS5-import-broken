#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr std::int32_t SCE_HMD2_ERROR_UNSUPPORTED_FEATURE = static_cast<std::int32_t>(0x81110016);

[[noreturn]] void Unavailable(const char* function) {
    throw std::runtime_error(std::string(function) + ": PS VR2 runtime is not available");
}

}

extern "C" {

std::int32_t APS5_VABI sceHmd2Initialize(const void* param) {
    (void)param;
    return SCE_HMD2_ERROR_UNSUPPORTED_FEATURE;
}

int APS5_VABI sceHmd2Close() {
    Unavailable(__func__);
}

int APS5_VABI sceHmd2GazeGetResultForFoveatedRendering() {
    Unavailable(__func__);
}

int APS5_VABI sceHmd2GetDeviceInformation() {
    Unavailable(__func__);
}

int APS5_VABI sceHmd2GetFieldOfViewWithoutHandle() {
    Unavailable(__func__);
}

int APS5_VABI sceHmd2Open() {
    Unavailable(__func__);
}

int APS5_VABI sceHmd2ReprojectionBeginFrame() {
    Unavailable(__func__);
}

int APS5_VABI sceHmd2ReprojectionDisableVrMode() {
    Unavailable(__func__);
}

int APS5_VABI sceHmd2ReprojectionEnableVrMode() {
    Unavailable(__func__);
}

int APS5_VABI sceHmd2ReprojectionGetPredictedDisplayTime() {
    Unavailable(__func__);
}

int APS5_VABI sceHmd2ReprojectionInitialize() {
    Unavailable(__func__);
}

int APS5_VABI sceHmd2ReprojectionQueryBufferSizeAlign() {
    Unavailable(__func__);
}

int APS5_VABI sceHmd2ReprojectionQueryDisplayBufferSizeAlign() {
    Unavailable(__func__);
}

int APS5_VABI sceHmd2ReprojectionSetAllowPositionalReprojection() {
    Unavailable(__func__);
}

int APS5_VABI sceHmd2ReprojectionSetParamWithBuffer() {
    Unavailable(__func__);
}

int APS5_VABI sceHmd2ReprojectionSetRenderConfig() {
    Unavailable(__func__);
}

int APS5_VABI sceHmd2SetVibration() {
    Unavailable(__func__);
}

}
