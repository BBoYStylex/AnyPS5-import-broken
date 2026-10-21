#ifndef CORE_LIBS_PRX_LIBKERNEL_SYS_INCLUDE_KERNELSYS_HPP
#define CORE_LIBS_PRX_LIBKERNEL_SYS_INCLUDE_KERNELSYS_HPP

#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>

extern "C" {

int APS5_VABI sceKernelGetBasicProductShape(int* out);
int APS5_VABI sceKernelGetCpuUsage(void* buffer, int* count);
int APS5_VABI sceKernelGetCpuUsageAll(int* perCorePercentage, int* count);
int APS5_VABI sceKernelGetCpuUsageProc(int processId, void* buffer, int* count);
int APS5_VABI sceKernelGetCurrentFanDuty(std::uint16_t* duty, std::uint64_t* chassis);
int APS5_VABI sceKernelGetHwModelName(char* modelName);
int APS5_VABI sceKernelGetHwSerialNumber(char* serialNumber);
int APS5_VABI sceKernelGetSocClock(std::uint32_t* rawDomains);
int APS5_VABI sceKernelGetUniversalMode(int* mode);
int APS5_VABI sceKernelIccGetBDPowerState(std::uint8_t* state);
int APS5_VABI sceKernelIccGetPowerNumberOfBootShutdown(std::uint64_t* count);
int APS5_VABI sceKernelIccGetPowerOperatingTime(std::uint64_t* time);
int APS5_VABI sceKernelIccGetUSBPowerState(std::uint8_t* state);
int APS5_VABI sceKernelSetBudget(std::int64_t budget);
int APS5_VABI sceKernelSetProcessName(const char* name);

}

#endif
