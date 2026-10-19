#include <cstdint>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceLoginServiceInitialize(void) {
    return 0;
}

int APS5_VABI sceLoginServiceTerminate(void) {
    return 0;
}

int APS5_VABI sceLoginServiceRequestDevices(void* param) {
    (void)param;
    return 0;
}

}
