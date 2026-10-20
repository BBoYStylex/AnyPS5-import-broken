#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libc/include/SonyCrt.hpp"
#include <cfenv>
#include <climits>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <limits>

extern "C" {
SonyLconv* APS5_VABI sceAmprLocaleconv(void);
double APS5_VABI sceAmprSinh(double x, double y);
char* APS5_VABI sceAmprStrtokS(char* str, std::size_t* remaining, const char* delim, char** context);
int APS5_VABI sceAmprCtimeS(char* buffer, std::size_t size, const std::int64_t* time);
int APS5_VABI sceAmprWcscpyS(std::uint16_t* dest, std::size_t size, const std::uint16_t* src);
void* APS5_VABI sceAmprSetConstraintHandlerS(void* handler);
int APS5_VABI sceAmprFltrounds(void);
void APS5_VABI sceAmprIgnoreHandlerS(const char* message, void* pointer, int error);
std::size_t APS5_VABI sceAmprMbstowcs(std::uint16_t* dest, const char* src, std::size_t size);
int APS5_VABI sceAmprAsctimeS(char* buffer, std::size_t size, const GuestTm* time);
int APS5_VABI ctime_s_nid_postfix(char* buffer, std::size_t size, const std::int64_t* time);
int APS5_VABI asctime_s_nid_postfix(char* buffer, std::size_t size, const GuestTm* time);
void* APS5_VABI set_constraint_handler_s_nid_postfix(void* handler);
std::size_t APS5_VABI mbstowcs_nid_postfix(std::uint16_t* dest, const char* src, std::size_t size);
char* APS5_VABI strtok_s_nid_postfix(char* str, std::size_t* remaining, const char* delim, char** context);
int APS5_VABI __fpclassifyd_nid_postfix(double value);
unsigned __int128 APS5_VABI __udivti3_nid_postfix(unsigned __int128 dividend, unsigned __int128 divisor);
__int128 APS5_VABI __divti3_nid_postfix(__int128 dividend, __int128 divisor);
unsigned __int128 APS5_VABI __umodti3_nid_postfix(unsigned __int128 dividend, unsigned __int128 divisor);
__int128 APS5_VABI __modti3_nid_postfix(__int128 dividend, __int128 divisor);
SonyLconv* APS5_VABI localeconv_nid_postfix(void);
int APS5_VABI sceAmprFpclassifyd(double value);
unsigned int APS5_VABI _ZSt14_Random_devicev_nid_postfix();
}

#define Require(condition) do { if (!(condition)) { std::fprintf(stderr, "guest_crt: line %d: %s\n", __LINE__, #condition); std::abort(); } } while (0)

static int g_handlerCalls = 0;
static const char* g_handlerMessage = nullptr;
static void* g_handlerPointer = nullptr;
static int g_handlerError = 0;

static void APS5_VABI CountingHandler(const char* message, void* pointer, int error) {
    g_handlerMessage = message;
    g_handlerPointer = pointer;
    g_handlerError = error;
    ++g_handlerCalls;
}

int main() {
    Require(sceAmprFpclassifyd(std::numeric_limits<double>::quiet_NaN()) == 2);
    Require(sceAmprFpclassifyd(std::numeric_limits<double>::infinity()) == 1);
    Require(sceAmprFpclassifyd(0.0) == 0);
    Require(sceAmprFpclassifyd(std::numeric_limits<double>::denorm_min()) == -2);
    Require(sceAmprFpclassifyd(1.5) == -1);
    Require(__fpclassifyd_nid_postfix(0.0) == 0);

    Require(sceAmprFltrounds() == 1);
    Require(std::fesetround(FE_DOWNWARD) == 0);
    Require(sceAmprFltrounds() == 3);
    Require(std::fesetround(FE_TONEAREST) == 0);
    Require(sceAmprFltrounds() == 1);

    Require(sceAmprSinh(1.0, 2.0) == 2.0 * std::sinh(1.0));
    Require(sceAmprSinh(0.0, 5.0) == 0.0);

    SonyLconv* locale = sceAmprLocaleconv();
    Require(reinterpret_cast<std::uintptr_t>(&locale->decimal_point) - reinterpret_cast<std::uintptr_t>(locale) == 0x48);
    Require(std::strcmp(locale->decimal_point, ".") == 0);
    Require(std::memcmp(localeconv_nid_postfix()->chars, locale->chars, sizeof(locale->chars)) == 0);
    Require(std::strcmp(localeconv_nid_postfix()->decimal_point, ".") == 0);
    for (const char* text : locale->monetary) Require(text != nullptr && text[0] == '\0');
    for (char value : locale->chars) Require(value == CHAR_MAX);
    Require(std::strcmp(locale->false_text, "false") == 0 && std::strcmp(locale->true_text, "true") == 0);
    Require(locale->grouping[0] == '\0' && locale->thousands_sep[0] == '\0');

    Require(__udivti3_nid_postfix(100, 3) == 33);
    Require(SonyUdivti3(100, 3) == 33);
    Require(SonyUdivti3(0, 5) == 0);
    Require(SonyUdivti3((unsigned __int128)1 << 100, 3) == (((unsigned __int128)1 << 100) / 3));
    bool trapped = false;
    try {
        SonyUdivti3(1, 0);
    } catch (const std::exception&) {
        trapped = true;
    }
    Require(trapped);

    Require(__divti3_nid_postfix(100, 3) == 33);
    Require(__divti3_nid_postfix(-100, 3) == -33);
    Require(__divti3_nid_postfix(100, -3) == -33);
    Require(__divti3_nid_postfix(-100, -3) == 33);
    Require(SonyDivti3(0, 5) == 0);
    const __int128 large = (__int128)1 << 100;
    Require(SonyDivti3(large, 3) == large / 3);
    Require(SonyDivti3(-large, 3) == -(large / 3));
    Require(SonyDivti3(large, -3) == -(large / 3));
    Require(SonyDivti3(-large, -3) == large / 3);
    const __int128 minimum = (__int128)((unsigned __int128)1 << 127);
    const __int128 maximum = (__int128)(((unsigned __int128)1 << 127) - 1);
    Require(SonyDivti3(minimum, 1) == minimum);
    Require(SonyDivti3(minimum, -1) == minimum);
    Require(SonyDivti3(maximum, -1) == -maximum);
    trapped = false;
    try {
        SonyDivti3(1, 0);
    } catch (const std::exception&) {
        trapped = true;
    }
    Require(trapped);

    Require(__modti3_nid_postfix(100, 3) == 1);
    Require(__modti3_nid_postfix(-100, 3) == -1);
    Require(__modti3_nid_postfix(100, -3) == 1);
    Require(__modti3_nid_postfix(-100, -3) == -1);
    Require(SonyModti3(0, 5) == 0);
    Require(SonyModti3(3, 100) == 3);
    Require(SonyModti3(-3, 100) == -3);
    Require(SonyModti3(3, -100) == 3);
    Require(SonyModti3(-3, -100) == -3);
    Require(SonyModti3(large, 3) == large % 3);
    Require(SonyModti3(-large, 3) == -(large % 3));
    Require(SonyModti3(large, -3) == large % 3);
    Require(SonyModti3(-large, -3) == -(large % 3));
    Require(SonyModti3(minimum, 1) == 0);
    Require(SonyModti3(minimum, -1) == 0);
    Require(SonyModti3(maximum, 1) == 0);
    Require(SonyModti3(maximum, -1) == 0);
    trapped = false;
    try {
        SonyModti3(1, 0);
    } catch (const std::exception&) {
        trapped = true;
    }
    Require(trapped);

    const unsigned __int128 unsignedMaximum = ~(unsigned __int128)0;
    const unsigned __int128 largeDivisor = ((unsigned __int128)1 << 64) + 3;
    Require(__umodti3_nid_postfix(100, 3) == 1);
    Require(__umodti3_nid_postfix(0, 5) == 0);
    Require(__umodti3_nid_postfix(3, 100) == 3);
    Require(__umodti3_nid_postfix(unsignedMaximum, 1) == 0);
    Require(__umodti3_nid_postfix(unsignedMaximum, unsignedMaximum) == 0);
    Require(__umodti3_nid_postfix(largeDivisor, largeDivisor) == 0);
    Require(__umodti3_nid_postfix(unsignedMaximum, 10) == 5);
    Require(__umodti3_nid_postfix((unsigned __int128)1 << 100, 3) == 1);
    Require(__umodti3_nid_postfix(unsignedMaximum, (unsigned __int128)1 << 64) == 0xFFFFFFFFFFFFFFFFull);
    Require(__umodti3_nid_postfix(unsignedMaximum, largeDivisor) == 8);
    Require(__umodti3_nid_postfix((unsigned __int128)1 << 127, largeDivisor) == 0x8000000000000006ull);
    Require(__umodti3_nid_postfix(unsignedMaximum - 1, unsignedMaximum) == unsignedMaximum - 1);
    Require(__umodti3_nid_postfix(unsignedMaximum, (unsigned __int128)1 << 127) == ((unsigned __int128)1 << 127) - 1);
    trapped = false;
    try {
        __umodti3_nid_postfix(((unsigned __int128)0x1234 << 64) | 0x5678, 0);
    } catch (const std::runtime_error& error) {
        trapped = std::strcmp(error.what(), "__umodti3: integer divide by zero (0x00000000000012340000000000005678 % 0)") == 0;
    }
    Require(trapped);

    void* previous = sceAmprSetConstraintHandlerS(reinterpret_cast<void*>(&CountingHandler));
    Require(previous != nullptr);
    Require(previous != reinterpret_cast<void*>(&CountingHandler));
    const std::uint16_t hello[] = {'h', 'i', 0};
    std::uint16_t buffer[8] = {};
    Require(sceAmprWcscpyS(buffer, 8, hello) == 0);
    Require(buffer[0] == 'h' && buffer[1] == 'i' && buffer[2] == 0);
    const std::uint16_t longText[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 0};
    g_handlerCalls = 0;
    Require(sceAmprWcscpyS(buffer, 4, longText) == ERANGE);
    Require(g_handlerCalls == 1);
    Require(sceAmprWcscpyS(nullptr, 4, longText) == EINVAL);
    previous = sceAmprSetConstraintHandlerS(nullptr);
    Require(previous == reinterpret_cast<void*>(&CountingHandler));
    previous = sceAmprSetConstraintHandlerS(reinterpret_cast<void*>(&CountingHandler));
    Require(previous != nullptr && previous != reinterpret_cast<void*>(&CountingHandler));
    sceAmprIgnoreHandlerS("msg", nullptr, 0);

    void* libcHandler = set_constraint_handler_s_nid_postfix(reinterpret_cast<void*>(&CountingHandler));
    char text[64];
    GuestTm known{};
    known.tm_year = 70;
    known.tm_mon = 0;
    known.tm_mday = 1;
    known.tm_wday = 4;
    Require(sceAmprAsctimeS(text, sizeof(text), &known) == 0);
    Require(std::strcmp(text, "Thu Jan  1 00:00:00 1970\n") == 0);
    std::memset(text, 0, sizeof(text));
    Require(asctime_s_nid_postfix(text, sizeof(text), &known) == 0);
    Require(std::strcmp(text, "Thu Jan  1 00:00:00 1970\n") == 0);
    known.tm_wday = 7;
    Require(sceAmprAsctimeS(text, sizeof(text), &known) == EINVAL);
    known.tm_wday = 4;
    Require(sceAmprAsctimeS(nullptr, sizeof(text), &known) == EINVAL);
    Require(sceAmprAsctimeS(text, sizeof(text), nullptr) == EINVAL);
    Require(sceAmprAsctimeS(text, 25, &known) == EINVAL);
    const std::int64_t epoch = 0;
    Require(sceAmprCtimeS(text, sizeof(text), &epoch) == 0);
    Require(std::strlen(text) == 25 && text[24] == '\n');
    char libcText[64];
    Require(ctime_s_nid_postfix(libcText, sizeof(libcText), &epoch) == 0);
    Require(std::strcmp(libcText, text) == 0);
    Require(sceAmprCtimeS(text, 25, &epoch) == EINVAL);
    Require(sceAmprCtimeS(text, sizeof(text), nullptr) == EINVAL);

    using AsctimeFunction = int (APS5_VABI *)(char*, std::size_t, const GuestTm*);
    for (AsctimeFunction format : {asctime_s_nid_postfix, sceAmprAsctimeS}) {
        const auto invalid = [&](const GuestTm* time, std::size_t size, bool clear, const char* message) {
            std::memset(text, '!', sizeof(text));
            g_handlerCalls = 0;
            Require(format(text, size, time) == EINVAL);
            Require(g_handlerCalls == 1 && g_handlerError == EINVAL && g_handlerPointer == nullptr);
            Require(std::strcmp(g_handlerMessage, message) == 0);
            Require(text[0] == (clear ? '\0' : '!') && text[1] == '!');
        };
        struct InvalidField {
            int GuestTm::* member;
            int value;
        };
        const InvalidField fields[] = {
            {&GuestTm::tm_sec, -1}, {&GuestTm::tm_sec, 61},
            {&GuestTm::tm_min, -1}, {&GuestTm::tm_min, 60},
            {&GuestTm::tm_hour, -1}, {&GuestTm::tm_hour, 24},
            {&GuestTm::tm_mday, 0}, {&GuestTm::tm_mday, 32},
            {&GuestTm::tm_mon, -1}, {&GuestTm::tm_mon, 12},
            {&GuestTm::tm_wday, -1}, {&GuestTm::tm_wday, 7},
            {&GuestTm::tm_yday, -1}, {&GuestTm::tm_yday, 366},
            {&GuestTm::tm_year, -1901}, {&GuestTm::tm_year, 8100},
            {&GuestTm::tm_year, INT_MIN}, {&GuestTm::tm_year, INT_MAX}
        };
        for (const auto& field : fields) {
            GuestTm bad = known;
            bad.*(field.member) = field.value;
            invalid(&bad, sizeof(text), true, "asctime_s: bad argument");
        }
        invalid(nullptr, sizeof(text), true, "asctime_s: bad argument");
        invalid(&known, 1, true, "asctime_s: bad argument");
        invalid(&known, 25, true, "asctime_s: bad argument");
        invalid(&known, 0, false, "asctime_s: bad destination argument");
        invalid(nullptr, 0, false, "asctime_s: bad destination argument");
        invalid(&known, std::size_t{1} << 61, false, "asctime_s: bad destination argument");
        invalid(&known, std::numeric_limits<std::size_t>::max(), false, "asctime_s: bad destination argument");
        g_handlerCalls = 0;
        Require(format(nullptr, 26, &known) == EINVAL && g_handlerCalls == 1);
        Require(std::strcmp(g_handlerMessage, "asctime_s: bad destination argument") == 0);
        Require(format(text, (std::size_t{1} << 61) - 1, &known) == 0);
        GuestTm edge = known;
        edge.tm_year = -1900;
        edge.tm_sec = 60;
        edge.tm_yday = 365;
        edge.tm_isdst = INT_MAX;
        std::memset(text, '!', sizeof(text));
        Require(format(text, 26, &edge) == 0);
        Require(std::strcmp(text, "Thu Jan  1 00:00:60 0000\n") == 0 && text[26] == '!');
        edge.tm_year = 8099;
        edge.tm_mon = 11;
        edge.tm_mday = 31;
        edge.tm_hour = 23;
        edge.tm_min = 59;
        Require(format(text, 26, &edge) == 0);
        Require(std::strcmp(text, "Thu Dec 31 23:59:60 9999\n") == 0);
    }
    using CtimeFunction = int (APS5_VABI *)(char*, std::size_t, const std::int64_t*);
    for (CtimeFunction format : {ctime_s_nid_postfix, sceAmprCtimeS}) {
        const auto invalid = [&](const std::int64_t* time, std::size_t size, bool clear, const char* message) {
            std::memset(text, '!', sizeof(text));
            g_handlerCalls = 0;
            Require(format(text, size, time) == EINVAL);
            Require(g_handlerCalls == 1 && g_handlerError == EINVAL && g_handlerPointer == nullptr);
            Require(std::strcmp(g_handlerMessage, message) == 0);
            Require(text[0] == (clear ? '\0' : '!') && text[1] == '!');
        };
        invalid(nullptr, sizeof(text), true, "ctime_s: bad argument");
        invalid(&epoch, 1, true, "ctime_s: bad argument");
        invalid(&epoch, 25, true, "ctime_s: bad argument");
        invalid(&epoch, 0, false, "ctime_s: bad destination argument");
        invalid(nullptr, 0, false, "ctime_s: bad destination argument");
        invalid(&epoch, std::size_t{1} << 61, false, "ctime_s: bad destination argument");
        invalid(&epoch, std::numeric_limits<std::size_t>::max(), false, "ctime_s: bad destination argument");
        const std::int64_t extreme = std::numeric_limits<std::int64_t>::max();
        invalid(&extreme, sizeof(text), true, "ctime_s: bad argument");
        g_handlerCalls = 0;
        Require(format(nullptr, 26, &epoch) == EINVAL && g_handlerCalls == 1);
        Require(std::strcmp(g_handlerMessage, "ctime_s: bad destination argument") == 0);
        Require(format(text, (std::size_t{1} << 61) - 1, &epoch) == 0);
    }
    set_constraint_handler_s_nid_postfix(libcHandler);

    using TokenizeFunction = char* (APS5_VABI *)(char*, std::size_t*, const char*, char**);
    for (TokenizeFunction tokenize : {strtok_s_nid_postfix, sceAmprStrtokS}) {
        void* previousHandler = set_constraint_handler_s_nid_postfix(reinterpret_cast<void*>(&CountingHandler));
        char line[] = "a,b,,c";
        std::size_t remaining = sizeof(line);
        char* context = nullptr;
        g_handlerCalls = 0;
        Require(tokenize(line, &remaining, ",", &context) == line && remaining == 5 && context == line + 2);
        Require(std::strcmp(line, "a") == 0);
        Require(tokenize(nullptr, &remaining, ",", &context) == line + 2 && remaining == 3 && context == line + 4);
        Require(std::strcmp(line + 2, "b") == 0);
        Require(tokenize(nullptr, &remaining, ",", &context) == line + 5 && remaining == 1 && context == line + 6);
        Require(std::strcmp(line + 5, "c") == 0);
        Require(tokenize(nullptr, &remaining, ",", &context) == nullptr && remaining == 1 && context == line + 6);
        Require(g_handlerCalls == 0);
        const auto invalid = [&](char* source, std::size_t* count, const char* separators, char** state, const char* message) {
            g_handlerCalls = 0;
            Require(tokenize(source, count, separators, state) == nullptr);
            Require(g_handlerCalls == 1 && g_handlerError == EINVAL && g_handlerPointer == nullptr);
            Require(std::strcmp(g_handlerMessage, message) == 0);
        };
        char bounded[] = {'a', 'b', 'c', '!'};
        remaining = 3;
        context = line;
        invalid(bounded, &remaining, ",", &context, "strtok_s: missing delimiter");
        Require(remaining == 3 && context == line && std::memcmp(bounded, "abc!", 4) == 0);
        char skipped[] = {',', ',', '!'};
        remaining = 2;
        invalid(skipped, &remaining, ",", &context, "strtok_s: missing token");
        Require(remaining == 2 && context == line && std::memcmp(skipped, ",,!", 3) == 0);
        char prefixed[] = ",,abc";
        remaining = 4;
        invalid(prefixed, &remaining, ",", &context, "strtok_s: missing delimiter");
        Require(remaining == 2 && context == line && std::strcmp(prefixed, ",,abc") == 0);
        remaining = 0;
        invalid(line, &remaining, ",", &context, "strtok_s: missing token");
        invalid(line, &remaining, nullptr, &context, "strtok_s: bad argument");
        Require(remaining == 0 && context == line);
        remaining = std::size_t{1} << 61;
        invalid(line, &remaining, ",", &context, "strtok_s: bad argument");
        Require(remaining == (std::size_t{1} << 61) && context == line);
        invalid(line, nullptr, ",", &context, "strtok_s: bad argument");
        remaining = sizeof(line);
        invalid(line, &remaining, ",", nullptr, "strtok_s: bad argument");
        context = nullptr;
        invalid(nullptr, &remaining, ",", &context, "strtok_s: bad argument");
        Require(context == nullptr && remaining == sizeof(line));
        char empty[] = "";
        remaining = 1;
        g_handlerCalls = 0;
        Require(tokenize(empty, &remaining, ",", &context) == nullptr && remaining == 1 && context == empty);
        char unsplit[] = "whole string";
        remaining = (std::size_t{1} << 61) - 1;
        Require(tokenize(unsplit, &remaining, "", &context) == unsplit);
        Require(remaining == (std::size_t{1} << 61) - sizeof(unsplit) && context == unsplit + sizeof(unsplit) - 1);
        char changing[] = "a,b;c";
        remaining = sizeof(changing);
        Require(tokenize(changing, &remaining, ",", &context) == changing);
        Require(tokenize(nullptr, &remaining, ";", &context) == changing + 2);
        Require(std::strcmp(changing + 2, "b") == 0);
        Require(tokenize(nullptr, &remaining, ";", &context) == changing + 4);
        char highBytes[] = {'a', static_cast<char>(0xff), 'b', 0};
        const char highDelimiter[] = {static_cast<char>(0xff), 0};
        remaining = sizeof(highBytes);
        Require(tokenize(highBytes, &remaining, highDelimiter, &context) == highBytes && remaining == 2);
        Require(tokenize(nullptr, &remaining, highDelimiter, &context) == highBytes + 2 && remaining == 1);
        Require(g_handlerCalls == 0);
        set_constraint_handler_s_nid_postfix(previousHandler);
    }

    std::uint16_t wide[8] = {};
    Require(sceAmprMbstowcs(wide, "ABC", 8) == 3);
    Require(wide[0] == 65 && wide[1] == 66 && wide[2] == 67 && wide[3] == 0);
    Require(sceAmprMbstowcs(nullptr, "ABC", 8) == 3);
    Require(sceAmprMbstowcs(nullptr, "ABC", 0) == 3 && sceAmprMbstowcs(nullptr, "ABC", 1) == 3);
    Require(mbstowcs_nid_postfix(nullptr, "ABC", 0) == 3 && mbstowcs_nid_postfix(nullptr, "ABC", 1) == 3);
    Require(sceAmprMbstowcs(wide, "ABC", 2) == 2);
    Require(mbstowcs_nid_postfix(wide, "ABC", 8) == 3);

    const unsigned int firstRandom = _ZSt14_Random_devicev_nid_postfix();
    bool randomChanged = false;
    for (int draw = 0; draw < 8; ++draw) randomChanged |= _ZSt14_Random_devicev_nid_postfix() != firstRandom;
    Require(randomChanged);
}
