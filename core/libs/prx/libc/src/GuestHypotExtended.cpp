#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cmath>
#include <limits>
extern "C" {
long double APS5_VABI hypotl_nid_postfix(long double left, long double right) {
    static_assert(sizeof(long double) == 16 && std::numeric_limits<long double>::digits == 64);
    const int savedError = errno;
    const auto result = std::isinf(left) || std::isinf(right) ? std::numeric_limits<long double>::infinity() : std::hypot(left, right);
    errno = savedError;
    return result;
}
}
