#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>

extern "C" {
int APS5_VABI sceHmdSetupDialogInitialize(void);
int APS5_VABI sceHmdSetupDialogOpen(const void* param);
int APS5_VABI sceHmdSetupDialogUpdateStatus(void);
int APS5_VABI sceHmdSetupDialogGetStatus(void);
int APS5_VABI sceHmdSetupDialogGetResult(void* result);
int APS5_VABI sceHmdSetupDialogClose(void);
int APS5_VABI sceHmdSetupDialogTerminate(void);

int APS5_VABI sceNpFriendListDialogInitialize(void);
int APS5_VABI sceNpFriendListDialogOpenA(const void* param);
int APS5_VABI sceNpFriendListDialogUpdateStatus(void);
int APS5_VABI sceNpFriendListDialogGetStatus(void);
int APS5_VABI sceNpFriendListDialogGetResultA(void* result);
int APS5_VABI sceNpFriendListDialogClose(void);
int APS5_VABI sceNpFriendListDialogTerminate(void);

int APS5_VABI sceNpProfileDialogInitialize(void);
int APS5_VABI sceNpProfileDialogOpenA(const void* param);
int APS5_VABI sceNpProfileDialogUpdateStatus(void);
int APS5_VABI sceNpProfileDialogGetStatus(void);
int APS5_VABI sceNpProfileDialogGetResult(void* result);
int APS5_VABI sceNpProfileDialogClose(void);
int APS5_VABI sceNpProfileDialogTerminate(void);

int APS5_VABI sceSocialScreenDialogInitialize(void);
int APS5_VABI sceSocialScreenDialogOpen(const void* param);
int APS5_VABI sceSocialScreenDialogUpdateStatus(void);
int APS5_VABI sceSocialScreenDialogGetStatus(void);
int APS5_VABI sceSocialScreenDialogGetResult(void* result);
int APS5_VABI sceSocialScreenDialogClose(void);
int APS5_VABI sceSocialScreenDialogTerminate(void);

int APS5_VABI sceVrServiceDialogInitialize(void);
int APS5_VABI sceVrServiceDialogOpen(const void* param);
int APS5_VABI sceVrServiceDialogUpdateStatus(void);
int APS5_VABI sceVrServiceDialogGetStatus(void);
int APS5_VABI sceVrServiceDialogGetResult(void* result);
int APS5_VABI sceVrServiceDialogClose(void);
int APS5_VABI sceVrServiceDialogTerminate(void);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

constexpr int kStatusNone = 0;
constexpr int kStatusInitialized = 1;
constexpr int kStatusFinished = 3;
constexpr int kErrNotInitialized = static_cast<int>(0x80B80003);
constexpr int kErrAlreadyInitialized = static_cast<int>(0x80B80004);
constexpr int kErrNotFinished = static_cast<int>(0x80B80005);
constexpr int kErrArgNull = static_cast<int>(0x80B8000D);
constexpr int kResultUserCanceled = 1;

struct Dialog {
    int (APS5_VABI* initialize)(void);
    int (APS5_VABI* open)(const void*);
    int (APS5_VABI* updateStatus)(void);
    int (APS5_VABI* getStatus)(void);
    int (APS5_VABI* getResult)(void*);
    int (APS5_VABI* close)(void);
    int (APS5_VABI* terminate)(void);
};

constexpr Dialog DIALOGS[] = {
    {sceHmdSetupDialogInitialize, sceHmdSetupDialogOpen, sceHmdSetupDialogUpdateStatus, sceHmdSetupDialogGetStatus,
        sceHmdSetupDialogGetResult, sceHmdSetupDialogClose, sceHmdSetupDialogTerminate},
    {sceNpFriendListDialogInitialize, sceNpFriendListDialogOpenA, sceNpFriendListDialogUpdateStatus, sceNpFriendListDialogGetStatus,
        sceNpFriendListDialogGetResultA, sceNpFriendListDialogClose, sceNpFriendListDialogTerminate},
    {sceNpProfileDialogInitialize, sceNpProfileDialogOpenA, sceNpProfileDialogUpdateStatus, sceNpProfileDialogGetStatus,
        sceNpProfileDialogGetResult, sceNpProfileDialogClose, sceNpProfileDialogTerminate},
    {sceSocialScreenDialogInitialize, sceSocialScreenDialogOpen, sceSocialScreenDialogUpdateStatus, sceSocialScreenDialogGetStatus,
        sceSocialScreenDialogGetResult, sceSocialScreenDialogClose, sceSocialScreenDialogTerminate},
    {sceVrServiceDialogInitialize, sceVrServiceDialogOpen, sceVrServiceDialogUpdateStatus, sceVrServiceDialogGetStatus,
        sceVrServiceDialogGetResult, sceVrServiceDialogClose, sceVrServiceDialogTerminate},
};

void CheckLifecycle(const Dialog& dialog) {
    std::int64_t param = 0;
    std::int32_t result[2] = {-1, -1};

    Require(dialog.terminate() == kErrNotInitialized);
    Require(dialog.open(&param) == kErrNotInitialized);
    Require(dialog.close() == kErrNotInitialized);
    Require(dialog.getResult(result) == kErrNotInitialized);
    Require(dialog.getStatus() == kStatusNone);
    Require(dialog.updateStatus() == kStatusNone);

    Require(dialog.initialize() == 0);
    Require(dialog.initialize() == kErrAlreadyInitialized);
    Require(dialog.getStatus() == kStatusInitialized);
    Require(dialog.updateStatus() == kStatusInitialized);

    Require(dialog.open(nullptr) == kErrArgNull);
    Require(dialog.getResult(result) == kErrNotFinished);

    Require(dialog.open(&param) == 0);
    Require(dialog.getStatus() == kStatusFinished);
    Require(dialog.updateStatus() == kStatusFinished);

    Require(dialog.getResult(nullptr) == kErrArgNull);
    Require(dialog.getResult(result) == 0);
    Require(result[0] == kResultUserCanceled);
    Require(result[1] == -1);

    Require(dialog.open(&param) == 0);
    Require(dialog.getStatus() == kStatusFinished);

    Require(dialog.close() == 0);
    Require(dialog.terminate() == 0);
    Require(dialog.terminate() == kErrNotInitialized);
    Require(dialog.getStatus() == kStatusNone);
    Require(dialog.updateStatus() == kStatusNone);

    Require(dialog.initialize() == 0);
    Require(dialog.close() == 0);
    Require(dialog.updateStatus() == kStatusFinished);
    result[0] = -1;
    Require(dialog.getResult(result) == 0);
    Require(result[0] == kResultUserCanceled);
    Require(dialog.terminate() == 0);
}

void CheckIndependentState(const Dialog& opened) {
    std::int64_t param = 0;
    Require(opened.initialize() == 0);
    Require(opened.open(&param) == 0);
    for (const Dialog& other : DIALOGS) {
        if (&other != &opened) Require(other.getStatus() == kStatusNone);
    }
    Require(opened.terminate() == 0);
}

}

int main() {
    for (const Dialog& dialog : DIALOGS) CheckLifecycle(dialog);
    for (const Dialog& dialog : DIALOGS) CheckIndependentState(dialog);
}
