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
double APS5_VABI erf_nid_postfix(double);
double APS5_VABI erfc_nid_postfix(double);
float APS5_VABI erff_nid_postfix(float);
float APS5_VABI erfcf_nid_postfix(float);
long double APS5_VABI erfl_nid_postfix(long double);
long double APS5_VABI erfcl_nid_postfix(long double);
int* APS5_VABI __error_nid_postfix();
}

int main() {
    CHECK(erf_nid_postfix(0.0) == 0.0 && std::signbit(erf_nid_postfix(-0.0)));
    CHECK(erfc_nid_postfix(0.0) == 1.0);
    CHECK(std::fabs(erf_nid_postfix(1.0) - 0.84270079294971486934) < 2e-15);
    CHECK(std::fabs(erfc_nid_postfix(1.0) - 0.15729920705028513066) < 2e-15);
    CHECK(std::fabs(erff_nid_postfix(1.0f) - 0.8427007929f) < 2e-7f);
    CHECK(std::fabs(erfcf_nid_postfix(1.0f) - 0.1572992071f) < 2e-7f);
    CHECK(std::fabs(erfl_nid_postfix(1.0L) - 0.84270079294971486934L) < 2e-18L);
    CHECK(std::fabs(erfcl_nid_postfix(1.0L) - 0.15729920705028513066L) < 2e-18L);
    CHECK(erf_nid_postfix(std::numeric_limits<double>::infinity()) == 1.0);
    CHECK(erfc_nid_postfix(-std::numeric_limits<double>::infinity()) == 2.0);
    CHECK(std::isnan(erf_nid_postfix(std::numeric_limits<double>::quiet_NaN())));
    CHECK(std::fabs(erfc_nid_postfix(10.0) - 2.088487583762545e-45) < 1e-58);
    int* guestError = __error_nid_postfix();
    *guestError = 777;
    CHECK(erfc_nid_postfix(40.0) == 0.0 && *guestError == 777);
}
