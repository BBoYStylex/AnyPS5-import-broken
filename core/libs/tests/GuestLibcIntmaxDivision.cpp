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
struct GuestIntmaxDivision { std::int64_t quotient; std::int64_t remainder; };
GuestIntmaxDivision APS5_VABI imaxdiv_nid_postfix(std::int64_t, std::int64_t);
}

int main() {
    static_assert(sizeof(GuestIntmaxDivision) == 16);
    for (const auto numerator : {17LL, -17LL, 0LL, 9223372036854775807LL, -9223372036854775807LL}) {
        for (const auto denominator : {5LL, -5LL, 1LL, -1LL}) {
            const auto result = imaxdiv_nid_postfix(numerator, denominator);
            CHECK(result.quotient * denominator + result.remainder == numerator);
            CHECK(result.remainder > -5 && result.remainder < 5);
            CHECK(result.remainder == 0 || (result.remainder < 0) == (numerator < 0));
        }
    }
    const auto zero = imaxdiv_nid_postfix(0, INT64_MIN);
    CHECK(zero.quotient == 0 && zero.remainder == 0);
    const auto boundary = imaxdiv_nid_postfix(INT64_MIN, 1);
    CHECK(boundary.quotient == INT64_MIN && boundary.remainder == 0);
    const auto negative = imaxdiv_nid_postfix(-17, 5);
    CHECK(negative.quotient == -3 && negative.remainder == -2);
}
