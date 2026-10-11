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
long double APS5_VABI hypotl_nid_postfix(long double, long double);
int* APS5_VABI __error_nid_postfix();
}

int main() {
    static_assert(sizeof(long double) == 16 && std::numeric_limits<long double>::digits == 64);
    CHECK(hypotl_nid_postfix(3.0L, -4.0L) == 5.0L);
    CHECK(hypotl_nid_postfix(-0.0L, -0.0L) == 0.0L && !std::signbit(hypotl_nid_postfix(-0.0L, -0.0L)));
    const long double scale = 0x1p8000L;
    CHECK(hypotl_nid_postfix(3.0L * scale, 4.0L * scale) == 5.0L * scale);
    const long double tiny = 0x1p-8000L;
    CHECK(hypotl_nid_postfix(3.0L * tiny, 4.0L * tiny) == 5.0L * tiny);
    const long double precise = 1.0L + 0x1p-60L;
    CHECK(hypotl_nid_postfix(precise, 0.0L) == precise);
    const long double infinity = std::numeric_limits<long double>::infinity();
    const long double nan = std::numeric_limits<long double>::quiet_NaN();
    CHECK(hypotl_nid_postfix(infinity, nan) == infinity && hypotl_nid_postfix(nan, infinity) == infinity);
    CHECK(std::isnan(hypotl_nid_postfix(nan, 1.0L)));
    int* guestError = __error_nid_postfix();
    *guestError = 777;
    CHECK(std::isinf(hypotl_nid_postfix(std::numeric_limits<long double>::max(), std::numeric_limits<long double>::max())) && *guestError == 777);
}
