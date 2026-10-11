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
void* APS5_VABI memccpy_nid_postfix(void*, const void*, int, std::size_t);
}

int main() {
    const unsigned char source[] = {0x80, 0, 0xff, 4};
    unsigned char destination[6];
    std::memset(destination, 0x55, sizeof(destination));
    CHECK(memccpy_nid_postfix(destination + 1, source, 0, 4) == destination + 3);
    CHECK(destination[0] == 0x55 && destination[1] == 0x80 && destination[2] == 0 && destination[3] == 0x55);
    CHECK(memccpy_nid_postfix(destination, source, -1, 4) == destination + 3);
    CHECK(destination[2] == 0xff);
    CHECK(memccpy_nid_postfix(destination, source, 0x180, 4) == destination + 1);
    CHECK(memccpy_nid_postfix(destination, source, 4, 3) == nullptr);
    CHECK(std::memcmp(destination, source, 3) == 0);
    CHECK(memccpy_nid_postfix(nullptr, nullptr, 0, 0) == nullptr);
}
