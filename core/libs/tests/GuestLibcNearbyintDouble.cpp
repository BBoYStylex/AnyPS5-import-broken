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
double APS5_VABI nearbyint_nid_postfix(double);
}

int main() {
    const int previous = std::fegetround();
    const struct { int mode; double positive; double negative; } cases[] = {
        {FE_TONEAREST, 2.0, -2.0}, {FE_UPWARD, 3.0, -2.0},
        {FE_DOWNWARD, 2.0, -3.0}, {FE_TOWARDZERO, 2.0, -2.0}};
    for (const auto& entry : cases) {
        CHECK(std::fesetround(entry.mode) == 0);
        std::feclearexcept(FE_ALL_EXCEPT);
        CHECK(nearbyint_nid_postfix(2.5) == entry.positive);
        CHECK(nearbyint_nid_postfix(-2.5) == entry.negative);
        CHECK((std::fetestexcept(FE_INEXACT) & FE_INEXACT) == 0);
        std::feraiseexcept(FE_INEXACT);
        CHECK(nearbyint_nid_postfix(1.2) >= 1.0);
        CHECK((std::fetestexcept(FE_INEXACT) & FE_INEXACT) != 0);
    }
    CHECK(std::fesetround(previous) == 0);
    CHECK(std::signbit(nearbyint_nid_postfix(-0.0)));
    CHECK(std::isinf(nearbyint_nid_postfix(std::numeric_limits<double>::infinity())));
    CHECK(std::isnan(nearbyint_nid_postfix(std::numeric_limits<double>::quiet_NaN())));
    std::feclearexcept(FE_ALL_EXCEPT);
    const double signaling = std::bit_cast<double>(0x7ff0000000000001ull);
    const auto quieted = std::bit_cast<std::uint64_t>(nearbyint_nid_postfix(signaling));
    CHECK((quieted & 0x7ff8000000000000ull) == 0x7ff8000000000000ull);
    CHECK(std::fetestexcept(FE_ALL_EXCEPT) == 0);
    std::feraiseexcept(FE_INVALID | FE_DIVBYZERO);
    CHECK(nearbyint_nid_postfix(1.2) == 1.0);
    CHECK(std::fetestexcept(FE_ALL_EXCEPT) == (FE_INVALID | FE_DIVBYZERO));
}
