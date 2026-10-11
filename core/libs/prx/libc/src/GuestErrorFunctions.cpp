#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cmath>
#include <limits>
extern "C" {
double APS5_VABI erf_nid_postfix(double value) {
    const int savedError = errno;
    const auto result = std::erf(value);
    errno = savedError;
    return result;
}
double APS5_VABI erfc_nid_postfix(double value) {
    const int savedError = errno;
    const auto result = std::erfc(value);
    errno = savedError;
    return result;
}
float APS5_VABI erff_nid_postfix(float value) {
    const int savedError = errno;
    const auto result = std::erf(value);
    errno = savedError;
    return result;
}
float APS5_VABI erfcf_nid_postfix(float value) {
    const int savedError = errno;
    const auto result = std::erfc(value);
    errno = savedError;
    return result;
}
long double APS5_VABI erfl_nid_postfix(long double value) {
    static_assert(sizeof(long double) == 16 && std::numeric_limits<long double>::digits == 64);
    const int savedError = errno;
    const auto result = std::erf(value);
    errno = savedError;
    return result;
}
long double APS5_VABI erfcl_nid_postfix(long double value) {
    static_assert(sizeof(long double) == 16 && std::numeric_limits<long double>::digits == 64);
    const int savedError = errno;
    const auto result = std::erfc(value);
    errno = savedError;
    return result;
}
}
