#include "prx/libc/include/general/VabiMacros.hpp"
#include <bit>
#include <cstdint>
#include <climits>
extern "C" {
int APS5_VABI ilogb_nid_postfix(double value) {
    const auto bits = std::bit_cast<std::uint64_t>(value) & 0x7fffffffffffffffull;
    const auto exponent = static_cast<int>(bits >> 52);
    const auto fraction = bits & 0x000fffffffffffffull;
    if (exponent == 0) return fraction == 0 ? -INT_MAX : static_cast<int>(std::bit_width(fraction)) - 1075;
    if (exponent == 0x7ff) return INT_MAX;
    return exponent - 1023;
}
int APS5_VABI ilogbf_nid_postfix(float value) {
    const auto bits = std::bit_cast<std::uint32_t>(value) & 0x7fffffffu;
    const auto exponent = static_cast<int>(bits >> 23);
    const auto fraction = bits & 0x007fffffu;
    if (exponent == 0) return fraction == 0 ? -INT_MAX : static_cast<int>(std::bit_width(fraction)) - 150;
    if (exponent == 0xff) return INT_MAX;
    return exponent - 127;
}
}
