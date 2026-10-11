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
double APS5_VABI copysign_nid_postfix(double, double);
float APS5_VABI copysignf_nid_postfix(float, float);
long double APS5_VABI copysignl_nid_postfix(long double, long double);
}

int main() {
    CHECK(copysign_nid_postfix(-3.0, 1.0) == 3.0);
    CHECK(copysign_nid_postfix(3.0, -0.0) == -3.0);
    CHECK(std::bit_cast<std::uint64_t>(copysign_nid_postfix(0.0, -1.0)) == 0x8000000000000000ull);
    CHECK(std::bit_cast<std::uint32_t>(copysignf_nid_postfix(-0.0f, 1.0f)) == 0);
    const auto payload = std::bit_cast<double>(0x7ff8000000012345ull);
    CHECK(std::bit_cast<std::uint64_t>(copysign_nid_postfix(payload, -1.0)) == 0xfff8000000012345ull);
    const auto floatPayload = std::bit_cast<float>(0x7fc01234u);
    CHECK(std::bit_cast<std::uint32_t>(copysignf_nid_postfix(floatPayload, -1.0f)) == 0xffc01234u);
    CHECK(copysignl_nid_postfix(0x1.000000000000001p0L, -0.0L) == -0x1.000000000000001p0L);
    CHECK(std::signbit(copysignl_nid_postfix(0.0L, -1.0L)));
    std::feclearexcept(FE_ALL_EXCEPT);
    const auto signaling = std::bit_cast<double>(0x7ff0000000001234ull);
    CHECK(std::bit_cast<std::uint64_t>(copysign_nid_postfix(signaling, -1.0)) == 0xfff0000000001234ull);
    CHECK(std::fetestexcept(FE_ALL_EXCEPT) == 0);
}
