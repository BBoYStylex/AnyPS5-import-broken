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
int APS5_VABI ilogb_nid_postfix(double);
int APS5_VABI ilogbf_nid_postfix(float);
int* APS5_VABI __error_nid_postfix();
}

int main() {
    CHECK(ilogb_nid_postfix(1.0) == 0 && ilogb_nid_postfix(-8.0) == 3);
    CHECK(ilogb_nid_postfix(0.75) == -1);
    CHECK(ilogb_nid_postfix(std::numeric_limits<double>::min()) == -1022);
    CHECK(ilogb_nid_postfix(std::numeric_limits<double>::denorm_min()) == -1074);
    CHECK(ilogb_nid_postfix(std::numeric_limits<double>::max()) == 1023);
    CHECK(ilogbf_nid_postfix(std::numeric_limits<float>::min()) == -126);
    CHECK(ilogbf_nid_postfix(std::numeric_limits<float>::denorm_min()) == -149);
    CHECK(ilogbf_nid_postfix(std::numeric_limits<float>::max()) == 127);
    int* error = __error_nid_postfix();
    *error = 123;
    std::feclearexcept(FE_ALL_EXCEPT);
    CHECK(ilogb_nid_postfix(0.0) == -INT_MAX && ilogbf_nid_postfix(-0.0f) == -INT_MAX);
    CHECK(ilogb_nid_postfix(std::numeric_limits<double>::infinity()) == INT_MAX);
    CHECK(ilogbf_nid_postfix(std::numeric_limits<float>::quiet_NaN()) == INT_MAX);
    CHECK(ilogb_nid_postfix(std::bit_cast<double>(0x7ff0000000000001ull)) == INT_MAX);
    CHECK(std::fetestexcept(FE_ALL_EXCEPT) == 0 && *error == 123);
}
