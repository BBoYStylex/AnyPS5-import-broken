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
double APS5_VABI nexttoward_nid_postfix(double, long double);
int* APS5_VABI __error_nid_postfix();
}

int main() {
    static_assert(sizeof(long double) == 16 && std::numeric_limits<long double>::digits == 64);
    const long double above = 1.0L + 0x1p-60L;
    const long double below = 1.0L - 0x1p-60L;
    CHECK(static_cast<double>(above) == 1.0 && static_cast<double>(below) == 1.0);
    CHECK(std::bit_cast<std::uint64_t>(nexttoward_nid_postfix(1.0, above)) == 0x3ff0000000000001ull);
    CHECK(std::bit_cast<std::uint64_t>(nexttoward_nid_postfix(1.0, below)) == 0x3fefffffffffffffull);
    CHECK(std::signbit(nexttoward_nid_postfix(0.0, -0.0L)));
    CHECK(nexttoward_nid_postfix(0.0, 1.0L) == std::numeric_limits<double>::denorm_min());
    CHECK(nexttoward_nid_postfix(std::numeric_limits<double>::infinity(), 0.0L) == std::numeric_limits<double>::max());
    CHECK(std::isnan(nexttoward_nid_postfix(1.0, std::numeric_limits<long double>::quiet_NaN())));
    int* guestError = __error_nid_postfix();
    *guestError = 777;
    CHECK(std::isinf(nexttoward_nid_postfix(std::numeric_limits<double>::max(), std::numeric_limits<long double>::infinity())));
    CHECK(*guestError == 777);
}
