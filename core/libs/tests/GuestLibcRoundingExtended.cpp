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
long double APS5_VABI ceill_nid_postfix(long double);
long double APS5_VABI floorl_nid_postfix(long double);
long double APS5_VABI roundl_nid_postfix(long double);
int* APS5_VABI __error_nid_postfix();
}

int main() {
    static_assert(sizeof(long double) == 16 && std::numeric_limits<long double>::digits == 64);
    const int previous = std::fegetround();
    for (int mode : {FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO}) {
        CHECK(std::fesetround(mode) == 0);
        CHECK(ceill_nid_postfix(2.25L) == 3.0L && ceill_nid_postfix(-2.25L) == -2.0L);
        CHECK(floorl_nid_postfix(2.25L) == 2.0L && floorl_nid_postfix(-2.25L) == -3.0L);
        CHECK(roundl_nid_postfix(2.5L) == 3.0L && roundl_nid_postfix(-2.5L) == -3.0L);
    }
    CHECK(std::fesetround(previous) == 0);
    const long double large = 0x1p60L + 0.25L;
    CHECK(ceill_nid_postfix(large) == 0x1p60L + 1.0L);
    CHECK(floorl_nid_postfix(large) == 0x1p60L && roundl_nid_postfix(large) == 0x1p60L);
    CHECK(ceill_nid_postfix(1.0L + 0x1p-60L) == 2.0L);
    CHECK(std::signbit(ceill_nid_postfix(-0.25L)));
    CHECK(std::signbit(roundl_nid_postfix(-0.25L)));
    CHECK(!std::signbit(floorl_nid_postfix(0.25L)));
    for (auto operation : {&ceill_nid_postfix, &floorl_nid_postfix, &roundl_nid_postfix}) {
        CHECK(operation(0x1p12000L) == 0x1p12000L);
        CHECK(operation(-0.0L) == 0.0L && std::signbit(operation(-0.0L)));
        CHECK(std::isinf(operation(std::numeric_limits<long double>::infinity())));
        CHECK(std::isnan(operation(std::numeric_limits<long double>::quiet_NaN())));
    }
}
