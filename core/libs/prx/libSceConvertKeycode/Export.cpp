#include <cstdint>
#include <string>
#include "SceTypes.hpp"
#include "HitLog.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceKeyboard/include/KeyCharacters.hpp"

namespace {
constexpr int kErrInvalidAddress = static_cast<int>(0x80BC0031u);
constexpr int kErrInvalidUserId = static_cast<int>(0x80BC0010u);
constexpr std::int32_t kLayoutEnglishUs = 4;
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
 *character = Keyboard::KeyCharacter(false, (status >> 16) & 0xFFu, (status >> 8) & 0xFFu, keycode);
 return 0;
}

}
