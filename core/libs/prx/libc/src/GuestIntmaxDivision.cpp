#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstddef>
struct GuestIntmaxDivision { std::int64_t quotient; std::int64_t remainder; };
static_assert(sizeof(GuestIntmaxDivision) == 16);
static_assert(offsetof(GuestIntmaxDivision, remainder) == 8);
extern "C" GuestIntmaxDivision APS5_VABI imaxdiv_nid_postfix(std::int64_t numerator, std::int64_t denominator) {
    return {numerator / denominator, numerator % denominator};
}
