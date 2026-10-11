#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cmath>
#include <limits>
extern "C" {
double APS5_VABI nearbyint_nid_postfix(double value) {
    const int savedError = errno;
    const auto result = std::nearbyint(value);
    errno = savedError;
    return result;
}
}
