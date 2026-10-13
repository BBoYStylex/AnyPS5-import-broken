#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libc/include/GuestTime.hpp"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

extern "C" {
std::size_t APS5_VABI strftime_nid_postfix(char*, std::size_t, const char*, const GuestTm*);
char* APS5_VABI asctime_nid_postfix(const GuestTm*);
}

namespace {

GuestTm MakeTime() {
    GuestTm time{};
    time.tm_sec = 5;
    time.tm_min = 4;
    time.tm_hour = 3;
    time.tm_mday = 2;
    time.tm_mon = 0;
    time.tm_year = 125;
    time.tm_wday = 4;
    time.tm_yday = 1;
    time.tm_isdst = 0;
    time.tm_gmtoff = 3600;
    time.tm_zone = "ABC";
    return time;
}

GuestTm MakeDate(int year, int month, int day, int wday, int yday) {
    GuestTm time = MakeTime();
    time.tm_year = year - 1900;
    time.tm_mon = month - 1;
    time.tm_mday = day;
    time.tm_wday = wday;
    time.tm_yday = yday;
    return time;
}

void UseConsoleZone() {
#ifdef _WIN32
    _putenv_s("TZ", "BRT3");
    _tzset();
#else
    setenv("TZ", "BRT3", 1);
    tzset();
#endif
}

}

int main() {
    UseConsoleZone();
    struct Case {
        const char* name;
        GuestTm time;
        const char* format;
        const char* expected;
    };
    const GuestTm base = MakeTime();
    const auto with = [&](auto change) {
        GuestTm time = base;
        change(time);
        return time;
    };
    const Case cases[] = {
        {"names", base, "%a|%A|%b|%B|%h", "Thu|Thursday|Jan|January|Jan"},
        {"date and time", base, "%c", "Thu Jan  2 03:04:05 2025"},
        {"years", base, "%C|%y|%Y|%g|%G", "20|25|2025|25|2025"},
        {"days", base, "%d|%e|%j|%m", "02| 2|002|01"},
        {"composites", base, "%D|%F|%T|%R|%r|%x|%X", "01/02/25|2025-01-02|03:04:05|03:04|03:04:05 AM|01/02/25|03:04:05"},
        {"clock", base, "%H|%I|%M|%S|%p", "03|03|04|05|AM"},
        {"weeks", base, "%u|%w|%U|%W|%V", "4|4|00|00|01"},
        {"zone", base, "[%z][%Z]", "[-0300][]"},
        {"daylight saving", with([](GuestTm& time) { time.tm_isdst = 1; }), "%z", "-0200"},
        {"daylight saving before 1986", with([](GuestTm& time) { time.tm_year = 85; time.tm_isdst = 1; }), "%z", "-0300"},
        {"modifiers", base, "%Ec|%EC|%Ex|%EX|%Ey|%EY|%Od|%OH|%Oa", "Thu Jan  2 03:04:05 2025|20|01/02/25|03:04:05|25|2025|02|03|Thu"},
        {"unknown conversions", base, "%k|%l|%+|%P|%v|%s|%-d|%_d|%5Y|%#a|%^a", "%k|%l|%+|%P|%v|%s|%-d|%_d|%5Y|%#a|%^a"},
        {"unknown after a modifier", base, "[%E]][%Ok][%EE]", "[%]][%k][%E]"},
        {"literals", base, "%n%t%%|x%", "\n\t%|x%"},
        {"hour -1", with([](GuestTm& time) { time.tm_hour = -1; }), "%H|%I|%p", "00|11|AM"},
        {"hour 100", with([](GuestTm& time) { time.tm_hour = 100; }), "%H|%I|%p", "00|04|PM"},
        {"day 100", with([](GuestTm& time) { time.tm_mday = 100; }), "%d|%e", "00| 0"},
        {"month -1", with([](GuestTm& time) { time.tm_mon = -1; }), "%b|%B|%m", "Jan|Jan|00"},
        {"month 12", with([](GuestTm& time) { time.tm_mon = 12; }), "[%b][%B][%m][%c]", "[][][13][Thu   2 03:04:05 2025]"},
        {"weekday -1", with([](GuestTm& time) { time.tm_wday = -1; }), "%a|%A|%u|%w|%U|%W", "Sun|Sun|0|0|00|00"},
        {"weekday 7", with([](GuestTm& time) { time.tm_wday = 7; }), "[%a][%A]%u|%w|%U|%W", "[][]7|7|01|00"},
        {"year 1899", with([](GuestTm& time) { time.tm_year = -1; }), "%C|%y|%Y", "19|99|1899"},
        {"year 1", with([](GuestTm& time) { time.tm_year = -1899; }), "%C|%y|%Y|%G", "01|01|0001|0001"},
        {"year 10000", with([](GuestTm& time) { time.tm_year = 8100; }), "%C|%y|%Y", "00|00|0000"},
        {"largest year", with([](GuestTm& time) { time.tm_year = 2147483647; }), "%C|%y|%Y", "55|47|5547"},
        {"day of year 1000", with([](GuestTm& time) { time.tm_yday = 1000; }), "%j|%U|%W|%V", "001|43|43|43"},
        {"seconds", with([](GuestTm& time) { time.tm_sec = 61; time.tm_min = -1; }), "%M:%S", "00:61"},
        {"ISO 2020-12-31", MakeDate(2020, 12, 31, 4, 365), "%G|%g|%V|%U|%W", "2020|20|53|52|52"},
        {"ISO 2021-01-03", MakeDate(2021, 1, 3, 0, 2), "%G|%g|%V|%U|%W", "2020|20|53|01|00"},
        {"ISO 2024-12-30", MakeDate(2024, 12, 30, 1, 364), "%G|%g|%V|%U|%W", "2025|25|01|52|53"},
        {"ISO 2027-01-03", MakeDate(2027, 1, 3, 0, 2), "%G|%g|%V", "2026|26|53"},
    };
    bool correct = true;
    for (const auto& test : cases) {
        char text[128];
        const std::size_t length = strftime_nid_postfix(text, sizeof(text), test.format, &test.time);
        if (length != std::strlen(test.expected) || std::strcmp(text, test.expected) != 0) {
            std::fprintf(stderr, "strftime %s (\"%s\"): expected \"%s\", received \"%s\" (%zu)\n",
                test.name, test.format, test.expected, length == 0 ? "" : text, length);
            correct = false;
        }
    }
    struct Capacity {
        const char* format;
        std::size_t size;
        std::size_t result;
        const char* written;
    };
    constexpr Capacity capacities[] = {
        {"%Y-%m-%d", 3, 0, ""},
        {"%Y-%m-%d", 4, 0, "2025"},
        {"%Y-%m-%d", 6, 0, "2025-"},
        {"%Y-%m-%d", 10, 0, "2025-01-02"},
        {"%Y-%m-%d", 11, 10, "2025-01-02"},
        {"abcdef", 3, 0, "abc"},
        {"%c", 9, 0, "Thu Jan "},
        {"%c", 10, 0, "Thu Jan  2"},
        {"%k", 1, 0, ""},
        {"%k", 2, 0, "%k"},
        {"%r", 11, 0, "03:04:05 AM"},
    };
    for (const auto& test : capacities) {
        char text[32];
        std::memset(text, '~', sizeof(text));
        const std::size_t length = strftime_nid_postfix(text, test.size, test.format, &base);
        const std::size_t written = std::strlen(test.written);
        if (length != test.result || std::memcmp(text, test.written, written) != 0 || text[written] != (length == 0 ? '~' : '\0')) {
            std::fprintf(stderr, "strftime \"%s\" into %zu bytes: expected %zu and \"%s\"\n", test.format, test.size, test.result, test.written);
            correct = false;
        }
    }
    char text[32];
    if (strftime_nid_postfix(text, sizeof(text), nullptr, &base) == 0 || std::strcmp(text, "Thu Jan  2 03:04:05 2025") != 0) {
        std::fprintf(stderr, "strftime with a null format does not format %%c\n");
        correct = false;
    }
    struct AsctimeCase {
        GuestTm time;
        const char* expected;
    };
    const AsctimeCase asctimeCases[] = {
        {base, "Thu Jan  2 03:04:05 2025\n"},
        {with([](GuestTm& time) { time.tm_year = -1; }), "Thu Jan  2 03:04:05 1899\n"},
        {with([](GuestTm& time) { time.tm_year = 8100; }), "Thu Jan  2 03:04:05 0000\n"},
        {with([](GuestTm& time) { time.tm_year = -2147483647 - 1; }), "Thu Jan  2 03:04:05 0000\n"},
        {with([](GuestTm& time) { time.tm_wday = 7; }), " Jan  2 03:04:05 2025\n"},
        {with([](GuestTm& time) { time.tm_mon = 12; }), "Thu   2 03:04:05 2025\n"},
        {with([](GuestTm& time) { time.tm_hour = 99; time.tm_mday = 0; }), "Thu Jan  0 99:04:05 2025\n"},
    };
    for (const auto& test : asctimeCases) {
        const char* result = asctime_nid_postfix(&test.time);
        if (std::strcmp(result, test.expected) != 0) {
            std::fprintf(stderr, "asctime: expected \"%s\", received \"%s\"\n", test.expected, result);
            correct = false;
        }
    }
    return correct ? 0 : 1;
}
