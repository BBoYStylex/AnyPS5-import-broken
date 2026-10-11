#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cerrno>
extern "C" {
long long APS5_VABI wcstoll_nid_postfix(const char16_t*, char16_t**, int);
unsigned long long APS5_VABI wcstoull_nid_postfix(const char16_t*, char16_t**, int);
std::int64_t APS5_VABI wcstoimax_nid_postfix(const char16_t* text, char16_t** end, int base) {
    char16_t* parsedEnd = nullptr;
    const auto result = wcstoll_nid_postfix(text, &parsedEnd, base);
    if (parsedEnd == text) errno = 22;
    if (end) *end = parsedEnd;
    return result;
}
std::uint64_t APS5_VABI wcstoumax_nid_postfix(const char16_t* text, char16_t** end, int base) {
    char16_t* parsedEnd = nullptr;
    const auto result = wcstoull_nid_postfix(text, &parsedEnd, base);
    if (parsedEnd == text) errno = 22;
    if (end) *end = parsedEnd;
    return result;
}
}
