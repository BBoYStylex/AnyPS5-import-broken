#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cmath>
#include <limits>
extern "C" {
long double APS5_VABI ceill_nid_postfix(long double value) {
    static_assert(sizeof(long double) == 16 && std::numeric_limits<long double>::digits == 64);
    const int savedError = errno;
    const auto result = std::ceil(value);
    errno = savedError;
    return result;
}
long double APS5_VABI floorl_nid_postfix(long double value) {
    static_assert(sizeof(long double) == 16 && std::numeric_limits<long double>::digits == 64);
    const int savedError = errno;
    const auto result = std::floor(value);
    errno = savedError;
    return result;
}
long double APS5_VABI roundl_nid_postfix(long double value) {
    static_assert(sizeof(long double) == 16 && std::numeric_limits<long double>::digits == 64);
    const int savedError = errno;
    const auto result = std::round(value);
    errno = savedError;
    return result;
}
}
