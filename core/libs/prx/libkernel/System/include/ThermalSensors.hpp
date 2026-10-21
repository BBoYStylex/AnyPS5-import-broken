#ifndef CORE_LIBS_PRX_LIBKERNEL_SYSTEM_THERMALSENSORS_HPP
#define CORE_LIBS_PRX_LIBKERNEL_SYSTEM_THERMALSENSORS_HPP

#include "prx/libc/include/general/VabiMacros.hpp"

extern "C" void ReadThermalSensors_nid_no_patch(const char* root, int celsius[3]);

extern "C" int APS5_VABI sceKernelGetSocPowerConsumption(void* consumption, double reserved);

#endif
