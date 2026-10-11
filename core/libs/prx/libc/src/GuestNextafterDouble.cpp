#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cmath>
#include <limits>
extern "C" {
double APS5_VABI nextafter_nid_postfix(double from, double toward) {
    const int savedError = errno;
    const auto result = std::nextafter(from, toward);
    errno = savedError;
    return result;
}
}
