#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr std::int32_t SCE_HMD2_ERROR_UNSUPPORTED_FEATURE = static_cast<std::int32_t>(0x81110016);
constexpr std::int32_t SCE_HMD2_ERROR_NOT_INITIALIZED = static_cast<std::int32_t>(0x8A720002);
constexpr std::int32_t SCE_HMD2_ERROR_REPROJECTION_NOT_INITIALIZED = static_cast<std::int32_t>(0x8A72000B);

}

extern "C" {

int APS5_VABI sceHmd2ReprojectionQuerySeeThroughBufferSizeAlign() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceHmd2Initialize(const void* param) {
    (void)param;
    return SCE_HMD2_ERROR_UNSUPPORTED_FEATURE;
}

int APS5_VABI sceHmd2GetFieldOfView() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2GetDeviceInformationByHandle() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2Terminate() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2Close() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2GazeGetResult() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2GazeGetResultForFoveatedRendering() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceHmd2GetDeviceInformation(void* info) {
    (void)info;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2GetFieldOfViewWithoutHandle(void* fieldOfView) {
    (void)fieldOfView;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

int APS5_VABI sceHmd2Open() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionBeginFrame() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionDisableMirroring() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionDisableVrMode() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionEnableMirroring() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceHmd2ReprojectionEnableVrMode(std::uint64_t outputMode) {
    (void)outputMode;
    return SCE_HMD2_ERROR_REPROJECTION_NOT_INITIALIZED;
}

int APS5_VABI sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionGetPredictedDisplayTime() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionInitialize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionQueryBufferSizeAlign() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionQueryDisplayBufferSizeAlign() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionSetAllowPositionalReprojection() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionSetMirroringOption() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionSetParam() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionSetParamWithBuffer() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionSetRenderConfig() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceHmd2ReprojectionSetTiming(std::int32_t timing) {
    (void)timing;
    return SCE_HMD2_ERROR_REPROJECTION_NOT_INITIALIZED;
}

int APS5_VABI sceHmd2ReprojectionTerminate() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2SetVibration() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
