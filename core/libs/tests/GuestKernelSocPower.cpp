#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/KernelErrors.hpp"
#include "prx/libkernel/System/include/ThermalSensors.hpp"
#include <cstdint>
#include <cstdlib>
#include <type_traits>

static void Require(bool value) { if (!value) std::abort(); }

static_assert(std::is_same_v<decltype(&sceKernelGetSocPowerConsumption), int (APS5_VABI *)(void*, double)>);

int main() {
    std::uint8_t buffer[128]{};
    Require(sceKernelGetSocPowerConsumption(nullptr, 0.0) == SCE_KERNEL_ERROR_EFAULT);
    Require(sceKernelGetSocPowerConsumption(buffer, 0.0) == SCE_KERNEL_ERROR_EINVAL);
    return 0;
}
