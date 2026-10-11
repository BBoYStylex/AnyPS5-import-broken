#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>

extern "C" {
char16_t* APS5_VABI wcscat_nid_postfix(char16_t* destination, const char16_t* source) {
    char16_t* end = destination;
    while (*end != 0) ++end;
    do { *end++ = *source; } while (*source++ != 0);
    return destination;
}

char16_t* APS5_VABI wcsncat_nid_postfix(char16_t* destination, const char16_t* source, std::size_t count) {
    char16_t* end = destination;
    while (*end != 0) ++end;
    while (count != 0 && *source != 0) {
        *end++ = *source++;
        --count;
    }
    *end = 0;
    return destination;
}
}
