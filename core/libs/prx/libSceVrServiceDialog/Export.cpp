#include "prx/libc/include/General.hpp"
#include "prx/libSceCommonDialog/OfflineDialog.hpp"

namespace {

OfflineDialog g_dialog;

}

extern "C" {

int APS5_VABI sceVrServiceDialogClose(void) {
    return g_dialog.Close();
}

int APS5_VABI sceVrServiceDialogGetResult(void* result) {
    return g_dialog.GetResult(result);
}

int APS5_VABI sceVrServiceDialogGetStatus(void) {
    return g_dialog.GetStatus();
}

int APS5_VABI sceVrServiceDialogInitialize(void) {
    return g_dialog.Initialize();
}

int APS5_VABI sceVrServiceDialogOpen(const void* param) {
    return g_dialog.Open(param);
}

int APS5_VABI sceVrServiceDialogTerminate(void) {
    return g_dialog.Terminate();
}

int APS5_VABI sceVrServiceDialogUpdateStatus(void) {
    return g_dialog.GetStatus();
}

}
