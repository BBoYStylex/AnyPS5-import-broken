#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceVideoOut/include/Output.hpp"
#include "prx/libSceVideoOut/include/VideoOutDriver.hpp"
#include <stdexcept>

static constexpr int VIDEO_OUT_CURSOR_NUM_MAX = 2;

namespace {

int validateCursorHandle(std::int32_t handle, std::int32_t index) {
    if (!VideoOutDriver::Get().HasConfig(handle) || !VideoOutDriver::Get().IsOpen(handle))
        return VIDEO_OUT_ERROR_INVALID_HANDLE;
    if (index < 0 || index >= VIDEO_OUT_CURSOR_NUM_MAX)
        return VIDEO_OUT_ERROR_INVALID_INDEX;
    return 0;
}

}

extern "C" {

int APS5_VABI sceVideoOutSetFlipMaster() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceVideoOutClearFlipMaster() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceVideoOutCursorEnable(std::int32_t handle, std::int32_t index, const void* address) {
    const int err = validateCursorHandle(handle, index);
    if (err != 0) return err;
    if (address == nullptr) return VIDEO_OUT_ERROR_INVALID_ADDRESS;
    return 0;
}

int APS5_VABI sceVideoOutCursorDisable(std::int32_t handle, std::int32_t index) {
    const int err = validateCursorHandle(handle, index);
    if (err != 0) return err;
    return 0;
}

int APS5_VABI sceVideoOutCursorSetImageAddress(std::int32_t handle, std::int32_t index, const void* address) {
    const int err = validateCursorHandle(handle, index);
    if (err != 0) return err;
    if (address == nullptr) return VIDEO_OUT_ERROR_INVALID_ADDRESS;
    return 0;
}

int APS5_VABI sceVideoOutCursorSetPosition(std::int32_t handle, std::int32_t index, std::uint32_t x, std::uint32_t y) {
    (void)x;
    (void)y;
    const int err = validateCursorHandle(handle, index);
    if (err != 0) return err;
    return 0;
}

int APS5_VABI sceVideoOutAdjustColor_(int handle, const VideoOutColorSettings* settings, uint32_t settings_size) {
    if (settings_size < sizeof(VideoOutColorSettings)) throw std::runtime_error("sceVideoOutAdjustColor_: VIDEO_OUT_ERROR_INVALID_VALUE");
    return sceVideoOutAdjustColor(handle, settings);
}

int APS5_VABI sceVideoOutColorSettingsSetGamma_(VideoOutColorSettings* settings, float gamma, uint32_t settings_size) {
    if (settings_size < sizeof(VideoOutColorSettings)) throw std::runtime_error("sceVideoOutColorSettingsSetGamma_: VIDEO_OUT_ERROR_INVALID_VALUE");
    return sceVideoOutColorSettingsSetGamma(settings, gamma);
}

int APS5_VABI sceVideoOutColorSpaceConversionSettingsSetDefaultSdrToHdrConversion_(VideoOutColorSpaceConversionSettings* settings, uint32_t settings_size) {
    if (settings_size < sizeof(VideoOutColorSpaceConversionSettings)) throw std::runtime_error("sceVideoOutColorSpaceConversionSettingsSetDefaultSdrToHdrConversion_: VIDEO_OUT_ERROR_INVALID_VALUE");
    return sceVideoOutColorSpaceConversionSettingsSetDefaultSdrToHdrConversion(settings);
}

int APS5_VABI sceVideoOutColorSpaceConversionSettingsSetSdrToHdrConversion_(VideoOutColorSpaceConversionSettings* settings, float lumaScale, uint32_t option, uint32_t settings_size) {
    if (settings_size < sizeof(VideoOutColorSpaceConversionSettings)) throw std::runtime_error("sceVideoOutColorSpaceConversionSettingsSetSdrToHdrConversion_: VIDEO_OUT_ERROR_INVALID_VALUE");
    return sceVideoOutColorSpaceConversionSettingsSetSdrToHdrConversion(settings, lumaScale, option);
}

int APS5_VABI sceVideoOutAdjustColorSpaceConversion_(int handle, const VideoOutColorSpaceConversionSettings* settings, uint32_t settings_size) {
    if (settings_size < sizeof(VideoOutColorSpaceConversionSettings)) throw std::runtime_error("sceVideoOutAdjustColorSpaceConversion_: VIDEO_OUT_ERROR_INVALID_VALUE");
    return sceVideoOutAdjustColorSpaceConversion(handle, settings);
}

}
