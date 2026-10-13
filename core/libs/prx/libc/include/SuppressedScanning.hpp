#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_SUPPRESSEDSCANNING_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_SUPPRESSEDSCANNING_HPP

#include "WindowsFormatting.hpp"
#include <algorithm>
#include <charconv>
#include <cstdarg>
#include <cstring>

namespace LibcDetail {

template<class TScanner>
inline int ScanSuppressedArguments_nid_no_patch(const char* format, std::va_list source, TScanner scan) {
    if (!format || !source) { errno = 22; return EOF; }
    if (!std::strchr(format, '*')) return scan(format, source);
    std::string translated;
    std::vector<size_t> positions;
    std::vector<int> suppressedEnds(std::count(format, format + std::strlen(format), '%'), -1);
    size_t suppressedCount = 0, nextPosition = 1, maximumPosition = 0;
    const char* cursor = format;
    while (*cursor) {
        const char value = *cursor++;
        translated += value;
        if (value != '%') continue;
        if (*cursor == '%') { translated += *cursor++; continue; }
        const char* specifier = cursor;
        const char* digits = cursor;
        while (*cursor >= '0' && *cursor <= '9') ++cursor;
        size_t position = 0;
        if (*cursor == '$') {
            const auto parsed = std::from_chars(digits, cursor, position);
            if (parsed.ec != std::errc{} || position == 0) return scan(format, source);
            specifier = ++cursor;
        } else cursor = specifier;
        bool suppressed = false;
        while (*cursor && std::strchr("*'I", *cursor)) {
            suppressed |= *cursor == '*';
            ++cursor;
        }
        while (*cursor >= '0' && *cursor <= '9') ++cursor;
        while (*cursor && std::strchr("hljztLqm", *cursor)) ++cursor;
        const char conversion = *cursor;
        if (!conversion || !std::strchr("diouxXaAeEfFgGcCsSpn[", conversion)) return scan(format, source);
        ++cursor;
        if (conversion == '[') {
            if (*cursor == '^') ++cursor;
            if (*cursor == ']') ++cursor;
            while (*cursor && *cursor != ']') ++cursor;
            if (*cursor != ']') return scan(format, source);
            ++cursor;
        }
        translated.append(specifier, cursor);
        if (!suppressed) {
            if (!position) position = nextPosition++;
            maximumPosition = std::max(maximumPosition, position);
            positions.push_back(position);
        } else if (conversion != 'n') {
            translated += "%n";
            positions.push_back(0);
            ++suppressedCount;
        }
    }
    if (!suppressedCount) return scan(format, source);
    FormatArguments arguments(source);
    std::vector<void*> original;
    original.reserve(maximumPosition);
    for (size_t index = 0; index < maximumPosition; ++index) original.push_back(arguments.Next<void*>());
    std::vector<void*> pointers;
    pointers.reserve(positions.size());
    size_t suppressedIndex = 0;
    for (const size_t position : positions)
        pointers.push_back(position ? original[position - 1] : &suppressedEnds[suppressedIndex++]);
    const VaList supplied{48, 176, pointers.data(), nullptr};
    static_assert(sizeof(std::va_list) == sizeof(VaList));
    std::va_list forwarded;
    std::memcpy(&forwarded, &supplied, sizeof(supplied));
    const int result = scan(translated.c_str(), forwarded);
    va_end(forwarded);
    return result == EOF && std::any_of(suppressedEnds.begin(), suppressedEnds.begin() + suppressedCount,
        [](int end) { return end >= 0; }) ? 0 : result;
}

}

#endif
