#ifndef CORE_LIBS_PRX_LIBSCEHMD2_HMD2_HPP
#define CORE_LIBS_PRX_LIBSCEHMD2_HMD2_HPP

#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstddef>
#include <cstdint>

namespace Hmd2 {

inline constexpr std::int32_t NotInitialized = static_cast<std::int32_t>(0x8A720002);
inline constexpr std::int32_t ParameterNull = static_cast<std::int32_t>(0x8A720008);
inline constexpr std::int32_t RuntimeUnavailable = static_cast<std::int32_t>(0x8A720016);

struct OpenParam;

struct DeviceInformation {
    std::uint32_t status;
    std::uint8_t reserved0[4];
    std::uint32_t panelWidth;
    std::uint32_t panelHeight;
    std::uint16_t latency90Hz;
    std::uint16_t latency120Hz;
    std::uint8_t hmuMount;
    std::uint8_t lensSeparationDistance;
    std::uint8_t reserved1[2];
    float virtualImageDistance;
};

struct FieldOfView {
    float tanOut;
    float tanIn;
    float tanTop;
    float tanBottom;
};

struct SizeAlign {
    std::uint64_t size;
    std::uint64_t align;
};

static_assert(sizeof(DeviceInformation) == 28);
static_assert(offsetof(DeviceInformation, panelWidth) == 8);
static_assert(offsetof(DeviceInformation, latency90Hz) == 16);
static_assert(offsetof(DeviceInformation, hmuMount) == 20);
static_assert(offsetof(DeviceInformation, virtualImageDistance) == 24);
static_assert(sizeof(FieldOfView) == 16);
static_assert(sizeof(SizeAlign) == 16);

}

extern "C" {
std::int32_t APS5_VABI sceHmd2Initialize(const void* param);
std::int32_t APS5_VABI sceHmd2Open(std::int32_t userId, std::int32_t type, std::int32_t index,
                                 const Hmd2::OpenParam* param);
std::int32_t APS5_VABI sceHmd2Close(std::int32_t handle);
std::int32_t APS5_VABI sceHmd2GetDeviceInformation(Hmd2::DeviceInformation* info);
std::int32_t APS5_VABI sceHmd2GetFieldOfViewWithoutHandle(Hmd2::FieldOfView* fieldOfView);
std::int32_t APS5_VABI sceHmd2SetVibration(std::int32_t handle, std::int32_t strength);
std::int32_t APS5_VABI sceHmd2ReprojectionInitialize(const void* param, const void* reserved);
std::int32_t APS5_VABI sceHmd2ReprojectionEnableVrMode(std::uint64_t outputMode);
std::int32_t APS5_VABI sceHmd2ReprojectionDisableVrMode();
std::int32_t APS5_VABI sceHmd2ReprojectionBeginFrame(std::uint64_t frameNumber);
std::int32_t APS5_VABI sceHmd2ReprojectionGetPredictedDisplayTime(std::uint64_t frameNumber,
                                                                std::uint64_t* displayTime);
Hmd2::SizeAlign APS5_VABI sceHmd2ReprojectionQueryBufferSizeAlign();
Hmd2::SizeAlign APS5_VABI sceHmd2ReprojectionQueryDisplayBufferSizeAlign();
}

#endif
