#include "prx/libc/include/general/VabiMacros.hpp"
#include <bit>
#include <cstdint>
extern "C" {
int APS5_VABI ffs_nid_postfix(int value) {
    const auto bits = static_cast<std::uint32_t>(value);
    return bits == 0 ? 0 : std::countr_zero(bits) + 1;
}
int APS5_VABI ffsl_nid_postfix(std::int64_t value) {
    const auto bits = static_cast<std::uint64_t>(value);
    return bits == 0 ? 0 : std::countr_zero(bits) + 1;
}
int APS5_VABI ffsll_nid_postfix(std::int64_t value) { return ffsl_nid_postfix(value); }
}
