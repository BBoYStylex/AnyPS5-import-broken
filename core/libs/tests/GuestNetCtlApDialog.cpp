#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>

extern "C" {
int APS5_VABI sceNetCtlApDialogInitialize(void);
int APS5_VABI sceNetCtlApDialogOpen(void);
int APS5_VABI sceNetCtlApDialogGetStatus(void);
int APS5_VABI sceNetCtlApDialogUpdateStatus(void);
int APS5_VABI sceNetCtlApDialogGetResult(void);
int APS5_VABI sceNetCtlApDialogClose(void);
int APS5_VABI sceNetCtlApDialogTerminate(void);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

constexpr int kStatusFinished = 3;
constexpr int kErrNotInitialized = static_cast<int>(0x80412101);
constexpr int kErrInvalidState = static_cast<int>(0x8041210E);
constexpr int kErrUserCanceled = static_cast<int>(0x80412110);

}

int main() {
    Require(sceNetCtlApDialogTerminate() == kErrNotInitialized);
    Require(sceNetCtlApDialogOpen() == kErrNotInitialized);
    Require(sceNetCtlApDialogUpdateStatus() == kErrNotInitialized);

    Require(sceNetCtlApDialogInitialize() == 0);
    Require(sceNetCtlApDialogInitialize() == kErrInvalidState);

    Require(sceNetCtlApDialogGetStatus() == 1);
    Require(sceNetCtlApDialogGetResult() == kErrUserCanceled);

    Require(sceNetCtlApDialogOpen() == 0);
    Require(sceNetCtlApDialogGetStatus() == kStatusFinished);
    Require(sceNetCtlApDialogUpdateStatus() == kStatusFinished);

    Require(sceNetCtlApDialogOpen() == 0);
    Require(sceNetCtlApDialogGetStatus() == kStatusFinished);

    Require(sceNetCtlApDialogClose() == 0);
    Require(sceNetCtlApDialogTerminate() == 0);
    Require(sceNetCtlApDialogTerminate() == kErrNotInitialized);
    return 0;
}
