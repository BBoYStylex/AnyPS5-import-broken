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
double APS5_VABI nextafter_nid_postfix(double, double);
int* APS5_VABI __error_nid_postfix();
}

int main() {
    CHECK(std::bit_cast<std::uint64_t>(nextafter_nid_postfix(1.0, 2.0)) == 0x3ff0000000000001ull);
    CHECK(std::bit_cast<std::uint64_t>(nextafter_nid_postfix(1.0, 0.0)) == 0x3fefffffffffffffull);
    CHECK(std::bit_cast<std::uint64_t>(nextafter_nid_postfix(0.0, -1.0)) == 0x8000000000000001ull);
    CHECK(std::signbit(nextafter_nid_postfix(0.0, -0.0)));
    const double infinity = std::numeric_limits<double>::infinity();
    CHECK(nextafter_nid_postfix(infinity, 0.0) == std::numeric_limits<double>::max());
    CHECK(std::isnan(nextafter_nid_postfix(std::numeric_limits<double>::quiet_NaN(), 1.0)));
    int* guestError = __error_nid_postfix();
    *guestError = 777;
    std::feclearexcept(FE_ALL_EXCEPT);
    CHECK(std::isinf(nextafter_nid_postfix(std::numeric_limits<double>::max(), infinity)));
    CHECK((std::fetestexcept(FE_OVERFLOW) & FE_OVERFLOW) != 0 && *guestError == 777);
    std::feclearexcept(FE_ALL_EXCEPT);
    CHECK(nextafter_nid_postfix(0.0, 1.0) == std::numeric_limits<double>::denorm_min());
    CHECK((std::fetestexcept(FE_UNDERFLOW) & FE_UNDERFLOW) != 0 && *guestError == 777);
}
