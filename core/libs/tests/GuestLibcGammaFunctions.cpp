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
double APS5_VABI tgamma_nid_postfix(double);
float APS5_VABI tgammaf_nid_postfix(float);
long double APS5_VABI tgammal_nid_postfix(long double);
int* APS5_VABI __error_nid_postfix();
}

int main() {
    CHECK(tgamma_nid_postfix(1.0) == 1.0 && tgamma_nid_postfix(2.0) == 1.0);
    CHECK(std::fabs(tgamma_nid_postfix(6.0) - 120.0) < 1e-12);
    CHECK(std::fabs(tgamma_nid_postfix(0.5) - 1.7724538509055160273) < 3e-15);
    CHECK(std::fabs(tgamma_nid_postfix(-0.5) + 3.5449077018110320546) < 5e-15);
    CHECK(std::fabs(tgammaf_nid_postfix(6.0f) - 120.0f) < 0.0001f);
    CHECK(std::fabs(tgammal_nid_postfix(0.5L) - 1.7724538509055160273L) < 3e-18L);
    CHECK(std::isinf(tgamma_nid_postfix(0.0)) && !std::signbit(tgamma_nid_postfix(0.0)));
    CHECK(std::isinf(tgamma_nid_postfix(-0.0)) && std::signbit(tgamma_nid_postfix(-0.0)));
    CHECK(std::isnan(tgamma_nid_postfix(-1.0)));
    CHECK(std::isnan(tgamma_nid_postfix(-std::numeric_limits<double>::infinity())));
    CHECK(std::isinf(tgamma_nid_postfix(std::numeric_limits<double>::infinity())));
    int* guestError = __error_nid_postfix();
    *guestError = 777;
    CHECK(std::isnan(tgamma_nid_postfix(-2.0)) && *guestError == 777);
    CHECK(std::isinf(tgamma_nid_postfix(172.0)) && *guestError == 777);
}
