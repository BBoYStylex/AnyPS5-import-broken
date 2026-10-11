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
std::int64_t APS5_VABI labs_nid_postfix(std::int64_t);
std::int64_t APS5_VABI llabs_nid_postfix(std::int64_t);
std::int64_t APS5_VABI imaxabs_nid_postfix(std::int64_t);
}

int main() {
    using Operation = std::int64_t (APS5_VABI*)(std::int64_t);
    for (Operation operation : {&labs_nid_postfix, &llabs_nid_postfix, &imaxabs_nid_postfix}) {
        CHECK(operation(0) == 0);
        CHECK(operation(1) == 1 && operation(-1) == 1);
        CHECK(operation(2147483648LL) == 2147483648LL);
        CHECK(operation(-2147483648LL) == 2147483648LL);
        CHECK(operation(INT64_MAX) == INT64_MAX);
        CHECK(operation(INT64_MIN + 1) == INT64_MAX);
    }
}
