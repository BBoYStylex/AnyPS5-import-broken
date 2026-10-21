#include "prx/libc/include/General.hpp"
#include "prx/libSceCommonDialog/OfflineDialog.hpp"

namespace {

OfflineDialog g_dialog;

}

extern "C" {

int APS5_VABI sceHmdSetupDialogClose(void) {
    return g_dialog.Close();
}

int APS5_VABI sceHmdSetupDialogGetResult(void* result) {
    return g_dialog.GetResult(result);
}

int APS5_VABI sceHmdSetupDialogGetStatus(void) {
    return g_dialog.GetStatus();
}

int APS5_VABI sceHmdSetupDialogInitialize(void) {
    return g_dialog.Initialize();
}

int APS5_VABI sceHmdSetupDialogOpen(const void* param) {
    return g_dialog.Open(param);
}

int APS5_VABI sceHmdSetupDialogTerminate(void) {
    return g_dialog.Terminate();
}

int APS5_VABI sceHmdSetupDialogUpdateStatus(void) {
    return g_dialog.GetStatus();
}

}
