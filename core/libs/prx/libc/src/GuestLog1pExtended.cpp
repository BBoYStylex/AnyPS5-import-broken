#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cmath>
#include <limits>
extern "C" {
long double APS5_VABI log1pl_nid_postfix(long double value) {
    static_assert(sizeof(long double) == 16 && std::numeric_limits<long double>::digits == 64);
    const int savedError = errno;
    const auto result = std::log1p(value);
    errno = savedError;
    return result;
}
}
