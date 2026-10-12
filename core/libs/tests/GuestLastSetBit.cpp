#include "prx/libc/include/general/VabiMacros.hpp"
#include <bit>
#include <cstdint>
#include <cstdlib>

extern "C" {
int APS5_VABI fls_nid_postfix(std::int32_t);
}

static void Require(bool value) {
    if (!value) std::abort();
}

int main() {
    Require(fls_nid_postfix(0) == 0);
    Require(fls_nid_postfix(-1) == 32);
    for (int last = 0; last < 32; ++last) {
        const auto high = std::uint32_t{1} << last;
        Require(fls_nid_postfix(std::bit_cast<std::int32_t>(high)) == last + 1);
        Require(fls_nid_postfix(std::bit_cast<std::int32_t>(high - 1)) == last);
        for (int first = 0; first <= last; ++first) {
            const auto bits = high | (std::uint32_t{1} << first);
            Require(fls_nid_postfix(std::bit_cast<std::int32_t>(bits)) == last + 1);
        }
    }
}
