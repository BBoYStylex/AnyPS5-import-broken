#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
extern "C" float APS5_VABI tgammaf_nid_postfix(float);
extern "C" int* APS5_VABI __error_nid_postfix();
static void Require(bool value) {
    if (!value) std::abort();
}
int main() {
    float (APS5_VABI *volatile call)(float) = tgammaf_nid_postfix;
    Require(call(1.0f) == 1.0f && call(5.0f) == 24.0f);
    const auto half = call(0.5f);
    Require(std::abs(half - std::sqrt(std::acos(-1.0f))) < 0.000002f);
    Require(std::isnan(call(std::numeric_limits<float>::quiet_NaN())));
    Require(std::isinf(call(std::numeric_limits<float>::infinity())));
    std::feclearexcept(FE_ALL_EXCEPT);
    *__error_nid_postfix() = 0;
    Require(std::isinf(call(0.0f)));
    Require(std::fetestexcept(FE_DIVBYZERO));
    if (math_errhandling & MATH_ERRNO) Require(*__error_nid_postfix() == ERANGE);
    std::feclearexcept(FE_ALL_EXCEPT);
    *__error_nid_postfix() = 0;
    Require(std::isnan(call(-1.0f)));
    Require(std::fetestexcept(FE_INVALID));
    if (math_errhandling & MATH_ERRNO) Require(*__error_nid_postfix() == EDOM);
    Require(std::signbit(call(-0.5f)));
    std::puts("tgammaf: values, signs, NaN, infinity, poles, errno and SysV ABI passed");
}
