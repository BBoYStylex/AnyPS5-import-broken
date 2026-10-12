#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>

extern "C" {
int APS5_VABI sceConvertKeycodeGetCharacter(std::uint16_t keycode, std::uint32_t status, std::int32_t layout, std::uint32_t* character);
}

namespace {

constexpr int InvalidAddress = static_cast<int>(0x80BC0031u);
constexpr std::int32_t EnglishUs = 4;
constexpr std::uint32_t LeftShift = 1u << 1 << 8;
constexpr std::uint32_t RightShift = 1u << 5 << 8;
constexpr std::uint32_t LeftControl = 1u << 0 << 8;
constexpr std::uint32_t NumLock = 1u << 16;
constexpr std::uint32_t CapsLock = 1u << 17;

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "ConvertKeycode: %s\n", message);
        std::abort();
    }
}

std::uint32_t Character(std::uint16_t keycode, std::uint32_t status) {
    std::uint32_t character = 0xa5a5a5a5u;
    Require(sceConvertKeycodeGetCharacter(keycode, status, EnglishUs, &character) == 0, "conversion failed");
    return character;
}

}

int main() {
    Require(sceConvertKeycodeGetCharacter(0x04, 0, EnglishUs, nullptr) == InvalidAddress, "null character");

    Require(Character(0x04, 0) == 'a', "a");
    Require(Character(0x1D, 0) == 'z', "z");
    Require(Character(0x04, LeftShift) == 'A', "left shift a");
    Require(Character(0x1D, RightShift) == 'Z', "right shift z");
    Require(Character(0x04, CapsLock) == 'A', "caps lock a");
    Require(Character(0x04, CapsLock | LeftShift) == 'a', "caps lock and shift a");
    Require(Character(0x04, LeftShift) == Character(0x04, LeftShift | LeftControl), "control changes a letter");

    Require(Character(0x1E, 0) == '1', "1");
    Require(Character(0x1E, LeftShift) == '!', "shift 1");
    Require(Character(0x27, 0) == '0', "0");
    Require(Character(0x27, RightShift) == ')', "shift 0");
    Require(Character(0x1E, CapsLock) == '1', "caps lock changes a digit");

    Require(Character(0x28, 0) == '\n', "enter");
    Require(Character(0x29, 0) == 0, "escape");
    Require(Character(0x2A, 0) == '\b', "backspace");
    Require(Character(0x2B, 0) == '\t', "tab");
    Require(Character(0x2C, LeftShift) == ' ', "space");
    Require(Character(0x2D, LeftShift) == '_', "shift minus");
    Require(Character(0x31, 0) == '\\', "backslash");
    Require(Character(0x32, 0) == 0, "non-US hash key");
    Require(Character(0x34, LeftShift) == '"', "shift quote");
    Require(Character(0x35, LeftShift) == '~', "shift grave");
    Require(Character(0x38, LeftShift) == '?', "shift slash");
    Require(Character(0x4C, 0) == 0, "delete");

    Require(Character(0x3A, 0) == 0, "F1");
    Require(Character(0x4F, 0) == 0, "right arrow");
    Require(Character(0xE1, 0) == 0, "left shift key");

    Require(Character(0x54, 0) == '/', "keypad slash");
    Require(Character(0x57, 0) == '+', "keypad plus");
    Require(Character(0x58, 0) == '\n', "keypad enter");
    Require(Character(0x59, NumLock) == '1', "keypad 1");
    Require(Character(0x62, NumLock) == '0', "keypad 0");
    Require(Character(0x63, NumLock) == '.', "keypad point");
    Require(Character(0x59, 0) == 0, "keypad 1 without num lock");

    std::uint32_t character = 0;
    bool threw = false;
    try {
        sceConvertKeycodeGetCharacter(0x04, 0, 2, &character);
    } catch (const std::exception&) {
        threw = true;
    }
    Require(threw, "unsupported layout did not throw");
}
