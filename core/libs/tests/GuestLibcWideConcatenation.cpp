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
char16_t* APS5_VABI wcscat_nid_postfix(char16_t*, const char16_t*);
char16_t* APS5_VABI wcsncat_nid_postfix(char16_t*, const char16_t*, std::size_t);
}

int main() {
    static_assert(sizeof(char16_t) == 2);
    char16_t text[16] = u"A";
    CHECK(wcscat_nid_postfix(text, u"\u03a9\U0001f600") == text);
    CHECK(text[0] == u'A' && text[1] == 0x03a9 && text[2] == 0xd83d && text[3] == 0xde00 && text[4] == 0);
    CHECK(wcscat_nid_postfix(text, u"") == text && text[4] == 0);
    const char16_t unterminated[] = {u'B', u'C', u'D'};
    CHECK(wcsncat_nid_postfix(text, unterminated, 2) == text);
    CHECK(text[4] == u'B' && text[5] == u'C' && text[6] == 0);
    text[7] = 0x1234;
    CHECK(wcsncat_nid_postfix(text, unterminated, 0) == text && text[6] == 0 && text[7] == 0x1234);
    CHECK(wcsncat_nid_postfix(text, u"Z", 4) == text && text[6] == u'Z' && text[7] == 0);
    char16_t empty[2] = {};
    CHECK(wcscat_nid_postfix(empty, u"Q") == empty && empty[0] == u'Q' && empty[1] == 0);
}
