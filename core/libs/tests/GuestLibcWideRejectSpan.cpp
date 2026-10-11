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
std::size_t APS5_VABI wcscspn_nid_postfix(const char16_t*, const char16_t*);
}

int main() {
    CHECK(wcscspn_nid_postfix(u"", u"x") == 0);
    CHECK(wcscspn_nid_postfix(u"abc", u"") == 3);
    CHECK(wcscspn_nid_postfix(u"abc", u"az") == 0);
    CHECK(wcscspn_nid_postfix(u"abc", u"xc") == 2);
    CHECK(wcscspn_nid_postfix(u"abc", u"xyz") == 3);
    const char16_t units[] = {0xffff, 0x8000, 0xd800, 0};
    const char16_t reject[] = {0xd800, 0};
    CHECK(wcscspn_nid_postfix(units, reject) == 2);
    const char16_t bounded[] = {u'a', 0, u'x', 0};
    CHECK(wcscspn_nid_postfix(bounded, u"x") == 1);
}
