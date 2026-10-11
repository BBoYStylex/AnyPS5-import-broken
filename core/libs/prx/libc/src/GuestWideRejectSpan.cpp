#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
extern "C" std::size_t APS5_VABI wcscspn_nid_postfix(const char16_t* text, const char16_t* rejected) {
    std::size_t length = 0;
    for (; text[length] != 0; ++length) {
        for (const char16_t* character = rejected; *character != 0; ++character) {
            if (text[length] == *character) return length;
        }
    }
    return length;
}
