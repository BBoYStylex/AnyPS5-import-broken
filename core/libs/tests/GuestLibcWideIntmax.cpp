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
std::int64_t APS5_VABI wcstoimax_nid_postfix(const char16_t*, char16_t**, int);
std::uint64_t APS5_VABI wcstoumax_nid_postfix(const char16_t*, char16_t**, int);
int* APS5_VABI __error_nid_postfix();
}

int main() {
    int* error = __error_nid_postfix();
    char16_t* end = nullptr;
    const char16_t signedText[] = u" -9223372036854775808!";
    CHECK(wcstoimax_nid_postfix(signedText, &end, 10) == INT64_MIN && *end == u'!');
    CHECK(wcstoimax_nid_postfix(u"9223372036854775807", nullptr, 10) == INT64_MAX);
    CHECK(wcstoumax_nid_postfix(u"18446744073709551615", nullptr, 10) == UINT64_MAX);
    CHECK(wcstoumax_nid_postfix(u"-1", nullptr, 10) == UINT64_MAX);
    CHECK(wcstoimax_nid_postfix(u"0x7fffffffffffffff", nullptr, 0) == INT64_MAX);
    CHECK(wcstoimax_nid_postfix(u"077", nullptr, 0) == 63);
    *error = 123;
    CHECK(wcstoimax_nid_postfix(u"2147483648", nullptr, 10) == 2147483648LL && *error == 123);
    *error = 0;
    CHECK(wcstoimax_nid_postfix(u"9223372036854775808", nullptr, 10) == INT64_MAX && *error == 34);
    *error = 0;
    CHECK(wcstoumax_nid_postfix(u"18446744073709551616", nullptr, 10) == UINT64_MAX && *error == 34);
    const char16_t invalid[] = u"x";
    *error = 0;
    CHECK(wcstoimax_nid_postfix(invalid, &end, 10) == 0 && end == invalid && *error == 22);
    *error = 0;
    CHECK(wcstoumax_nid_postfix(invalid, &end, 10) == 0 && end == invalid && *error == 22);
    const char16_t trailing[] = {u'4', u'2', 0x03a9, 0};
    CHECK(wcstoimax_nid_postfix(trailing, &end, 10) == 42 && end == trailing + 2);
}
