#include <cstdint>
#include <string>
#include "SceTypes.hpp"
#include "HitLog.hpp"
#include "prx/libc/include/General.hpp"

namespace {
constexpr int kErrInvalidAddress = static_cast<int>(0x80BC0031u);
constexpr int kErrInvalidUserId = static_cast<int>(0x80BC0010u);
constexpr std::int32_t kLayoutEnglishUs = 4;
constexpr std::uint32_t kShiftMask = (1u << 1 | 1u << 5) << 8;
constexpr std::uint32_t kNumLock = 1u << 16;
constexpr std::uint32_t kCapsLock = 1u << 17;

struct KeyCharacters {
    char plain;
    char shifted;
};

constexpr KeyCharacters kDigitRow[] = {
    {'1', '!'}, {'2', '@'}, {'3', '#'}, {'4', '$'}, {'5', '%'},
    {'6', '^'}, {'7', '&'}, {'8', '*'}, {'9', '('}, {'0', ')'},
};

constexpr KeyCharacters kPunctuation[] = {
    {'\r', '\r'}, {'\x1b', '\x1b'}, {'\b', '\b'}, {'\t', '\t'}, {' ', ' '},
    {'-', '_'}, {'=', '+'}, {'[', '{'}, {']', '}'}, {'\\', '|'}, {'\0', '\0'},
    {';', ':'}, {'\'', '"'}, {'`', '~'}, {',', '<'}, {'.', '>'}, {'/', '?'},
};

constexpr char kKeypadDigits[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '.'};

std::uint32_t EnglishUsCharacter(std::uint16_t keycode, std::uint32_t status) {
    const bool shift = (status & kShiftMask) != 0;
    if (keycode >= 0x04 && keycode <= 0x1D) {
        const bool upper = shift != ((status & kCapsLock) != 0);
        return static_cast<std::uint32_t>((upper ? 'A' : 'a') + (keycode - 0x04));
    }
    if (keycode >= 0x1E && keycode <= 0x27) {
        const auto& key = kDigitRow[keycode - 0x1E];
        return static_cast<unsigned char>(shift ? key.shifted : key.plain);
    }
    if (keycode >= 0x28 && keycode <= 0x38) {
        const auto& key = kPunctuation[keycode - 0x28];
        return static_cast<unsigned char>(shift ? key.shifted : key.plain);
    }
    switch (keycode) {
        case 0x4C: return 0x7F;
        case 0x54: return '/';
        case 0x55: return '*';
        case 0x56: return '-';
        case 0x57: return '+';
        case 0x58: return '\r';
        default: break;
    }
    if (keycode >= 0x59 && keycode <= 0x63) {
        return (status & kNumLock) != 0 ? static_cast<std::uint32_t>(kKeypadDigits[keycode - 0x59]) : 0;
    }
    return 0;
}

}  // namespace

extern "C" {

int APS5_VABI sceConvertKeycodeGetImeKeyboardType(int32_t userId, uint32_t* type) {
 if (type == nullptr) return kErrInvalidAddress;
 if (userId < 0) return kErrInvalidUserId;
 *type = 0;
 return 0;
}
int APS5_VABI sceConvertKeycodeGetVirtualKeycode(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3) {
 APS5_HIT("CONVERTKEYCODE", "sceConvertKeycodeGetVirtualKeycode args 0x%llx 0x%llx 0x%llx 0x%llx -> error (signature unknown)",
          static_cast<unsigned long long>(a0), static_cast<unsigned long long>(a1), static_cast<unsigned long long>(a2), static_cast<unsigned long long>(a3));
 return kErrInvalidAddress;
}

int APS5_VABI sceConvertKeycodeGetCharacterFromKeyboardData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceConvertKeycodeGetCharacter(std::uint16_t keycode, std::uint32_t status, std::int32_t layout, std::uint32_t* character) {
 if (character == nullptr) return kErrInvalidAddress;
 if (layout != kLayoutEnglishUs) {
  NotImplemented_nid_no_patch(("sceConvertKeycodeGetCharacter: keyboard layout " + std::to_string(layout)).c_str());
 }
 *character = EnglishUsCharacter(keycode, status);
 return 0;
}

}
