#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceSysUtilSendSystemNotificationWithText(int32_t type, const char* message) {
    (void)type;
    (void)message;
    return 0;
}

}
