#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel_sys/include/KernelSys.hpp"
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <type_traits>

static void Require(bool value) { if (!value) std::abort(); }

static_assert(std::is_same_v<decltype(&sceKernelGetBasicProductShape), int (APS5_VABI *)(int*)>);
static_assert(std::is_same_v<decltype(&sceKernelGetCpuUsage), int (APS5_VABI *)(void*, int*)>);
static_assert(std::is_same_v<decltype(&sceKernelGetCpuUsageAll), int (APS5_VABI *)(int*, int*)>);
static_assert(std::is_same_v<decltype(&sceKernelGetCpuUsageProc), int (APS5_VABI *)(int, void*, int*)>);
static_assert(std::is_same_v<decltype(&sceKernelGetCurrentFanDuty), int (APS5_VABI *)(std::uint16_t*, std::uint64_t*)>);
static_assert(std::is_same_v<decltype(&sceKernelGetHwModelName), int (APS5_VABI *)(char*)>);
static_assert(std::is_same_v<decltype(&sceKernelGetHwSerialNumber), int (APS5_VABI *)(char*)>);
static_assert(std::is_same_v<decltype(&sceKernelGetSocClock), int (APS5_VABI *)(std::uint32_t*)>);
static_assert(std::is_same_v<decltype(&sceKernelGetUniversalMode), int (APS5_VABI *)(int*)>);
static_assert(std::is_same_v<decltype(&sceKernelIccGetBDPowerState), int (APS5_VABI *)(std::uint8_t*)>);
static_assert(std::is_same_v<decltype(&sceKernelIccGetPowerNumberOfBootShutdown), int (APS5_VABI *)(std::uint64_t*)>);
static_assert(std::is_same_v<decltype(&sceKernelIccGetPowerOperatingTime), int (APS5_VABI *)(std::uint64_t*)>);
static_assert(std::is_same_v<decltype(&sceKernelIccGetUSBPowerState), int (APS5_VABI *)(std::uint8_t*)>);
static_assert(std::is_same_v<decltype(&sceKernelSetBudget), int (APS5_VABI *)(std::int64_t)>);
static_assert(std::is_same_v<decltype(&sceKernelSetProcessName), int (APS5_VABI *)(const char*)>);

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
    int number = 0;
    char text[64]{};
    std::uint8_t byte = 0;
    std::uint16_t half = 0;
    std::uint32_t word = 0;
    std::uint64_t wide = 0;
    void* buffer = &number;
    Require(ReportsNotImplemented("sceKernelGetBasicProductShape", [&] { sceKernelGetBasicProductShape(&number); }));
    Require(ReportsNotImplemented("sceKernelGetCpuUsage", [&] { sceKernelGetCpuUsage(buffer, &number); }));
    Require(ReportsNotImplemented("sceKernelGetCpuUsageAll", [&] { sceKernelGetCpuUsageAll(&number, &number); }));
    Require(ReportsNotImplemented("sceKernelGetCpuUsageProc", [&] { sceKernelGetCpuUsageProc(1, buffer, &number); }));
    Require(ReportsNotImplemented("sceKernelGetCurrentFanDuty", [&] { sceKernelGetCurrentFanDuty(&half, &wide); }));
    Require(ReportsNotImplemented("sceKernelGetHwModelName", [&] { sceKernelGetHwModelName(text); }));
    Require(ReportsNotImplemented("sceKernelGetHwSerialNumber", [&] { sceKernelGetHwSerialNumber(text); }));
    Require(ReportsNotImplemented("sceKernelGetSocClock", [&] { sceKernelGetSocClock(&word); }));
    Require(ReportsNotImplemented("sceKernelGetUniversalMode", [&] { sceKernelGetUniversalMode(&number); }));
    Require(ReportsNotImplemented("sceKernelIccGetBDPowerState", [&] { sceKernelIccGetBDPowerState(&byte); }));
    Require(ReportsNotImplemented("sceKernelIccGetPowerNumberOfBootShutdown", [&] { sceKernelIccGetPowerNumberOfBootShutdown(&wide); }));
    Require(ReportsNotImplemented("sceKernelIccGetPowerOperatingTime", [&] { sceKernelIccGetPowerOperatingTime(&wide); }));
    Require(ReportsNotImplemented("sceKernelIccGetUSBPowerState", [&] { sceKernelIccGetUSBPowerState(&byte); }));
    Require(ReportsNotImplemented("sceKernelSetBudget", [&] { sceKernelSetBudget(0); }));
    Require(ReportsNotImplemented("sceKernelSetProcessName", [&] { sceKernelSetProcessName(text); }));
    return 0;
}
