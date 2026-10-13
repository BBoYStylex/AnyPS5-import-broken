#include <cstdint>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI __tsan_acquire(void) {
    return 0;
}

int APS5_VABI __tsan_release(void) {
    return 0;
}

}
