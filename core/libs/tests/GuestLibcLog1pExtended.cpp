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
long double APS5_VABI log1pl_nid_postfix(long double);
int* APS5_VABI __error_nid_postfix();
}

int main() {
    static_assert(sizeof(long double) == 16 && std::numeric_limits<long double>::digits == 64);
    const long double tiny = 0x1p-70L;
    CHECK(log1pl_nid_postfix(tiny) == tiny && log1pl_nid_postfix(-tiny) == -tiny);
    CHECK(log1pl_nid_postfix(0.0L) == 0.0L && std::signbit(log1pl_nid_postfix(-0.0L)));
    CHECK(std::fabs(log1pl_nid_postfix(1.0L) - 0.693147180559945309417L) < 2e-19L);
    CHECK(std::isinf(log1pl_nid_postfix(-1.0L)) && std::signbit(log1pl_nid_postfix(-1.0L)));
    CHECK(std::isnan(log1pl_nid_postfix(-2.0L)));
    CHECK(std::isinf(log1pl_nid_postfix(std::numeric_limits<long double>::infinity())));
    int* guestError = __error_nid_postfix();
    *guestError = 777;
    CHECK(std::isnan(log1pl_nid_postfix(-2.0L)) && *guestError == 777);
}
