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
void APS5_VABI bcopy_nid_postfix(const void*, void*, std::size_t);
}

int main() {
    unsigned char bytes[] = {0, 1, 2, 3, 4, 5, 6};
    bcopy_nid_postfix(bytes, bytes + 1, 5);
    const unsigned char forward[] = {0, 0, 1, 2, 3, 4, 6};
    CHECK(std::memcmp(bytes, forward, sizeof(bytes)) == 0);
    bcopy_nid_postfix(bytes + 1, bytes, 5);
    const unsigned char backward[] = {0, 1, 2, 3, 4, 4, 6};
    CHECK(std::memcmp(bytes, backward, sizeof(bytes)) == 0);
    bcopy_nid_postfix(bytes, bytes, sizeof(bytes));
    CHECK(std::memcmp(bytes, backward, sizeof(bytes)) == 0);
    bcopy_nid_postfix(nullptr, nullptr, 0);
    unsigned char destination[3] = {};
    const unsigned char source[] = {0x80, 0, 0xff};
    bcopy_nid_postfix(source, destination, sizeof(source));
    CHECK(std::memcmp(source, destination, sizeof(source)) == 0);
}
