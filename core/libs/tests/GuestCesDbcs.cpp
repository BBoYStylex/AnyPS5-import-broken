#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" {
void* APS5_VABI sceCesUcsProfileInitEucKr(void*);
void* APS5_VABI sceCesUcsProfileInitUhc(void*);
void* APS5_VABI sceCesUcsProfileInitBig5Cp950(void*);
void* APS5_VABI sceCesUcsProfileInitEucJpCp51932(void*);
int APS5_VABI sceCesEucKrToUtf8(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUhcToUtf8(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesBig5ToUtf8(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesMbcsUcsContextInit(void*, const void*);
int APS5_VABI sceCesUtf8StrToMbcsStr(void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesMbcsStrToUtf8Str(void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUcs2StrToMbcsStr(void*, const std::uint16_t*, std::uint32_t, std::uint32_t*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesMbcsStrToUcs2Str(void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesEucKrToUcs2(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*);
int APS5_VABI sceCesEucKrToUtf16(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesEucKrToUtf16be(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesEucKrToUtf16le(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesEucKrToUtf32(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesEucKrToUtf32be(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesEucKrToUtf32le(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesUcs2ToEucKr(std::uint16_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf8ToEucKr(const std::uint8_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf16ToEucKr(const std::uint16_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf16beToEucKr(const std::uint16_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf16leToEucKr(const std::uint16_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf32ToEucKr(std::uint32_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf32beToEucKr(const std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf32leToEucKr(const std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUhcToUcs2(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*);
int APS5_VABI sceCesUhcToUtf16(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUhcToUtf16be(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUhcToUtf16le(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUhcToUtf32(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesUhcToUtf32be(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesUhcToUtf32le(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesUcs2ToUhc(std::uint16_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf8ToUhc(const std::uint8_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf16ToUhc(const std::uint16_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf16beToUhc(const std::uint16_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf16leToUhc(const std::uint16_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf32ToUhc(std::uint32_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf32beToUhc(const std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf32leToUhc(const std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesBig5ToUcs2(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*);
int APS5_VABI sceCesBig5ToUtf16(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesBig5ToUtf16be(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesBig5ToUtf16le(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesBig5ToUtf32(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesBig5ToUtf32be(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesBig5ToUtf32le(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesUcs2ToBig5(std::uint16_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf8ToBig5(const std::uint8_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf16ToBig5(const std::uint16_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf16beToBig5(const std::uint16_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf16leToBig5(const std::uint16_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf32ToBig5(std::uint32_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf32beToBig5(const std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf32leToBig5(const std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
}

using ToUtf8 = int (APS5_VABI*)(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint8_t*, std::uint32_t, std::uint32_t*);

constexpr int invalidParameter = static_cast<int>(0x805C0001);
constexpr int invalidSrcBuffer = static_cast<int>(0x805C0010);
constexpr int srcBufferEnd = static_cast<int>(0x805C0011);
constexpr int invalidEncode = static_cast<int>(0x805C0014);
constexpr int unassignedCode = static_cast<int>(0x805C0020);
constexpr int invalidDstBuffer = static_cast<int>(0x805C0030);
constexpr int dstBufferEnd = static_cast<int>(0x805C0031);

static void Require(bool value) { if (!value) std::abort(); }

static const std::uint8_t* Bytes(const char* text) { return reinterpret_cast<const std::uint8_t*>(text); }

static std::uint32_t Size(const char* text) { return static_cast<std::uint32_t>(std::strlen(text)); }

static std::uint64_t Fnv(std::uint64_t hash, std::uint8_t byte) { return (hash ^ byte) * 0x100000001B3ull; }

static void RequireTable(ToUtf8 convert, const void* profile, std::uint64_t expectedHash, std::uint32_t expectedCells) {
    std::uint64_t hash = 0xCBF29CE484222325ull;
    std::uint32_t cells = 0;
    for (std::uint32_t lead = 0; lead < 256; ++lead) {
        for (std::uint32_t trail = 0; trail < 256; ++trail) {
            const std::uint8_t source[2] = {static_cast<std::uint8_t>(lead), static_cast<std::uint8_t>(trail)};
            std::uint8_t utf8[4] = {};
            std::uint32_t consumed = 0;
            std::uint32_t produced = 0;
            const int result = convert(profile, source, sizeof(source), &consumed, utf8, sizeof(utf8), &produced);
            hash = Fnv(hash, static_cast<std::uint8_t>(result & 0xFF));
            if (result != 0) continue;
            hash = Fnv(hash, static_cast<std::uint8_t>(consumed));
            for (std::uint32_t i = 0; i < produced; ++i) hash = Fnv(hash, utf8[i]);
            if (consumed == 2) ++cells;
        }
    }
    Require(hash == expectedHash && cells == expectedCells);
}

static void RequireScalar(ToUtf8 convert, const void* profile, const char* source, const char* expected) {
    std::uint8_t utf8[4] = {};
    std::uint32_t consumed = 0;
    std::uint32_t produced = 0;
    Require(convert(profile, Bytes(source), Size(source), &consumed, utf8, sizeof(utf8), &produced) == 0);
    Require(consumed == Size(source) && produced == Size(expected) && std::memcmp(utf8, expected, produced) == 0);
}

static int Scalar(ToUtf8 convert, const void* profile, const char* source, std::uint32_t sourceMax, std::uint32_t utf8Max) {
    std::uint8_t utf8[4] = {0x55, 0x55, 0x55, 0x55};
    std::uint32_t consumed = 77;
    std::uint32_t produced = 77;
    const int result = convert(profile, Bytes(source), sourceMax, &consumed, utf8, utf8Max, &produced);
    if (result != 0) Require(consumed == 77 && produced == 77 && std::memcmp(utf8, "\x55\x55\x55\x55", 4) == 0);
    return result;
}

static void RequireString(void* context, const char* mbcs, const char* utf8) {
    std::uint8_t converted[32] = {};
    std::uint32_t read = 0;
    std::uint32_t written = 0;
    Require(sceCesMbcsStrToUtf8Str(context, Bytes(mbcs), Size(mbcs), &read, converted, sizeof(converted), &written) == 0);
    Require(read == Size(mbcs) && written == Size(utf8) && std::memcmp(converted, utf8, written) == 0);
    std::uint8_t back[32] = {};
    Require(sceCesUtf8StrToMbcsStr(context, Bytes(utf8), Size(utf8), &read, back, sizeof(back), &written) == 0);
    Require(read == Size(utf8) && written == Size(mbcs) && std::memcmp(back, mbcs, written) == 0);
}

static int Utf8ToMbcs(void* context, const char* utf8) {
    std::uint8_t mbcs[8] = {};
    std::uint32_t read = 0;
    std::uint32_t written = 0;
    return sceCesUtf8StrToMbcsStr(context, Bytes(utf8), Size(utf8), &read, mbcs, sizeof(mbcs), &written);
}

static std::uint32_t RoundTripMismatches(void* context) {
    std::uint32_t mismatches = 0;
    for (std::uint32_t lead = 0x81; lead < 0xFF; ++lead) {
        for (std::uint32_t trail = 0x40; trail < 0xFF; ++trail) {
            const std::uint8_t source[2] = {static_cast<std::uint8_t>(lead), static_cast<std::uint8_t>(trail)};
            std::uint16_t ucs2 = 0;
            std::uint32_t read = 0;
            std::uint32_t written = 0;
            if (sceCesMbcsStrToUcs2Str(context, source, sizeof(source), &read, &ucs2, 1, &written) != 0) continue;
            Require(read == 2 && written == 1);
            std::uint8_t back[2] = {};
            Require(sceCesUcs2StrToMbcsStr(context, &ucs2, 1, &read, back, sizeof(back), &written) == 0);
            Require(read == 1 && written == 2);
            if (std::memcmp(source, back, sizeof(back)) != 0) ++mismatches;
        }
    }
    return mismatches;
}

struct WideForms {
    ToUtf8 toUtf8;
    int (APS5_VABI* toUcs2)(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*);
    int (APS5_VABI* toUtf16[3])(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
    int (APS5_VABI* toUtf32[3])(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
    int (APS5_VABI* fromUcs2)(std::uint16_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
    int (APS5_VABI* fromUtf8)(const std::uint8_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
    int (APS5_VABI* fromUtf16[3])(const std::uint16_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
    int (APS5_VABI* fromUtf32)(std::uint32_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
    int (APS5_VABI* fromUtf32Array[2])(const std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
};

static std::uint16_t Swap(std::uint16_t value) { return static_cast<std::uint16_t>(value << 8 | value >> 8); }

static std::uint32_t Swap(std::uint32_t value) { return value << 24 | (value & 0xFF00) << 8 | (value >> 8 & 0xFF00) | value >> 24; }

static void RequireWideForms(const WideForms& forms, const void* profile, const void* other) {
    std::uint32_t assigned = 0;
    for (std::uint32_t value = 0; value < 0x10000; ++value) {
        const std::uint8_t source[2] = {static_cast<std::uint8_t>(value >> 8), static_cast<std::uint8_t>(value)};
        const std::uint32_t sourceMax = value < 0x100 ? 1 : 2;
        const std::uint8_t* bytes = value < 0x100 ? source + 1 : source;
        std::uint16_t ucs2 = 0x5555;
        std::uint32_t read = 77;
        const int result = forms.toUcs2(profile, bytes, sourceMax, &read, &ucs2);
        std::uint8_t utf8[4] = {};
        std::uint32_t consumed = 0;
        std::uint32_t produced = 0;
        Require((forms.toUtf8(profile, bytes, sourceMax, &consumed, utf8, sizeof(utf8), &produced) == 0) == (result == 0));
        std::uint16_t utf16[3] = {};
        std::uint32_t utf32[3] = {};
        std::uint32_t length = 0;
        for (int form = 0; form < 3; ++form) {
            std::uint32_t formRead = 78;
            std::uint32_t utf32Read = 79;
            utf16[form] = 0x5555;
            utf32[form] = 0x55555555;
            Require((forms.toUtf16[form](profile, bytes, sourceMax, &formRead, &utf16[form], 1, &length) == 0) == (result == 0));
            Require(forms.toUtf32[form](profile, bytes, sourceMax, &utf32Read, &utf32[form]) == result);
            if (result != 0) Require(formRead == 78 && utf32Read == 79 && utf16[form] == 0x5555 && utf32[form] == 0x55555555);
            else Require(formRead == consumed && utf32Read == consumed && length == 1);
        }
        if (result != 0) {
            Require(read == 77 && ucs2 == 0x5555);
            continue;
        }
        ++assigned;
        Require(read == consumed);
        Require(utf16[0] == ucs2 && utf16[1] == Swap(ucs2) && utf16[2] == ucs2);
        Require(utf32[0] == ucs2 && utf32[1] == Swap(static_cast<std::uint32_t>(ucs2)) && utf32[2] == ucs2);
        const std::uint32_t decoded = produced == 1 ? utf8[0] : produced == 2 ? (utf8[0] & 0x1Fu) << 6 | (utf8[1] & 0x3Fu) : (utf8[0] & 0x0Fu) << 12 | (utf8[1] & 0x3Fu) << 6 | (utf8[2] & 0x3Fu);
        Require(decoded == ucs2);
    }
    Require(assigned > 8000);

    for (std::uint32_t code = 0; code < 0x10000; ++code) {
        const bool surrogate = code >= 0xD800 && code < 0xE000;
        std::uint8_t expected[3] = {0x55, 0x55, 0x55};
        std::uint32_t expectedSize = 66;
        const int result = forms.fromUcs2(static_cast<std::uint16_t>(code), profile, expected, sizeof(expected), &expectedSize);
        Require(surrogate ? result == invalidEncode : result == 0 || result == unassignedCode);
        std::uint8_t utf8[3] = {};
        std::uint32_t utf8Size = 0;
        if (code < 0x80) { utf8[0] = static_cast<std::uint8_t>(code); utf8Size = 1; }
        else if (code < 0x800) { utf8[0] = static_cast<std::uint8_t>(0xC0 | code >> 6); utf8[1] = static_cast<std::uint8_t>(0x80 | (code & 0x3F)); utf8Size = 2; }
        else { utf8[0] = static_cast<std::uint8_t>(0xE0 | code >> 12); utf8[1] = static_cast<std::uint8_t>(0x80 | (code >> 6 & 0x3F)); utf8[2] = static_cast<std::uint8_t>(0x80 | (code & 0x3F)); utf8Size = 3; }
        const std::uint16_t units[3] = {static_cast<std::uint16_t>(code), Swap(static_cast<std::uint16_t>(code)), static_cast<std::uint16_t>(code)};
        const std::uint32_t wide[3] = {code, Swap(code), code};
        std::uint8_t actual[7][3];
        std::uint32_t sizes[7];
        std::uint32_t lengths[7];
        int results[7];
        for (int i = 0; i < 7; ++i) { std::memset(actual[i], 0x55, 3); sizes[i] = 66; lengths[i] = 99; }
        results[0] = forms.fromUtf8(utf8, utf8Size, &lengths[0], profile, actual[0], 3, &sizes[0]);
        for (int form = 0; form < 3; ++form) results[1 + form] = forms.fromUtf16[form](&units[form], 1, &lengths[1 + form], profile, actual[1 + form], 3, &sizes[1 + form]);
        results[4] = forms.fromUtf32(code, profile, actual[4], 3, &sizes[4]);
        results[5] = forms.fromUtf32Array[0](&wide[1], profile, actual[5], 3, &sizes[5]);
        results[6] = forms.fromUtf32Array[1](&wide[2], profile, actual[6], 3, &sizes[6]);
        for (int i = 0; i < 7; ++i) {
            if (surrogate && i >= 0 && i <= 3) continue;
            Require(results[i] == result);
            if (result != 0) Require(sizes[i] == 66 && std::memcmp(actual[i], "\x55\x55\x55", 3) == 0);
            else Require(sizes[i] == expectedSize && std::memcmp(actual[i], expected, expectedSize) == 0);
        }
        if (result == 0) Require(lengths[0] == utf8Size && lengths[1] == 1 && lengths[2] == 1 && lengths[3] == 1);
        if (code == 0x41) Require(forms.fromUcs2(0x41, other, expected, sizeof(expected), &expectedSize) == invalidParameter);
    }
    std::uint8_t out[3];
    std::uint32_t size = 0;
    Require(forms.fromUtf32(0x110000, profile, out, sizeof(out), &size) == invalidEncode);
    Require(forms.fromUcs2(0x41, profile, nullptr, sizeof(out), &size) == invalidDstBuffer);
    Require(forms.fromUtf8(Bytes("A"), 1, nullptr, profile, out, 0, &size) == dstBufferEnd);
    Require(forms.fromUtf16[0](nullptr, 1, nullptr, profile, out, sizeof(out), &size) == invalidSrcBuffer);
}

int main() {
    alignas(8) std::uint8_t eucKrSheet[64] = {};
    alignas(8) std::uint8_t uhcSheet[64] = {};
    alignas(8) std::uint8_t big5Sheet[64] = {};
    alignas(8) std::uint8_t eucJpSheet[64] = {};
    Require(sceCesUcsProfileInitEucKr(nullptr) == nullptr);
    Require(sceCesUcsProfileInitUhc(nullptr) == nullptr);
    Require(sceCesUcsProfileInitBig5Cp950(nullptr) == nullptr);
    void* eucKr = sceCesUcsProfileInitEucKr(eucKrSheet);
    void* uhc = sceCesUcsProfileInitUhc(uhcSheet);
    void* big5 = sceCesUcsProfileInitBig5Cp950(big5Sheet);
    void* eucJp = sceCesUcsProfileInitEucJpCp51932(eucJpSheet);
    Require(eucKr == eucKrSheet && uhc == uhcSheet && big5 == big5Sheet && eucJp == eucJpSheet);

    RequireTable(sceCesEucKrToUtf8, eucKr, 0x01FF720E36F4AD67ull, 8226);
    RequireTable(sceCesUhcToUtf8, uhc, 0x3B861A7E911985ABull, 17048);
    RequireTable(sceCesBig5ToUtf8, big5, 0xE4D0D6A8019F35D9ull, 13503);

    RequireScalar(sceCesEucKrToUtf8, eucKr, "\xB0\xA1", "\xEA\xB0\x80");
    RequireScalar(sceCesEucKrToUtf8, eucKr, "\xA2\xE6", "\xE2\x82\xAC");
    RequireScalar(sceCesUhcToUtf8, uhc, "\xB0\xA1", "\xEA\xB0\x80");
    RequireScalar(sceCesUhcToUtf8, uhc, "\x8C\x63", "\xEB\x98\xA0");
    RequireScalar(sceCesBig5ToUtf8, big5, "\xA4\xA4", "\xE4\xB8\xAD");
    RequireScalar(sceCesBig5ToUtf8, big5, "A", "A");
    std::uint8_t zero[4] = {0x55, 0x55, 0x55, 0x55};
    Require(sceCesUhcToUtf8(uhc, Bytes("\0"), 1, nullptr, zero, sizeof(zero), nullptr) == 0 && zero[0] == 0 && zero[1] == 0x55);

    Require(Scalar(sceCesEucKrToUtf8, nullptr, "\xB0\xA1", 2, 4) == invalidParameter);
    Require(Scalar(sceCesEucKrToUtf8, uhc, "\xB0\xA1", 2, 4) == invalidParameter);
    Require(Scalar(sceCesUhcToUtf8, eucKr, "\xB0\xA1", 2, 4) == invalidParameter);
    Require(Scalar(sceCesBig5ToUtf8, eucJp, "\xA4\xA4", 2, 4) == invalidParameter);
    Require(Scalar(sceCesEucKrToUtf8, eucKr, nullptr, 2, 4) == invalidSrcBuffer);
    Require(Scalar(sceCesEucKrToUtf8, eucKr, "\xB0\xA1", 0, 4) == srcBufferEnd);
    Require(Scalar(sceCesUhcToUtf8, uhc, "\x8C\x63", 1, 4) == srcBufferEnd);
    Require(Scalar(sceCesBig5ToUtf8, big5, "\xA4\xA4", 1, 4) == srcBufferEnd);
    Require(Scalar(sceCesEucKrToUtf8, eucKr, "\xB0\xA1", 2, 2) == dstBufferEnd);
    Require(Scalar(sceCesEucKrToUtf8, eucKr, "\x8C\x63", 2, 4) == invalidEncode);
    Require(Scalar(sceCesUhcToUtf8, uhc, "\x80\xA1", 2, 4) == invalidEncode);
    Require(Scalar(sceCesUhcToUtf8, uhc, "\xB0\x5B", 2, 4) == invalidEncode);
    Require(Scalar(sceCesUhcToUtf8, uhc, "\xC9\xA1", 2, 4) == unassignedCode);
    Require(Scalar(sceCesUhcToUtf8, uhc, "\xC7\x41", 2, 4) == unassignedCode);
    Require(Scalar(sceCesBig5ToUtf8, big5, "\xA4\x80", 2, 4) == invalidEncode);
    Require(Scalar(sceCesBig5ToUtf8, big5, "\xFF\xA1", 2, 4) == invalidEncode);
    Require(Scalar(sceCesBig5ToUtf8, big5, "\x81\x40", 2, 4) == unassignedCode);
    Require(Scalar(sceCesBig5ToUtf8, big5, "\xFA\x40", 2, 4) == unassignedCode);
    Require(sceCesEucKrToUtf8(eucKr, Bytes("\xB0\xA1"), 2, nullptr, nullptr, 4, nullptr) == invalidDstBuffer);

    const WideForms eucKrForms = {sceCesEucKrToUtf8, sceCesEucKrToUcs2, {sceCesEucKrToUtf16, sceCesEucKrToUtf16be, sceCesEucKrToUtf16le}, {sceCesEucKrToUtf32, sceCesEucKrToUtf32be, sceCesEucKrToUtf32le}, sceCesUcs2ToEucKr, sceCesUtf8ToEucKr, {sceCesUtf16ToEucKr, sceCesUtf16beToEucKr, sceCesUtf16leToEucKr}, sceCesUtf32ToEucKr, {sceCesUtf32beToEucKr, sceCesUtf32leToEucKr}};
    const WideForms uhcForms = {sceCesUhcToUtf8, sceCesUhcToUcs2, {sceCesUhcToUtf16, sceCesUhcToUtf16be, sceCesUhcToUtf16le}, {sceCesUhcToUtf32, sceCesUhcToUtf32be, sceCesUhcToUtf32le}, sceCesUcs2ToUhc, sceCesUtf8ToUhc, {sceCesUtf16ToUhc, sceCesUtf16beToUhc, sceCesUtf16leToUhc}, sceCesUtf32ToUhc, {sceCesUtf32beToUhc, sceCesUtf32leToUhc}};
    const WideForms big5Forms = {sceCesBig5ToUtf8, sceCesBig5ToUcs2, {sceCesBig5ToUtf16, sceCesBig5ToUtf16be, sceCesBig5ToUtf16le}, {sceCesBig5ToUtf32, sceCesBig5ToUtf32be, sceCesBig5ToUtf32le}, sceCesUcs2ToBig5, sceCesUtf8ToBig5, {sceCesUtf16ToBig5, sceCesUtf16beToBig5, sceCesUtf16leToBig5}, sceCesUtf32ToBig5, {sceCesUtf32beToBig5, sceCesUtf32leToBig5}};
    RequireWideForms(eucKrForms, eucKr, uhc);
    RequireWideForms(uhcForms, uhc, big5);
    RequireWideForms(big5Forms, big5, eucKr);

    alignas(8) std::uint8_t context[64] = {};
    Require(sceCesMbcsUcsContextInit(context, eucKr) == 0);
    RequireString(context, "\xC7\xD1\xB1\xB9\xBE\xEE A", "\xED\x95\x9C\xEA\xB5\xAD\xEC\x96\xB4 A");
    Require(Utf8ToMbcs(context, "\xEB\x98\xA0") == unassignedCode);
    Require(RoundTripMismatches(context) == 0);

    Require(sceCesMbcsUcsContextInit(context, uhc) == 0);
    RequireString(context, "\x8C\x63\xC7\xD1", "\xEB\x98\xA0\xED\x95\x9C");
    Require(Utf8ToMbcs(context, "\xE4\xB8\xAD") == 0);
    Require(Utf8ToMbcs(context, "\xE2\x82\xAC\xEF\xBF\xBF") == unassignedCode);
    Require(RoundTripMismatches(context) == 0);

    Require(sceCesMbcsUcsContextInit(context, big5) == 0);
    RequireString(context, "\xA4\xA4\xA4\xE5", "\xE4\xB8\xAD\xE6\x96\x87");
    RequireString(context, "\xF9\xF9\xA2\x7E\xA4\x51\xA4\xCA", "\xE2\x95\x90\xE2\x95\xAD\xE5\x8D\x81\xE5\x8D\x85");
    RequireString(context, "\xF9\xE9\xF9\xEB\xF9\xEA\xA2\xA1\xA2\xA3\xA2\xA2", "\xE2\x95\x9E\xE2\x95\xA1\xE2\x95\xAA\xE2\x95\xAE\xE2\x95\xAF\xE2\x95\xB0");
    Require(Utf8ToMbcs(context, "\xEA\xB0\x80") == unassignedCode);
    Require(RoundTripMismatches(context) == 10);
    return 0;
}
