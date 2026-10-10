#include "prx/libc/include/general/VabiMacros.hpp"
#include <bit>
#include <cerrno>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <limits>

extern "C" {
double APS5_VABI nextafter_nid_postfix(double, double);
int* APS5_VABI __error_nid_postfix();
}

static void Require(bool value, const char* message) {
    if (!value) {
        std::fprintf(stderr, "%s\n", message);
        std::abort();
    }
}

template<typename TFloat, typename TBits>
static void CheckNextafter(TFloat (APS5_VABI *next)(TFloat, TFloat)) {
    const auto infinity = std::numeric_limits<TFloat>::infinity();
    const auto nan = std::numeric_limits<TFloat>::quiet_NaN();
    const auto one = std::bit_cast<TBits>(TFloat{1});
    Require(std::bit_cast<TBits>(next(TFloat{1}, TFloat{2})) == one + 1, "nextafter: upward neighbour");
    Require(std::bit_cast<TBits>(next(TFloat{1}, TFloat{0})) == one - 1, "nextafter: downward neighbour");
    Require(next(TFloat{1}, TFloat{1}) == TFloat{1}, "nextafter: equal arguments");
    Require(std::signbit(next(TFloat{0}, -TFloat{0})), "nextafter: negative target zero");
    Require(!std::signbit(next(-TFloat{0}, TFloat{0})), "nextafter: positive target zero");
    Require(next(TFloat{0}, TFloat{1}) == std::numeric_limits<TFloat>::denorm_min(), "nextafter: positive subnormal");
    Require(next(TFloat{0}, -TFloat{1}) == -std::numeric_limits<TFloat>::denorm_min(), "nextafter: negative subnormal");
    Require(next(infinity, TFloat{0}) == std::numeric_limits<TFloat>::max(), "nextafter: infinity to finite");
    Require(std::isnan(next(nan, TFloat{1})) && std::isnan(next(TFloat{1}, nan)), "nextafter: NaN propagation");
    *__error_nid_postfix() = 13;
    Require(next(TFloat{1}, TFloat{2}) > TFloat{1} && *__error_nid_postfix() == 13, "nextafter: preserve errno on success");
    std::feclearexcept(FE_ALL_EXCEPT);
    *__error_nid_postfix() = 0;
    Require(std::isinf(next(std::numeric_limits<TFloat>::max(), infinity)), "nextafter: overflow value");
    if (math_errhandling & MATH_ERREXCEPT)
        Require(std::fetestexcept(FE_OVERFLOW) != 0, "nextafter: overflow exception");
    if (math_errhandling & MATH_ERRNO)
        Require(*__error_nid_postfix() == 34, "nextafter: overflow errno");
    std::feclearexcept(FE_ALL_EXCEPT);
    *__error_nid_postfix() = 0;
    Require(next(std::numeric_limits<TFloat>::denorm_min(), TFloat{0}) == TFloat{0}, "nextafter: underflow value");
    if (math_errhandling & MATH_ERREXCEPT)
        Require(std::fetestexcept(FE_UNDERFLOW) != 0, "nextafter: underflow exception");
    if (math_errhandling & MATH_ERRNO)
        Require(*__error_nid_postfix() == 34, "nextafter: underflow errno");
}

int main() {
    CheckNextafter<double, std::uint64_t>(nextafter_nid_postfix);
}
