#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdlib>

extern "C" {
int APS5_VABI sceImeDialogAbort(void);
int APS5_VABI sceImeDialogGetPanelPositionAndForm(PositionAndForm* form);
int APS5_VABI sceImeDialogGetPanelSize(const Param* param, uint32_t* width, uint32_t* height);
int APS5_VABI sceImeDialogGetPanelSizeExtended(const Param* param, const ExtendedParam* extended, uint32_t* width, uint32_t* height);
int APS5_VABI sceImeDialogGetResult(Result* result);
int APS5_VABI sceImeDialogGetStatus(void);
int APS5_VABI sceImeDialogInit(const Param* param, const ExtendedParam* extended);
int APS5_VABI sceImeDialogTerm(void);
}

namespace {

constexpr int kStatusNone = 0;
constexpr int kStatusRunning = 1;
constexpr int kStatusFinished = 2;
constexpr int kErrorBusy = static_cast<int>(0x80bc0001u);
constexpr int kErrorNotOpened = static_cast<int>(0x80bc0002u);

void Require(bool value) { if (!value) std::abort(); }

}

int main() {
    PositionAndForm form{};
    Result result{};
    std::uint32_t width = 0;
    std::uint32_t height = 0;

    Require(sceImeDialogGetStatus() == kStatusNone);
    Require(sceImeDialogTerm() == kErrorNotOpened);
    Require(sceImeDialogAbort() == kErrorNotOpened);
    Require(sceImeDialogGetResult(&result) == kErrorNotOpened);
    Require(sceImeDialogGetResult(nullptr) == kErrorNotOpened);
    Require(sceImeDialogGetPanelPositionAndForm(nullptr) == kErrorNotOpened);

    Require(sceImeDialogGetPanelPositionAndForm(&form) == 0);
    Require(form.width == 1920);
    Require(form.height == 1080);

    Require(sceImeDialogGetPanelSize(nullptr, &width, &height) == 0);
    Require(width == 1920);
    Require(height == 1080);

    Require(sceImeDialogGetPanelSizeExtended(nullptr, nullptr, &width, &height) == 0);
    Require(width == 1920);
    Require(height == 1080);

    Require(sceImeDialogInit(nullptr, nullptr) == 0);
    Require(sceImeDialogInit(nullptr, nullptr) == kErrorBusy);

    Require(sceImeDialogGetStatus() == kStatusRunning);
    Require(sceImeDialogGetStatus() == kStatusFinished);

    Require(sceImeDialogGetResult(&result) == 0);
    Require(result.endstatus == 0);

    Require(sceImeDialogTerm() == 0);
    Require(sceImeDialogTerm() == kErrorNotOpened);
    Require(sceImeDialogGetStatus() == kStatusNone);
}
