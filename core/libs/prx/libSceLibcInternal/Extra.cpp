#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#ifdef _WIN32
#include <windows.h>
#include <malloc.h>
#endif

extern "C" {

double APS5_VABI fabs_nid_postfix(double x) { return std::fabs(x); }
double APS5_VABI sqrt_nid_postfix(double x) { return std::sqrt(x); }
double APS5_VABI fma_nid_postfix(double x, double y, double z) { return std::fma(x, y, z); }
double APS5_VABI log1p_nid_postfix(double x) { return std::log1p(x); }
double APS5_VABI expm1_nid_postfix(double x) { return std::expm1(x); }
double APS5_VABI logb_nid_postfix(double x) { return std::logb(x); }
double APS5_VABI remainder_nid_postfix(double x, double y) { return std::remainder(x, y); }
double APS5_VABI sinh_nid_postfix(double x) { return std::sinh(x); }
double APS5_VABI cosh_nid_postfix(double x) { return std::cosh(x); }
double APS5_VABI tanh_nid_postfix(double x) { return std::tanh(x); }
double APS5_VABI asinh_nid_postfix(double x) { return std::asinh(x); }
double APS5_VABI atanh_nid_postfix(double x) { return std::atanh(x); }
double APS5_VABI sinhf_nid_postfix(float x) { return std::sinhf(x); }
double APS5_VABI coshf_nid_postfix(float x) { return std::coshf(x); }

long double APS5_VABI powl_nid_postfix(long double x, long double y) { return std::powl(x, y); }
long double APS5_VABI exp2l_nid_postfix(long double x) { return std::exp2l(x); }
long double APS5_VABI log10l_nid_postfix(long double x) { return std::log10l(x); }
long double APS5_VABI logl_nid_postfix(long double x) { return std::logl(x); }
long double APS5_VABI atanl_nid_postfix(long double x) { return std::atanl(x); }
long double APS5_VABI asinl_nid_postfix(long double x) { return std::asinl(x); }
long double APS5_VABI tanl_nid_postfix(long double x) { return std::tanl(x); }
long double APS5_VABI sinhl_nid_postfix(long double x) { return std::sinhl(x); }
long double APS5_VABI coshl_nid_postfix(long double x) { return std::coshl(x); }
long double APS5_VABI expl_nid_postfix(long double x) { return std::expl(x); }
long double APS5_VABI truncl_nid_postfix(long double x) { return std::truncl(x); }
long double APS5_VABI ldexpl_nid_postfix(long double x, int exp) { return std::ldexpl(x, exp); }
long double APS5_VABI sinl_nid_postfix(long double x) { return std::sinl(x); }
long double APS5_VABI cosl_nid_postfix(long double x) { return std::cosl(x); }

int APS5_VABI __fpclassifyd_nid_postfix(double x) {
    return std::fpclassify(x);
}

char* tzname_nid_postfix[2] = {const_cast<char*>("UTC"), const_cast<char*>("UTC")};

void* APS5_VABI memrchr_nid_postfix(const void* s, int c, std::size_t n) {
    if (!s || n == 0) return nullptr;
    const auto* bytes = static_cast<const unsigned char*>(s);
    for (std::size_t i = n; i > 0; --i) {
        if (bytes[i - 1] == static_cast<unsigned char>(c))
            return const_cast<void*>(static_cast<const void*>(bytes + i - 1));
    }
    return nullptr;
}

char* APS5_VABI strsep_nid_postfix(char** stringp, const char* delim) {
    if (!stringp || !*stringp) return nullptr;
    char* start = *stringp;
    char* end = start + strcspn(start, delim);
    if (*end != '\0') {
        *end = '\0';
        *stringp = end + 1;
    } else {
        *stringp = nullptr;
    }
    return start;
}

char* APS5_VABI dirname_nid_postfix(char* path) {
    static thread_local char buffer[1024];
    if (!path || !*path) return const_cast<char*>(".");
    std::strncpy(buffer, path, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    std::size_t length = std::strlen(buffer);
    while (length > 0 && buffer[length - 1] != '/' && buffer[length - 1] != '\\') --length;
    while (length > 1 && (buffer[length - 1] == '/' || buffer[length - 1] == '\\')) --length;
    if (length == 0) return const_cast<char*>("/");
    if (length == 1 && (buffer[0] == '/' || buffer[0] == '\\')) return const_cast<char*>("/");
    buffer[length] = '\0';
    if (buffer[0] == '\0') return const_cast<char*>(".");
    return buffer;
}

int APS5_VABI getc_unlocked_nid_postfix(FileStream* stream) {
    if (!stream) return -1;
    const int result = std::fgetc(GetNativeStream(stream));
    stream->SyncStatus();
    return result;
}

int APS5_VABI putc_unlocked_nid_postfix(int value, FileStream* stream) {
    if (!stream) return -1;
    const int result = std::fputc(value, GetNativeStream(stream));
    stream->SyncStatus();
    return result;
}

int APS5_VABI ftrylockfile_nid_postfix(FileStream*) { return 0; }

std::size_t APS5_VABI malloc_usable_size_nid_postfix(void* pointer) {
    if (!pointer) return 0;
#ifdef _WIN32
    return static_cast<std::size_t>(_msize(pointer));
#else
    return malloc_usable_size(pointer);
#endif
}

const char* APS5_VABI getprogname_nid_postfix(void) {
    static char name[256] = {};
    if (name[0] == '\0') {
#ifdef _WIN32
        wchar_t wide[MAX_PATH]{};
        GetModuleFileNameW(nullptr, wide, MAX_PATH);
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, name, sizeof(name), nullptr, nullptr);
#else
        std::strncpy(name, program_invocation_short_name, sizeof(name) - 1);
#endif
        if (char* slash = std::strrchr(name, '/')) {
            std::memmove(name, slash + 1, std::strlen(slash + 1) + 1);
        }
        if (char* dot = std::strrchr(name, '.')) *dot = '\0';
    }
    return name;
}

std::size_t APS5_VABI mbstowcs_nid_postfix(std::uint32_t* destination, const char* source, std::size_t maximum) {
    if (!source) return static_cast<std::size_t>(-1);
    const auto* cursor = reinterpret_cast<const unsigned char*>(source);
    std::size_t written = 0;
    auto emit = [&](std::uint32_t codepoint) {
        if (destination && written < maximum) destination[written] = codepoint;
        ++written;
    };
    while (*cursor != 0) {
        unsigned int codepoint = 0;
        int extra = 0;
        if (*cursor < 0x80) {
            codepoint = *cursor++;
        } else if ((*cursor & 0xE0) == 0xC0) {
            codepoint = *cursor++ & 0x1F;
            extra = 1;
        } else if ((*cursor & 0xF0) == 0xE0) {
            codepoint = *cursor++ & 0x0F;
            extra = 2;
        } else if ((*cursor & 0xF8) == 0xF0) {
            codepoint = *cursor++ & 0x07;
            extra = 3;
        } else {
            return static_cast<std::size_t>(-1);
        }
        for (int i = 0; i < extra; ++i) {
            if ((*cursor & 0xC0) != 0x80) return static_cast<std::size_t>(-1);
            codepoint = (codepoint << 6) | (*cursor++ & 0x3F);
        }
        emit(codepoint);
    }
    return written;
}

namespace {
struct LongOption {
    const char* name;
    int has_arg;
    int* flag;
    int val;
};
}

int optind_nid_postfix = 1;
char* optarg_nid_postfix = nullptr;

int APS5_VABI getopt_long_nid_postfix(int argc, char* const argv[], const char* optstring,
                                      const LongOption* longopts, int* longindex) {
    static const char* nextChar = nullptr;
    if (!argv) return -1;
    char* current = nullptr;
    if (nextChar && *nextChar) {
        current = const_cast<char*>(nextChar - 1);  // position of this cluster's '-'
    } else {
        if (optind_nid_postfix >= argc) return -1;
        current = argv[optind_nid_postfix];
        if (current[0] != '-' || current[1] == '\0') return -1;
        if (current[1] == '-' && current[2] == '\0') {
            ++optind_nid_postfix;
            return -1;
        }
        nextChar = current + 1;
    }
    if (current[0] == '-' && current[1] == '-') {
        nextChar = nullptr;
        ++optind_nid_postfix;
        const char* name = current + 2;
        if (longopts) {
            for (int i = 0; longopts[i].name; ++i) {
                const std::size_t nameLength = std::strlen(longopts[i].name);
                if (std::strncmp(name, longopts[i].name, nameLength) != 0) continue;
                const bool exactEnd = name[nameLength] == '\0';
                const bool hasEqual = name[nameLength] == '=';
                if (!exactEnd && !hasEqual) continue;
                if (hasEqual) {
                    optarg_nid_postfix = const_cast<char*>(name + nameLength + 1);
                } else if (longopts[i].has_arg != 0) {
                    if (optind_nid_postfix >= argc) return '?';
                    optarg_nid_postfix = argv[optind_nid_postfix++];
                }
                if (longindex) *longindex = i;
                if (longopts[i].flag) {
                    *longopts[i].flag = longopts[i].val;
                    return 0;
                }
                return longopts[i].val;
            }
        }
        return '?';
    }
    const char option = *nextChar++;
    if (*nextChar == '\0') {
        nextChar = nullptr;
        ++optind_nid_postfix;
    }
    if (!optstring) return '?';
    const char* position = std::strchr(optstring, option);
    if (!position) return '?';
    if (position[1] == ':') {
        if (nextChar) {
            optarg_nid_postfix = const_cast<char*>(nextChar);
            nextChar = nullptr;
            ++optind_nid_postfix;
        } else {
            if (optind_nid_postfix >= argc) return '?';
            optarg_nid_postfix = argv[optind_nid_postfix++];
        }
    }
    return option;
}

}
