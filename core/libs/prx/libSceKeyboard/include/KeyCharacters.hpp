#ifndef CORE_LIBS_PRX_LIBSCEKEYBOARD_KEYCHARACTERS_HPP
#define CORE_LIBS_PRX_LIBSCEKEYBOARD_KEYCHARACTERS_HPP

#include <cstdint>

#include "prx/libSceKeyboard/include/keyboard_structs.h"

namespace Keyboard {

inline constexpr char US_NORMAL[] = "1234567890\n\0\b\t -=[]\\\0;'`,./";
inline constexpr char US_SHIFTED[] = "!@#$%^&*()\n\0\b\t _+{}|\0:\"~<>?";
inline constexpr char JIS_NORMAL[] = "1234567890\n\0\b\t -^@[]];:\0,./";
inline constexpr char JIS_SHIFTED[] = "!\"#$%&'()\0\n\0\b\t =~`{}}+*\0<>?";
inline constexpr char KEYPAD_DIGITS[] = "1234567890.";

inline std::uint16_t KeyCharacter(bool jis, std::uint32_t ledState, std::uint32_t modifierKey, std::uint16_t keyCode) {
    const bool shift = (modifierKey & (KEYBOARD_MOD_LEFT_SHIFT | KEYBOARD_MOD_RIGHT_SHIFT)) != 0;
    if (keyCode >= 0x04 && keyCode <= 0x1d) {
        const bool upper = shift != ((ledState & KEYBOARD_LED_CAPS_LOCK) != 0);
        return static_cast<std::uint16_t>((upper ? 'A' : 'a') + keyCode - 0x04);
    }
    if (keyCode >= 0x1e && keyCode <= 0x38) {
        const char* table = jis ? (shift ? JIS_SHIFTED : JIS_NORMAL) : (shift ? US_SHIFTED : US_NORMAL);
        return static_cast<unsigned char>(table[keyCode - 0x1e]);
    }
    if (jis && keyCode == 0x87) return shift ? '_' : '\\';
    if (jis && keyCode == 0x89) return shift ? '|' : '\\';
    switch (keyCode) {
        case 0x54: return '/';
        case 0x55: return '*';
        case 0x56: return '-';
        case 0x57: return '+';
        case 0x58: return '\n';
        default: break;
    }
    if (keyCode >= 0x59 && keyCode <= 0x63 && (ledState & KEYBOARD_LED_NUM_LOCK) != 0) {
        return static_cast<unsigned char>(KEYPAD_DIGITS[keyCode - 0x59]);
    }
    return 0;
}

}

#endif
