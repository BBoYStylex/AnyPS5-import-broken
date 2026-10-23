#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <stdexcept>

extern "C" std::int64_t APS5_VABI sceFiosTimeIntervalFromNanoseconds(std::int64_t nanoseconds);
extern "C" std::int64_t APS5_VABI sceFiosTimeIntervalToNanoseconds(std::int64_t interval);

#ifdef ANYPS5_STANDALONE_TEST
extern "C" [[noreturn]] void NotImplemented_nid_no_patch(const char* function) {
    throw std::runtime_error(function);
}
#endif

namespace {
void Require(bool value) {
    if (!value) std::abort();
}
}

int main() {
    Require(sceFiosTimeIntervalFromNanoseconds(0) == 0);
    Require(sceFiosTimeIntervalFromNanoseconds(999) == 0);
    Require(sceFiosTimeIntervalFromNanoseconds(1000) == 1);
    Require(sceFiosTimeIntervalFromNanoseconds(123456789) == 123456);
    Require(sceFiosTimeIntervalFromNanoseconds(-999) == 0);
    Require(sceFiosTimeIntervalFromNanoseconds(-1000) == -1);
    Require(sceFiosTimeIntervalFromNanoseconds(-123456789) == -123456);
    Require(sceFiosTimeIntervalFromNanoseconds(std::numeric_limits<std::int64_t>::max()) == 9223372036854775LL);
    Require(sceFiosTimeIntervalFromNanoseconds(std::numeric_limits<std::int64_t>::min()) == -9223372036854775LL);

    for (const auto interval : std::array<std::int64_t, 7>{-9223372036854775LL, -1000000, -1, 0, 1, 1000000, 9223372036854775LL}) {
        Require(sceFiosTimeIntervalFromNanoseconds(sceFiosTimeIntervalToNanoseconds(interval)) == interval);
    }
    for (const auto interval : std::array<std::int64_t, 4>{std::numeric_limits<std::int64_t>::min(), -9223372036854776LL, 9223372036854776LL, std::numeric_limits<std::int64_t>::max()}) {
        bool threw = false;
        try {
            (void)sceFiosTimeIntervalToNanoseconds(interval);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Require(threw);
    }
}
