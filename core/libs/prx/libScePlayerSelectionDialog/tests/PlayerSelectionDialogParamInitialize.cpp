#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "SceTypes.hpp"
#include "prx/libSceCommonDialog/CommonDialog.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

extern "C" int APS5_VABI scePlayerSelectionDialogParamInitialize(PlayerSelectionDialogParam* param);
extern "C" int APS5_VABI sceUserServiceGetLoginUserIdList(UserServiceLoginUserIdList*) { return -1; }

namespace {

void Require(bool value) {
    if (!value) std::abort();
}

}

int main() {
    constexpr int kErrorArgNull = static_cast<int>(0x80B8000Du);
    Require(scePlayerSelectionDialogParamInitialize(nullptr) == kErrorArgNull);

    PlayerSelectionDialogParam param;
    std::memset(&param, 0xA5, sizeof(param));
    Require(scePlayerSelectionDialogParamInitialize(&param) == 0);

    Require(param.base.size == sizeof(param.base));
    Require(std::all_of(std::begin(param.base.reserved), std::end(param.base.reserved), [](std::uint8_t value) { return value == 0; }));
    Require(param.base.magic == static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&param.base) + 0xC0D1A109u));
    Require(param.size == sizeof(param));
    Require(param.userId == -1);
    Require(std::all_of(std::begin(param.title), std::end(param.title), [](char value) { return value == 0; }));
    Require(param.maxPlayers == 256);
    Require(param.mode == 0);
    Require(param.padding0 == 0);
    Require(param.players == nullptr);
    Require(param.playerCount == 0);
    Require(std::all_of(std::begin(param.reserved), std::end(param.reserved), [](std::uint8_t value) { return value == 0; }));
    Require(param.padding1 == 0);
}
