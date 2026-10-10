#include "Hmd2.hpp"

#include "prx/libc/include/General.hpp"

extern "C" {

std::int32_t APS5_VABI sceHmd2Initialize(const void* param) {
    (void)param;
    return Hmd2::RuntimeUnavailable;
}

std::int32_t APS5_VABI sceHmd2Open(std::int32_t userId, std::int32_t type, std::int32_t index,
                                 const Hmd2::OpenParam* param) {
    (void)userId;
    (void)type;
    (void)index;
    (void)param;
    return Hmd2::NotInitialized;
}

std::int32_t APS5_VABI sceHmd2Close(std::int32_t handle) {
    (void)handle;
    return Hmd2::NotInitialized;
}

std::int32_t APS5_VABI sceHmd2GetDeviceInformation(Hmd2::DeviceInformation* info) {
    if (!info) return Hmd2::ParameterNull;
    return Hmd2::NotInitialized;
}

std::int32_t APS5_VABI sceHmd2GetFieldOfViewWithoutHandle(Hmd2::FieldOfView* fieldOfView) {
    if (!fieldOfView) return Hmd2::ParameterNull;
    return Hmd2::NotInitialized;
}

std::int32_t APS5_VABI sceHmd2SetVibration(std::int32_t handle, std::int32_t strength) {
    (void)handle;
    (void)strength;
    return Hmd2::NotInitialized;
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

int APS5_VABI sceHmd2GazeGetResultForFoveatedRendering() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceHmd2ReprojectionInitialize(const void*, const void*) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceHmd2ReprojectionEnableVrMode(std::uint64_t) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceHmd2ReprojectionDisableVrMode() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceHmd2ReprojectionBeginFrame(std::uint64_t) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceHmd2ReprojectionGetPredictedDisplayTime(std::uint64_t, std::uint64_t*) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

Hmd2::SizeAlign APS5_VABI sceHmd2ReprojectionQueryBufferSizeAlign() {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

Hmd2::SizeAlign APS5_VABI sceHmd2ReprojectionQueryDisplayBufferSizeAlign() {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

int APS5_VABI sceHmd2ReprojectionSetAllowPositionalReprojection() {
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

int APS5_VABI sceHmd2GazeGetResult() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionDisableMirroring() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionEnableMirroring() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign() {
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

int APS5_VABI sceHmd2ReprojectionSetTiming() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionTerminate() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
