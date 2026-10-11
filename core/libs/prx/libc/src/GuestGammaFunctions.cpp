#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cmath>
#include <limits>
extern "C" {
double APS5_VABI tgamma_nid_postfix(double value) {
    const int savedError = errno;
    const auto result = std::tgamma(value);
    errno = savedError;
    return result;
}
float APS5_VABI tgammaf_nid_postfix(float value) {
    const int savedError = errno;
    const auto result = std::tgamma(value);
    errno = savedError;
    return result;
}
long double APS5_VABI tgammal_nid_postfix(long double value) {
    static_assert(sizeof(long double) == 16 && std::numeric_limits<long double>::digits == 64);
    const int savedError = errno;
    const auto result = std::tgamma(value);
    errno = savedError;
    return result;
}
}
