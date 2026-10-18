#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdlib>

extern "C" {
int APS5_VABI sceMsgDialogInitialize(void);
int APS5_VABI sceMsgDialogTerminate(void);
int APS5_VABI sceMsgDialogOpen(const void* param);
int APS5_VABI sceMsgDialogGetStatus(void);
int APS5_VABI sceMsgDialogUpdateStatus(void);
int APS5_VABI sceMsgDialogGetResult(void* result);
int APS5_VABI sceMsgDialogClose(void);
int APS5_VABI sceMsgDialogProgressBarInc(int target, std::uint32_t delta);
int APS5_VABI sceMsgDialogProgressBarSetMsg(int target, const char* msg);
int APS5_VABI sceMsgDialogProgressBarSetValue(int target, std::uint32_t rate);
}

namespace {

constexpr int kStatusInitialized = 1;
constexpr int kStatusRunning = 2;
constexpr int kStatusFinished = 3;

constexpr int kErrAlreadyInitialized = static_cast<int>(0x80B80004u);
constexpr int kErrNotInitialized = static_cast<int>(0x80B80003u);
constexpr int kErrNotFinished = static_cast<int>(0x80B80005u);
constexpr int kErrNotRunning = static_cast<int>(0x80B8000Bu);
constexpr int kErrArgNull = static_cast<int>(0x80B8000Du);

void Require(bool value) { if (!value) std::abort(); }

}

int main() {
    std::uint8_t param[0x60]{};
    std::int32_t result[3]{};

    Require(sceMsgDialogTerminate() == kErrNotInitialized);
    Require(sceMsgDialogClose() == kErrNotRunning);
    Require(sceMsgDialogGetResult(result) == kErrNotFinished);
    Require(sceMsgDialogGetResult(nullptr) == kErrArgNull);
    Require(sceMsgDialogOpen(nullptr) == kErrArgNull);

    Require(sceMsgDialogInitialize() == 0);
    Require(sceMsgDialogInitialize() == kErrAlreadyInitialized);
    Require(sceMsgDialogClose() == kErrNotRunning);
    Require(sceMsgDialogGetStatus() == kStatusInitialized);
    Require(sceMsgDialogUpdateStatus() == kStatusInitialized);

    Require(sceMsgDialogOpen(param) == 0);
    Require(sceMsgDialogUpdateStatus() == kStatusRunning);
    Require(sceMsgDialogUpdateStatus() == kStatusFinished);
    Require(sceMsgDialogGetStatus() == kStatusFinished);

    Require(sceMsgDialogGetResult(result) == 0);
    Require(sceMsgDialogClose() == kErrNotRunning);

    Require(sceMsgDialogTerminate() == 0);
    Require(sceMsgDialogTerminate() == kErrNotInitialized);
}
