#include "prx/libc/include/general/VabiMacros.hpp"
#include <bit>
#include <cstdint>
#include <cmath>
#include <limits>
extern "C" {
double APS5_VABI copysign_nid_postfix(double magnitude, double sign) {
    return std::bit_cast<double>((std::bit_cast<std::uint64_t>(magnitude) & 0x7fffffffffffffffull)
        | (std::bit_cast<std::uint64_t>(sign) & 0x8000000000000000ull));
}
float APS5_VABI copysignf_nid_postfix(float magnitude, float sign) {
    return std::bit_cast<float>((std::bit_cast<std::uint32_t>(magnitude) & 0x7fffffffu)
        | (std::bit_cast<std::uint32_t>(sign) & 0x80000000u));
}
long double APS5_VABI copysignl_nid_postfix(long double magnitude, long double sign) {
    static_assert(sizeof(long double) == 16);
    static_assert(std::numeric_limits<long double>::digits == 64);
    return std::copysign(magnitude, sign);
}
}
