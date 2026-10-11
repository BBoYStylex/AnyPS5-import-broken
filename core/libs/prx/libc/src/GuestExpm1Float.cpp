#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cmath>
#include <limits>
extern "C" {
float APS5_VABI expm1f_nid_postfix(float value) {
    const int savedError = errno;
    const auto result = std::expm1(value);
    errno = savedError;
    return result;
}
}
