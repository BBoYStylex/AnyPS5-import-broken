#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cmath>
#include <limits>
extern "C" {
double APS5_VABI nexttoward_nid_postfix(double from, long double toward) {
    static_assert(sizeof(long double) == 16 && std::numeric_limits<long double>::digits == 64);
    const int savedError = errno;
    const auto result = std::nexttoward(from, toward);
    errno = savedError;
    return result;
}
}
