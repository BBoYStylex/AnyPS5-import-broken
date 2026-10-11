#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <bit>
#include <cerrno>
#include <cfenv>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <initializer_list>

static void Require(bool condition, int line) {
    if (!condition) {
        std::fprintf(stderr, "Library regression failed at line %d\n", line);
        std::abort();
    }
}
#define CHECK(condition) Require((condition), __LINE__)

extern "C" {
int APS5_VABI ffs_nid_postfix(int);
int APS5_VABI ffsl_nid_postfix(std::int64_t);
int APS5_VABI ffsll_nid_postfix(std::int64_t);
}

int main() {
    CHECK(ffs_nid_postfix(0) == 0 && ffsl_nid_postfix(0) == 0 && ffsll_nid_postfix(0) == 0);
    CHECK(ffs_nid_postfix(-1) == 1 && ffsl_nid_postfix(-1) == 1);
    for (unsigned index = 0; index < 64; ++index) {
        const auto bits = std::uint64_t{1} << index;
        const auto value = std::bit_cast<std::int64_t>(bits);
        CHECK(ffsl_nid_postfix(value) == static_cast<int>(index + 1));
        CHECK(ffsll_nid_postfix(value) == static_cast<int>(index + 1));
        if (index < 32) CHECK(ffs_nid_postfix(std::bit_cast<int>(static_cast<std::uint32_t>(bits))) == static_cast<int>(index + 1));
        CHECK(ffsl_nid_postfix(std::bit_cast<std::int64_t>(bits | 1)) == 1);
    }
    CHECK(ffs_nid_postfix(INT_MIN) == 32);
    CHECK(ffsl_nid_postfix(INT64_MIN) == 64);
}
