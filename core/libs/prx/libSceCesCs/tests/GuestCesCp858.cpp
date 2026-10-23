#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" {
const std::uint8_t* APS5_VABI sceCesRefersUcsProfileCp858(void);
int APS5_VABI sceCesSbcToUtf8(const std::uint8_t*, std::uint8_t, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf8ToSbc(const std::uint8_t*, std::uint32_t, std::uint32_t*, const std::uint8_t*, std::uint8_t*);
int APS5_VABI sceCesMbcsUcsContextInit(void*, const void*);
int APS5_VABI sceCesMbcsStrToUtf8Str(void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf8StrToMbcsStr(void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint8_t*, std::uint32_t, std::uint32_t*);
}

static void Require(bool value) { if (!value) std::abort(); }

static void RequireDecode(const std::uint8_t* profile, std::uint8_t input, const char* expected) {
    std::uint8_t output[4] = {};
    std::uint32_t length = 0;
    Require(sceCesSbcToUtf8(profile, input, output, sizeof(output), &length) == 0);
    Require(length == std::strlen(expected));
    Require(std::memcmp(output, expected, length) == 0);
}

int main() {
    constexpr int unassignedCode = static_cast<int>(0x805C0020);
    const std::uint8_t* profile = sceCesRefersUcsProfileCp858();
    Require(profile != nullptr);
    Require(profile == sceCesRefersUcsProfileCp858());
    RequireDecode(profile, 'A', "A");
    RequireDecode(profile, 0x80, "\xC3\x87");
    RequireDecode(profile, 0xB3, "\xE2\x94\x82");
    RequireDecode(profile, 0xD5, "\xE2\x82\xAC");
    RequireDecode(profile, 0xFF, "\xC2\xA0");

    const std::uint8_t euro[] = {0xE2, 0x82, 0xAC};
    std::uint8_t encoded = 0;
    std::uint32_t consumed = 0;
    Require(sceCesUtf8ToSbc(euro, sizeof(euro), &consumed, profile, &encoded) == 0);
    Require(consumed == 3 && encoded == 0xD5);

    const std::uint8_t dotlessI[] = {0xC4, 0xB1};
    encoded = 0x5A;
    consumed = 0x5A;
    Require(sceCesUtf8ToSbc(dotlessI, sizeof(dotlessI), &consumed, profile, &encoded) == unassignedCode);
    Require(consumed == 0x5A && encoded == 0x5A);

    alignas(void*) std::uint8_t context[256]{};
    Require(sceCesMbcsUcsContextInit(context, profile) == 0);
    const std::uint8_t input[] = {0xD5, 'A', 0x80};
    const std::uint8_t expected[] = {0xE2, 0x82, 0xAC, 'A', 0xC3, 0x87};
    std::uint8_t decoded[16]{};
    std::uint32_t used = 0;
    std::uint32_t produced = 0;
    Require(sceCesMbcsStrToUtf8Str(context, input, sizeof(input), &used, decoded, sizeof(decoded), &produced) == 0);
    Require(used == sizeof(input) && produced == sizeof(expected));
    Require(std::memcmp(decoded, expected, sizeof(expected)) == 0);
    std::uint8_t roundTripString[16]{};
    Require(sceCesUtf8StrToMbcsStr(context, expected, sizeof(expected), &used, roundTripString, sizeof(roundTripString), &produced) == 0);
    Require(used == sizeof(expected) && produced == sizeof(input));
    Require(std::memcmp(roundTripString, input, sizeof(input)) == 0);

    for (std::uint32_t byte = 0; byte < 256; ++byte) {
        std::uint8_t utf8[4] = {};
        std::uint32_t utf8Length = 0;
        std::uint8_t roundTrip = 0;
        Require(sceCesSbcToUtf8(profile, static_cast<std::uint8_t>(byte), utf8, sizeof(utf8), &utf8Length) == 0);
        Require(sceCesUtf8ToSbc(utf8, utf8Length, &consumed, profile, &roundTrip) == 0);
        Require(consumed == utf8Length && roundTrip == byte);
    }
}
