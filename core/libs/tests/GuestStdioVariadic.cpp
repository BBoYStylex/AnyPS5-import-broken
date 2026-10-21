#include "prx/libc/include/FileStream.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdarg>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {
extern FileStream* __stdinp_nid_postfix;
int APS5_VABI vfscanf_nid_postfix(FileStream*, const char*, void*);
int APS5_VABI vscanf_nid_postfix(const char*, void*);
int APS5_VABI scanf_nid_postfix(const char*, ...);
int APS5_VABI vfwprintf_nid_postfix(FileStream*, const char16_t*, void*);
int APS5_VABI fwprintf_nid_postfix(FileStream*, const char16_t*, ...);
int APS5_VABI vswscanf_nid_postfix(const char16_t*, const char16_t*, void*);
int APS5_VABI vswscanf_s_nid_postfix(const char16_t*, const char16_t*, void*);
}

#ifdef _WIN32
#define APS5_VA_BEGIN(last) __builtin_sysv_va_list args; __builtin_sysv_va_start(args, last)
#define APS5_VA_END() __builtin_sysv_va_end(args)
#else
#define APS5_VA_BEGIN(last) std::va_list args; va_start(args, last)
#define APS5_VA_END() va_end(args)
#endif

static void Require(bool value, const char* what) {
    if (value) return;
    std::fprintf(stderr, "failed: %s\n", what);
    std::abort();
}

static int APS5_VABI ScanStream(FileStream* stream, const char* format, ...) {
    APS5_VA_BEGIN(format);
    VaList original;
    std::memcpy(&original, args, sizeof(original));
    const int result = vfscanf_nid_postfix(stream, format, args);
    Require(std::memcmp(&original, args, sizeof(original)) == 0, "vfscanf preserves the caller argument cursor");
    APS5_VA_END();
    return result;
}

static int APS5_VABI ScanInput(const char* format, ...) {
    APS5_VA_BEGIN(format);
    const int result = vscanf_nid_postfix(format, args);
    APS5_VA_END();
    return result;
}

static int APS5_VABI PrintWide(FileStream* stream, const char16_t* format, ...) {
    APS5_VA_BEGIN(format);
    const int result = vfwprintf_nid_postfix(stream, format, args);
    APS5_VA_END();
    return result;
}

static int APS5_VABI ScanWide(const char16_t* input, const char16_t* format, ...) {
    APS5_VA_BEGIN(format);
    const int result = vswscanf_nid_postfix(input, format, args);
    APS5_VA_END();
    return result;
}

static int APS5_VABI ScanWideSized(const char16_t* input, const char16_t* format, ...) {
    APS5_VA_BEGIN(format);
    const int result = vswscanf_s_nid_postfix(input, format, args);
    APS5_VA_END();
    return result;
}

static std::string Contents(FileStream& stream) {
    std::fflush(stream.GetHandle());
    std::rewind(stream.GetHandle());
    std::string text;
    for (int c; (c = std::fgetc(stream.GetHandle())) != EOF;) text.push_back(static_cast<char>(c));
    return text;
}

int main() {
    FileStream scanned(std::tmpfile());
    Require(std::fputs("42 answer 7", scanned.GetHandle()) >= 0, "seed the scanned stream");
    std::rewind(scanned.GetHandle());
    int number = 0;
    char word[16]{};
    Require(ScanStream(&scanned, "%d %15s", &number, word) == 2, "vfscanf assigns both conversions");
    Require(number == 42 && std::strcmp(word, "answer") == 0, "vfscanf values");
    Require(ScanStream(&scanned, "%d", &number) == 1 && number == 7, "vfscanf continues from the stream position");
    Require(ScanStream(&scanned, "%d", &number) == EOF && number == 7, "vfscanf at end of file");
    scanned.Close();

    FileStream input(std::tmpfile());
    Require(std::fputs("-5 12", input.GetHandle()) >= 0, "seed the standard input stream");
    std::rewind(input.GetHandle());
    FileStream* const standardInput = __stdinp_nid_postfix;
    __stdinp_nid_postfix = &input;
    int first = 0, second = 0;
    Require(ScanInput("%d", &first) == 1 && first == -5, "vscanf reads the guest standard input");
    Require(scanf_nid_postfix("%d", &second) == 1 && second == 12, "scanf reads the guest standard input");
    __stdinp_nid_postfix = standardInput;
    input.Close();

    FileStream wideInput(std::tmpfile());
    Require(std::fputs("skip alpha 1 2 3 4 5 6 7!", wideInput.GetHandle()) >= 0, "seed wide variadic input");
    std::rewind(wideInput.GetHandle());
    char16_t wideWord[8]{};
    int values[7]{};
    int count = -1;
    std::int64_t longCount = -1;
    Require(ScanStream(&wideInput, "%*ls %5ls %d %d %d %d %d %d %d%ln%n", wideWord,
        &values[0], &values[1], &values[2], &values[3], &values[4], &values[5], &values[6], &longCount, &count) == 8,
        "vfscanf wide conversion and overflow pointer arguments");
    Require(std::u16string(wideWord) == u"alpha", "vfscanf writes guest 16-bit characters");
    for (int index = 0; index < 7; ++index) Require(values[index] == index + 1, "vfscanf mixed pointer values");
    Require(count == 24 && longCount == 24 && std::ftell(wideInput.GetHandle()) == 24,
        "vfscanf counts consumed input");
    Require(std::fgetc(wideInput.GetHandle()) == '!', "vfscanf preserves the next unread byte");
    Require(ScanStream(&wideInput, "%ls", wideWord) == EOF && std::feof(wideInput.GetHandle()), "vfscanf wide EOF");
    Require((wideInput.GuestState().flags & 0x20) != 0, "vfscanf synchronizes guest EOF");
    wideInput.Close();
    errno = 0;
    Require(ScanStream(&wideInput, "%ls", wideWord) == -1 && errno == 9, "vfscanf rejects a closed wide stream");

    FileStream mismatch(std::tmpfile());
    Require(std::fputs("1", mismatch.GetHandle()) >= 0, "seed wide matching failure");
    std::rewind(mismatch.GetHandle());
    wideWord[0] = u'!';
    Require(ScanStream(&mismatch, "%l[a-z]", wideWord) == 0 && wideWord[0] == u'!', "vfscanf wide matching failure preserves destination");
    Require(std::fgetc(mismatch.GetHandle()) == '1', "vfscanf wide matching failure preserves input");
    mismatch.Close();

    FileStream wideStandard(std::tmpfile());
    Require(std::fputs("abc XYdef QZ", wideStandard.GetHandle()) >= 0, "seed wide standard input");
    std::rewind(wideStandard.GetHandle());
    __stdinp_nid_postfix = &wideStandard;
    char16_t pair[3] = {u'!', u'!', u'!'};
    Require(ScanInput("%l[a-c] %2lc", wideWord, pair) == 2, "vscanf wide scanset and width");
    Require(std::u16string(wideWord) == u"abc" && pair[0] == u'X' && pair[1] == u'Y' && pair[2] == u'!',
        "vscanf writes two-byte elements without terminating lc");
    Require(scanf_nid_postfix("%S %C", wideWord, pair) == 2 && std::u16string(wideWord) == u"def" && pair[0] == u'Q',
        "scanf guest standard input wide aliases");
    Require(std::fgetc(wideStandard.GetHandle()) == 'Z', "scanf preserves next unread input");
    __stdinp_nid_postfix = standardInput;
    wideStandard.Close();

    FileStream written(std::tmpfile());
    Require(fwprintf_nid_postfix(&written, u"%d:%ls|", 7, u"été") == 6, "fwprintf counts wide characters");
    Require(PrintWide(&written, u"%5.1f", 2.25) == 5, "vfwprintf counts wide characters");
    Require(Contents(written) == "7:\xc3\xa9t\xc3\xa9|  2.2", "the stream holds the formatted text");
    written.Close();

    std::int32_t a = 0, b = 0;
    Require(ScanWide(u"  31 -4", u"%d%d", &a, &b) == 2 && a == 31 && b == -4, "vswscanf");
    Require(ScanWide(u"", u"%d", &a) == EOF, "vswscanf on empty input");
    char16_t letters[4]{};
    Require(ScanWideSized(u"abcdef", u"%3ls", letters, 4u) == 1 && std::u16string(letters) == u"abc", "vswscanf_s %ls with a size");
    Require(ScanWideSized(u"abcdef", u"%ls", letters, 4u) == 0 && letters[0] == 0, "vswscanf_s rejects a too small buffer");
    return 0;
}
