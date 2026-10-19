#include <cstdint>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "HitLog.hpp"
#include "prx/libc/include/General.hpp"

namespace {
constexpr int kErrInvalidAddress = static_cast<int>(0x80BC0031u);
constexpr int kErrInvalidUserId = static_cast<int>(0x80BC0010u);
constexpr int kErrInvalidType = static_cast<int>(0x80BC0011u);

constexpr std::uint32_t kIgnoredStateBits = 0x4000000u;
constexpr std::uint32_t kKnownStateBits = 0x000000FFu | 0x0000FF00u | 0x00070000u | kIgnoredStateBits;

constexpr std::uint16_t VK_BACK = 0x08;
constexpr std::uint16_t VK_TAB = 0x09;
constexpr std::uint16_t VK_RETURN = 0x0D;
constexpr std::uint16_t VK_SHIFT = 0x10;
constexpr std::uint16_t VK_CONTROL = 0x11;
constexpr std::uint16_t VK_MENU = 0x12;
constexpr std::uint16_t VK_PAUSE = 0x13;
constexpr std::uint16_t VK_CAPITAL = 0x14;
constexpr std::uint16_t VK_KANA = 0x15;
constexpr std::uint16_t VK_CONVERT = 0x1C;
constexpr std::uint16_t VK_NONCONVERT = 0x1D;
constexpr std::uint16_t VK_ESCAPE = 0x1B;
constexpr std::uint16_t VK_SPACE = 0x20;
constexpr std::uint16_t VK_PRIOR = 0x21;
constexpr std::uint16_t VK_NEXT = 0x22;
constexpr std::uint16_t VK_END = 0x23;
constexpr std::uint16_t VK_HOME = 0x24;
constexpr std::uint16_t VK_LEFT = 0x25;
constexpr std::uint16_t VK_UP = 0x26;
constexpr std::uint16_t VK_RIGHT = 0x27;
constexpr std::uint16_t VK_DOWN = 0x28;
constexpr std::uint16_t VK_SNAPSHOT = 0x2C;
constexpr std::uint16_t VK_INSERT = 0x2D;
constexpr std::uint16_t VK_DELETE = 0x2E;
constexpr std::uint16_t VK_LWIN = 0x5B;
constexpr std::uint16_t VK_RWIN = 0x5C;
constexpr std::uint16_t VK_APPS = 0x5D;
constexpr std::uint16_t VK_NUMPAD0 = 0x60;
constexpr std::uint16_t VK_NUMPAD1 = 0x61;
constexpr std::uint16_t VK_MULTIPLY = 0x6A;
constexpr std::uint16_t VK_ADD = 0x6B;
constexpr std::uint16_t VK_SUBTRACT = 0x6D;
constexpr std::uint16_t VK_DECIMAL = 0x6E;
constexpr std::uint16_t VK_DIVIDE = 0x6F;
constexpr std::uint16_t VK_F1 = 0x70;
constexpr std::uint16_t VK_F13 = 0x7C;
constexpr std::uint16_t VK_NUMLOCK = 0x90;
constexpr std::uint16_t VK_SCROLL = 0x91;
constexpr std::uint16_t VK_LSHIFT = 0xA0;
constexpr std::uint16_t VK_RSHIFT = 0xA1;
constexpr std::uint16_t VK_LCONTROL = 0xA2;
constexpr std::uint16_t VK_RCONTROL = 0xA3;
constexpr std::uint16_t VK_LMENU = 0xA4;
constexpr std::uint16_t VK_RMENU = 0xA5;
constexpr std::uint16_t VK_OEM_1 = 0xBA;
constexpr std::uint16_t VK_OEM_PLUS = 0xBB;
constexpr std::uint16_t VK_OEM_COMMA = 0xBC;
constexpr std::uint16_t VK_OEM_MINUS = 0xBD;
constexpr std::uint16_t VK_OEM_PERIOD = 0xBE;
constexpr std::uint16_t VK_OEM_2 = 0xBF;
constexpr std::uint16_t VK_OEM_3 = 0xC0;
constexpr std::uint16_t VK_OEM_4 = 0xDB;
constexpr std::uint16_t VK_OEM_5 = 0xDC;
constexpr std::uint16_t VK_OEM_6 = 0xDD;
constexpr std::uint16_t VK_OEM_7 = 0xDE;
constexpr std::uint16_t VK_OEM_8 = 0xDF;
constexpr std::uint16_t VK_OEM_102 = 0xE2;

enum class LayoutCategory {
    Us,
    Uk,
    German,
    French,
    Spanish,
    Italian,
    Japanese,
    Unsupported,
};

LayoutCategory CategorizeLayout(std::uint32_t type) {
    switch (type) {
        case 0:
        case 4:
        case 11:
            return LayoutCategory::Us;
        case 5:
            return LayoutCategory::Uk;
        case 2:
        case 3:
            return LayoutCategory::German;
        case 9:
        case 10:
        case 12:
            return LayoutCategory::French;
        case 6:
        case 7:
            return LayoutCategory::Spanish;
        case 13:
            return LayoutCategory::Italian;
        case 22:
        case 23:
            return LayoutCategory::Japanese;
        default:
            return LayoutCategory::Unsupported;
    }
}

std::uint16_t MapVirtualKeycode(std::uint16_t keycode, LayoutCategory category) {
    if (keycode >= 0x04 && keycode <= 0x1D) {
        if (category == LayoutCategory::German) {
            if (keycode == 0x1C) return static_cast<std::uint16_t>('Z');
            if (keycode == 0x16) return static_cast<std::uint16_t>('Y');
        } else if (category == LayoutCategory::French) {
            if (keycode == 0x04) return static_cast<std::uint16_t>('Q');
            if (keycode == 0x14) return static_cast<std::uint16_t>('A');
            if (keycode == 0x1A) return static_cast<std::uint16_t>('Z');
            if (keycode == 0x1D) return static_cast<std::uint16_t>('W');
            if (keycode == 0x10) return VK_OEM_COMMA;
        }
        return static_cast<std::uint16_t>('A' + (keycode - 0x04));
    }
    if (keycode >= 0x1E && keycode <= 0x26) return static_cast<std::uint16_t>('1' + (keycode - 0x1E));
    if (keycode == 0x27) return static_cast<std::uint16_t>('0');
    if (keycode == 0x28) return VK_RETURN;
    if (keycode == 0x29) return VK_ESCAPE;
    if (keycode == 0x2A) return VK_BACK;
    if (keycode == 0x2B) return VK_TAB;
    if (keycode == 0x2C) return VK_SPACE;
    if (keycode == 0x2D) return (category == LayoutCategory::German) ? VK_OEM_4 : VK_OEM_MINUS;
    if (keycode == 0x2E) return (category == LayoutCategory::German) ? VK_OEM_6 : ((category == LayoutCategory::Japanese) ? VK_OEM_6 : VK_OEM_PLUS);
    if (keycode == 0x2F) return (category == LayoutCategory::German) ? VK_OEM_1 : ((category == LayoutCategory::Japanese) ? VK_OEM_3 : VK_OEM_4);
    if (keycode == 0x30) return (category == LayoutCategory::German) ? VK_OEM_PLUS : VK_OEM_6;
    if (keycode == 0x31) return VK_OEM_5;
    if (keycode == 0x32) return (category == LayoutCategory::Uk) ? VK_OEM_7 : VK_OEM_5;
    if (keycode == 0x33) return (category == LayoutCategory::French) ? static_cast<std::uint16_t>('M') : ((category == LayoutCategory::German) ? VK_OEM_3 : VK_OEM_1);
    if (keycode == 0x34) return (category == LayoutCategory::Japanese) ? VK_OEM_1 : VK_OEM_7;
    if (keycode == 0x35) return (category == LayoutCategory::French) ? VK_OEM_8 : VK_OEM_3;
    if (keycode == 0x36) return VK_OEM_COMMA;
    if (keycode == 0x37) return VK_OEM_PERIOD;
    if (keycode == 0x38) return (category == LayoutCategory::German) ? VK_OEM_MINUS : VK_OEM_2;
    if (keycode == 0x39) return VK_CAPITAL;
    if (keycode >= 0x3A && keycode <= 0x45) return static_cast<std::uint16_t>(VK_F1 + (keycode - 0x3A));
    if (keycode == 0x46) return VK_SNAPSHOT;
    if (keycode == 0x47) return VK_SCROLL;
    if (keycode == 0x48) return VK_PAUSE;
    if (keycode == 0x49) return VK_INSERT;
    if (keycode == 0x4A) return VK_HOME;
    if (keycode == 0x4B) return VK_PRIOR;
    if (keycode == 0x4C) return VK_DELETE;
    if (keycode == 0x4D) return VK_END;
    if (keycode == 0x4E) return VK_NEXT;
    if (keycode == 0x4F) return VK_RIGHT;
    if (keycode == 0x50) return VK_LEFT;
    if (keycode == 0x51) return VK_DOWN;
    if (keycode == 0x52) return VK_UP;
    if (keycode == 0x53) return VK_NUMLOCK;
    if (keycode == 0x54) return VK_DIVIDE;
    if (keycode == 0x55) return VK_MULTIPLY;
    if (keycode == 0x56) return VK_SUBTRACT;
    if (keycode == 0x57) return VK_ADD;
    if (keycode == 0x58) return VK_RETURN;
    if (keycode >= 0x59 && keycode <= 0x61) return static_cast<std::uint16_t>(VK_NUMPAD1 + (keycode - 0x59));
    if (keycode == 0x62) return VK_NUMPAD0;
    if (keycode == 0x63) return VK_DECIMAL;
    if (keycode == 0x64) return VK_OEM_102;
    if (keycode == 0x65) return VK_APPS;
    if (keycode >= 0x68 && keycode <= 0x73) return static_cast<std::uint16_t>(VK_F13 + (keycode - 0x68));
    if (category == LayoutCategory::Japanese) {
        if (keycode == 0x87) return VK_OEM_102;
        if (keycode == 0x88) return VK_KANA;
        if (keycode == 0x89) return VK_OEM_5;
        if (keycode == 0x8A) return VK_CONVERT;
        if (keycode == 0x8B) return VK_NONCONVERT;
    }
    if (keycode == 0xE0) return VK_LCONTROL;
    if (keycode == 0xE1) return VK_LSHIFT;
    if (keycode == 0xE2) return VK_LMENU;
    if (keycode == 0xE3) return VK_LWIN;
    if (keycode == 0xE4) return VK_RCONTROL;
    if (keycode == 0xE5) return VK_RSHIFT;
    if (keycode == 0xE6) return VK_RMENU;
    if (keycode == 0xE7) return VK_RWIN;
    return 0;
}

char16_t KeypadCharacter(std::uint16_t keycode) {
    if (keycode == 0x54) return u'/';
    if (keycode == 0x55) return u'*';
    if (keycode == 0x56) return u'-';
    if (keycode == 0x57) return u'+';
    static constexpr char16_t kDigits[] = u"1234567890.";
    if (keycode >= 0x59 && keycode <= 0x63) return kDigits[keycode - 0x59];
    return 0;
}

char16_t ConvertCharacter(std::uint16_t keycode, bool shift, bool caps, bool altGr, LayoutCategory category) {
    if (keycode == 0x2C) return u' ';
    if (keycode >= 0x54 && keycode <= 0x63) return KeypadCharacter(keycode);

    const bool upper = shift ^ caps;

    if (category == LayoutCategory::Us) {
        static constexpr char16_t kPuncNormal[] = u"-=[]\\\0;'`,./";
        static constexpr char16_t kPuncShifted[] = u"_+{}|\0:\"~<>?";
        static constexpr char16_t kDigitsShifted[] = u"!@#$%^&*()";
        if (keycode >= 0x04 && keycode <= 0x1D) return static_cast<char16_t>((upper ? u'A' : u'a') + (keycode - 0x04));
        if (keycode >= 0x1E && keycode <= 0x27) return shift ? kDigitsShifted[keycode - 0x1E] : static_cast<char16_t>(keycode == 0x27 ? u'0' : u'1' + (keycode - 0x1E));
        if (keycode >= 0x2D && keycode <= 0x38) return shift ? kPuncShifted[keycode - 0x2D] : kPuncNormal[keycode - 0x2D];
        if (keycode == 0x64) return shift ? u'|' : u'\\';
        return 0;
    }

    if (category == LayoutCategory::Japanese) {
        static constexpr char16_t kDigitsShifted[] = u"!\"#$%&'()\0";
        static constexpr char16_t kPuncNormal[] = u"-^@[]];:\0,./";
        static constexpr char16_t kPuncShifted[] = u"=~`{}}+*\0<>?";
        if (keycode >= 0x04 && keycode <= 0x1D) return static_cast<char16_t>((upper ? u'A' : u'a') + (keycode - 0x04));
        if (keycode >= 0x1E && keycode <= 0x27) return shift ? kDigitsShifted[keycode - 0x1E] : static_cast<char16_t>(keycode == 0x27 ? u'0' : u'1' + (keycode - 0x1E));
        if (keycode >= 0x2D && keycode <= 0x38) return shift ? kPuncShifted[keycode - 0x2D] : kPuncNormal[keycode - 0x2D];
        if (keycode == 0x87) return shift ? u'_' : u'\\';
        if (keycode == 0x89) return shift ? u'|' : u'\\';
        return 0;
    }

    if (category == LayoutCategory::Uk) {
        if (altGr) {
            if (keycode == 0x08 || keycode == 0x21) return u'\u20AC';
            if (keycode == 0x35) return u'|';
            return 0;
        }
        static constexpr char16_t kDigitsShifted[] = u"!\"\u00A3$%^&*()";
        static constexpr char16_t kPuncNormal[] = u"-=[]#\0;'`,./";
        static constexpr char16_t kPuncShifted[] = u"_+{}~\0:@~<>?";
        if (keycode >= 0x04 && keycode <= 0x1D) return static_cast<char16_t>((upper ? u'A' : u'a') + (keycode - 0x04));
        if (keycode >= 0x1E && keycode <= 0x27) return shift ? kDigitsShifted[keycode - 0x1E] : static_cast<char16_t>(keycode == 0x27 ? u'0' : u'1' + (keycode - 0x1E));
        if (keycode == 0x31 || keycode == 0x32) return shift ? u'~' : u'#';
        if (keycode == 0x35) return shift ? u'\u00AC' : u'`';
        if (keycode >= 0x2D && keycode <= 0x38) return shift ? kPuncShifted[keycode - 0x2D] : kPuncNormal[keycode - 0x2D];
        if (keycode == 0x64) return shift ? u'|' : u'\\';
        return 0;
    }

    if (category == LayoutCategory::German) {
        if (altGr) {
            if (keycode == 0x14) return u'@';
            if (keycode == 0x08) return u'\u20AC';
            if (keycode == 0x24) return u'{';
            if (keycode == 0x25) return u'[';
            if (keycode == 0x26) return u']';
            if (keycode == 0x27) return u'}';
            if (keycode == 0x2D) return u'\\';
            if (keycode == 0x30) return u'~';
            if (keycode == 0x64) return u'|';
            return 0;
        }
        if (keycode >= 0x04 && keycode <= 0x1D) {
            if (keycode == 0x1C) return upper ? u'Z' : u'z';
            if (keycode == 0x16) return upper ? u'Y' : u'y';
            return static_cast<char16_t>((upper ? u'A' : u'a') + (keycode - 0x04));
        }
        static constexpr char16_t kDigitsShifted[] = u"!\"\u00A7$%&/()=";
        if (keycode >= 0x1E && keycode <= 0x27) return shift ? kDigitsShifted[keycode - 0x1E] : static_cast<char16_t>(keycode == 0x27 ? u'0' : u'1' + (keycode - 0x1E));
        if (keycode == 0x2D) return shift ? u'?' : u'\u00DF';
        if (keycode == 0x2E) return shift ? u'`' : u'\u00B4';
        if (keycode == 0x2F) return upper ? u'\u00DC' : u'\u00FC';
        if (keycode == 0x30) return shift ? u'*' : u'+';
        if (keycode == 0x31 || keycode == 0x32) return shift ? u'\'' : u'#';
        if (keycode == 0x33) return upper ? u'\u00D6' : u'\u00F6';
        if (keycode == 0x34) return upper ? u'\u00C4' : u'\u00E4';
        if (keycode == 0x35) return shift ? u'\u00B0' : u'^';
        if (keycode == 0x36) return shift ? u';' : u',';
        if (keycode == 0x37) return shift ? u':' : u'.';
        if (keycode == 0x38) return shift ? u'_' : u'-';
        if (keycode == 0x64) return shift ? u'>' : u'<';
        return 0;
    }

    if (category == LayoutCategory::French) {
        if (altGr) {
            if (keycode == 0x08) return u'\u20AC';
            if (keycode == 0x1F) return u'~';
            if (keycode == 0x20) return u'#';
            if (keycode == 0x21) return u'{';
            if (keycode == 0x22) return u'[';
            if (keycode == 0x23) return u'|';
            if (keycode == 0x24) return u'`';
            if (keycode == 0x25) return u'\\';
            if (keycode == 0x26) return u'^';
            if (keycode == 0x27) return u'@';
            if (keycode == 0x2D) return u']';
            if (keycode == 0x2E) return u'}';
            if (keycode == 0x30) return u'\u00A4';
            return 0;
        }
        if (keycode >= 0x04 && keycode <= 0x1D) {
            if (keycode == 0x04) return upper ? u'Q' : u'q';
            if (keycode == 0x14) return upper ? u'A' : u'a';
            if (keycode == 0x1A) return upper ? u'Z' : u'z';
            if (keycode == 0x1D) return upper ? u'W' : u'w';
            if (keycode == 0x10) return shift ? u'?' : u',';
            return static_cast<char16_t>((upper ? u'A' : u'a') + (keycode - 0x04));
        }
        static constexpr char16_t kDigitsNormal[] = u"&\u00E9\"'(-\u00E8_\u00E7\u00E0";
        if (keycode >= 0x1E && keycode <= 0x27) return shift ? static_cast<char16_t>(keycode == 0x27 ? u'0' : u'1' + (keycode - 0x1E)) : kDigitsNormal[keycode - 0x1E];
        if (keycode == 0x2D) return shift ? u'\u00B0' : u')';
        if (keycode == 0x2E) return shift ? u'+' : u'=';
        if (keycode == 0x2F) return shift ? u'\u00A8' : u'^';
        if (keycode == 0x30) return shift ? u'\u00A3' : u'$';
        if (keycode == 0x31 || keycode == 0x32) return shift ? u'\u00B5' : u'*';
        if (keycode == 0x33) return upper ? u'M' : u'm';
        if (keycode == 0x34) return shift ? u'%' : u'\u00F9';
        if (keycode == 0x35) return shift ? 0 : u'\u00B2';
        if (keycode == 0x36) return shift ? u'.' : u';';
        if (keycode == 0x37) return shift ? u'/' : u':';
        if (keycode == 0x38) return shift ? u'\u00A7' : u'!';
        if (keycode == 0x64) return shift ? u'>' : u'<';
        return 0;
    }

    if (category == LayoutCategory::Spanish) {
        if (altGr) {
            if (keycode == 0x08) return u'\u20AC';
            if (keycode == 0x1E) return u'|';
            if (keycode == 0x1F) return u'@';
            if (keycode == 0x20) return u'#';
            if (keycode == 0x21) return u'~';
            if (keycode == 0x23) return u'\u00AC';
            if (keycode == 0x2F) return u'[';
            if (keycode == 0x30) return u']';
            if (keycode == 0x31 || keycode == 0x32) return u'}';
            if (keycode == 0x34) return u'{';
            if (keycode == 0x35) return u'\\';
            return 0;
        }
        if (keycode >= 0x04 && keycode <= 0x1D) return static_cast<char16_t>((upper ? u'A' : u'a') + (keycode - 0x04));
        static constexpr char16_t kDigitsShifted[] = u"!\"\u00B7$%&/()=";
        if (keycode >= 0x1E && keycode <= 0x27) return shift ? kDigitsShifted[keycode - 0x1E] : static_cast<char16_t>(keycode == 0x27 ? u'0' : u'1' + (keycode - 0x1E));
        if (keycode == 0x2D) return shift ? u'?' : u'\'';
        if (keycode == 0x2E) return shift ? u'\u00BF' : u'\u00A1';
        if (keycode == 0x2F) return shift ? u'^' : u'`';
        if (keycode == 0x30) return shift ? u'*' : u'+';
        if (keycode == 0x31 || keycode == 0x32) return upper ? u'\u00C7' : u'\u00E7';
        if (keycode == 0x33) return upper ? u'\u00D1' : u'\u00F1';
        if (keycode == 0x34) return shift ? u'\u00A8' : u'\u00B4';
        if (keycode == 0x35) return shift ? u'\u00AA' : u'\u00BA';
        if (keycode == 0x36) return shift ? u';' : u',';
        if (keycode == 0x37) return shift ? u':' : u'.';
        if (keycode == 0x38) return shift ? u'_' : u'-';
        if (keycode == 0x64) return shift ? u'>' : u'<';
        return 0;
    }

    if (category == LayoutCategory::Italian) {
        if (altGr) {
            if (keycode == 0x08) return u'\u20AC';
            if (keycode == 0x2F) return shift ? u'{' : u'[';
            if (keycode == 0x30) return shift ? u'}' : u']';
            if (keycode == 0x33) return u'@';
            if (keycode == 0x34) return u'#';
            return 0;
        }
        if (keycode >= 0x04 && keycode <= 0x1D) return static_cast<char16_t>((upper ? u'A' : u'a') + (keycode - 0x04));
        static constexpr char16_t kDigitsShifted[] = u"!\"\u00A3$%&/()=";
        if (keycode >= 0x1E && keycode <= 0x27) return shift ? kDigitsShifted[keycode - 0x1E] : static_cast<char16_t>(keycode == 0x27 ? u'0' : u'1' + (keycode - 0x1E));
        if (keycode == 0x2D) return shift ? u'?' : u'\'';
        if (keycode == 0x2E) return shift ? u'^' : u'\u00EC';
        if (keycode == 0x2F) return shift ? u'\u00E9' : u'\u00E8';
        if (keycode == 0x30) return shift ? u'*' : u'+';
        if (keycode == 0x31 || keycode == 0x32) return shift ? u'\u00A7' : u'\u00F9';
        if (keycode == 0x33) return shift ? u'\u00E7' : u'\u00F2';
        if (keycode == 0x34) return shift ? u'\u00B0' : u'\u00E0';
        if (keycode == 0x35) return shift ? u'|' : u'\\';
        if (keycode == 0x36) return shift ? u';' : u',';
        if (keycode == 0x37) return shift ? u':' : u'.';
        if (keycode == 0x38) return shift ? u'_' : u'-';
        if (keycode == 0x64) return shift ? u'>' : u'<';
        return 0;
    }

    return 0;
}

[[noreturn]] void ThrowUnsupportedState(std::uint32_t state) {
    throw std::runtime_error("sceConvertKeycodeGetCharacter: unsupported state " + std::to_string(state));
}

[[noreturn]] void ThrowUnsupportedType(std::uint32_t type) {
    throw std::runtime_error("sceConvertKeycodeGetCharacter: unsupported keyboard type " + std::to_string(type));
}
}  // namespace

extern "C" {

int APS5_VABI sceConvertKeycodeGetImeKeyboardType(int32_t userId, uint32_t* type) {
    if (type == nullptr) return kErrInvalidAddress;
    if (userId < 0) return kErrInvalidUserId;
    *type = 0;
    return 0;
}

int APS5_VABI sceConvertKeycodeGetVirtualKeycode(uint16_t keycode, uint32_t keyboardType, uint16_t* vkeycode) {
    if (vkeycode == nullptr) return kErrInvalidAddress;
    if (keyboardType > 37) return kErrInvalidType;
    const LayoutCategory category = CategorizeLayout(keyboardType);
    *vkeycode = MapVirtualKeycode(keycode, category);
    return 0;
}

int APS5_VABI sceConvertKeycodeGetCharacterFromKeyboardData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceConvertKeycodeGetCharacter(std::uint16_t keycode, std::uint32_t state, std::uint32_t type, char16_t* character) {
    if (character == nullptr) return kErrInvalidAddress;
    if ((state & ~kKnownStateBits) != 0) ThrowUnsupportedState(state);
    if (type > 37) return kErrInvalidType;
    const LayoutCategory category = CategorizeLayout(type);
    if (category == LayoutCategory::Unsupported) ThrowUnsupportedType(type);
    const bool shift = (state & (0x0200u | 0x2000u | 0x02u | 0x20u)) != 0;
    const bool caps = (state & 0x20000u) != 0;
    const bool altGr = (state & (0x4000u | 0x40u)) != 0;
    *character = ConvertCharacter(keycode, shift, caps, altGr, category);
    return 0;
}

}
