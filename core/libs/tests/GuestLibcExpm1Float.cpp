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
float APS5_VABI expm1f_nid_postfix(float);
int* APS5_VABI __error_nid_postfix();
}

int main() {
    CHECK(expm1f_nid_postfix(0.0f) == 0.0f && std::signbit(expm1f_nid_postfix(-0.0f)));
    const float small = 0x1p-30f;
    CHECK(expm1f_nid_postfix(small) == small && expm1f_nid_postfix(-small) == -small);
    CHECK(std::fabs(expm1f_nid_postfix(1.0f) - 1.718281828459045f) < 0.000001f);
    CHECK(expm1f_nid_postfix(-std::numeric_limits<float>::infinity()) == -1.0f);
    CHECK(std::isinf(expm1f_nid_postfix(std::numeric_limits<float>::infinity())));
    CHECK(std::isnan(expm1f_nid_postfix(std::numeric_limits<float>::quiet_NaN())));
    int* guestError = __error_nid_postfix();
    *guestError = 777;
    CHECK(std::isinf(expm1f_nid_postfix(100.0f)) && *guestError == 777);
}
