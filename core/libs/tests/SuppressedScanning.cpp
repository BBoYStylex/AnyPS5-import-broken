#include "SceTypes.hpp"
#include "prx/libc/include/FileStream.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

extern "C" {
int APS5_VABI sscanf_nid_postfix(const char*, const char*, ...);
int APS5_VABI vsscanf_nid_postfix(const char*, const char*, VaList*);
int APS5_VABI sscanf_s_nid_postfix(const char*, const char*, ...);
int APS5_VABI fscanf_nid_postfix(FileStream*, const char*, ...);
}

static int failures = 0;
static void Require(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "%s\n", message); ++failures; }
}

static int APS5_VABI ScanList(const char* input, const char* format, ...) {
    __builtin_sysv_va_list args;
    __builtin_sysv_va_start(args, format);
    VaList list;
    std::memcpy(&list, args, sizeof(list));
    const VaList before = list;
    const int result = vsscanf_nid_postfix(input, format, &list);
    Require(std::memcmp(&list, &before, sizeof(list)) == 0, "vsscanf modified caller va_list");
    __builtin_sysv_va_end(args);
    return result;
}

static void CheckStrings() {
    struct Case { const char* input; const char* format; int expected; };
    for (const Case test : std::array<Case, 14>{{
        {"12", "%*d%d", 0}, {"12 ", "%*d %d", 0}, {"abc", "%*s%d", 0},
        {"a", "%*c%d", 0}, {"abc", "%*[a-z]%d", 0}, {"1.5", "%*f%d", 0},
        {"12", "%*d%d%d", 0}, {"12", "%*d", 0}, {"", "%*d", EOF},
        {"  ", "%*d%d", EOF}, {"", "%d", EOF}, {"x", "%*d%d", 0},
        {"abc", "%*[a-z]%*d%d", 0}, {"a", "%*cX%d", 0}
    }}) {
        int number = -1, second = -1;
        Require(sscanf_nid_postfix(test.input, test.format, &number, &second) == test.expected, test.format);
        Require(ScanList(test.input, test.format, &number, &second) == test.expected, "vsscanf result");
        Require(sscanf_s_nid_postfix(test.input, test.format, &number, &second) == test.expected, "secure scan result");
        Require(number == -1 && second == -1, "failed conversion modified output");
    }
    int number = -1, position = -1;
    Require(ScanList("12", "%*d%n%d", &position, &number) == 0 && position == 2 && number == -1, "%n after suppression");
    Require(ScanList("", "%n%*d", &position) == EOF && position == 0, "%n alone is not input conversion");
    Require(ScanList("1 2", "%d%*d%d", &number, &position) == 1 && number == 1, "earlier assignment count");
    Require(ScanList("1 2 3", "%*d %d %*d", &number) == 1 && number == 2, "multiple suppressions");
    std::array<int, 8> values{};
    Require(ScanList("0 1 2 3 4 5 6 7 8", "%*d%d%d%d%d%d%d%d%d",
        &values[0], &values[1], &values[2], &values[3], &values[4], &values[5], &values[6], &values[7]) == 8, "register and stack destinations");
    for (size_t index = 0; index < values.size(); ++index)
        Require(values[index] == static_cast<int>(index + 1), "argument order");
    char rest[8] = {};
    Require(ScanList("0b101", "%*i%7s", rest) == 1 && std::strcmp(rest, "b101") == 0, "pre-C23 integer semantics");
#ifndef _WIN32
    int first = -1, second = -1;
    Require(ScanList("0 11 22", "%*d %2$d %1$d", &first, &second) == 2 && first == 22 && second == 11, "positional destinations");
    Require(ScanList("12", "%*d%2$d", &first, &second) == 0 && first == 22 && second == 11, "positional EOF");
    char set[8] = {};
    Require(ScanList("%*abc", "%7[%*a-z]", set) == 1 && std::strcmp(set, "%*abc") == 0, "scanset percent and star");
#endif
}

static void CheckStream(FileStream& stream, const char* format, int expected) {
    int count = -1, number = -1;
    Require(fscanf_nid_postfix(&stream, format, &count, &number) == expected, "file scan result");
    Require(number == -1, "file failed assignment");
    if (expected == 0) Require(count > 0, "file count side effect");
    stream.Close();
}

int main() {
    CheckStrings();
    for (const auto& fixture : std::array<std::array<const char*, 2>, 5>{{
        {"12", "%*d%n%d"}, {"abc", "%*s%n%d"}, {"a", "%*c%n%d"},
        {"abc", "%*[a-z]%n%d"}, {"1.5", "%*f%n%d"}
    }}) {
        FileStream stream(std::tmpfile());
        Require(std::fputs(fixture[0], stream.GetHandle()) >= 0, "file fixture write");
        std::rewind(stream.GetHandle());
        CheckStream(stream, fixture[1], 0);
    }
    FileStream empty(std::tmpfile());
    CheckStream(empty, "%n%*d%d", EOF);
    int descriptors[2];
#ifdef _WIN32
    Require(_pipe(descriptors, 4096, _O_BINARY) == 0, "pipe");
    Require(_write(descriptors[1], "12", 2) == 2, "pipe write");
    Require(_close(descriptors[1]) == 0, "pipe writer close");
    FileStream pipe(_fdopen(descriptors[0], "rb"));
#else
    Require(::pipe(descriptors) == 0, "pipe");
    Require(::write(descriptors[1], "12", 2) == 2, "pipe write");
    Require(::close(descriptors[1]) == 0, "pipe writer close");
    FileStream pipe(::fdopen(descriptors[0], "rb"));
#endif
    CheckStream(pipe, "%*d%n%d", 0);
    return failures == 0 ? 0 : 1;
}
