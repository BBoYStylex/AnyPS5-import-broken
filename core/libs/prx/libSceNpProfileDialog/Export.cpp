#include "prx/libc/include/General.hpp"
#include "prx/libSceCommonDialog/OfflineDialog.hpp"

namespace {

OfflineDialog g_dialog;

}

extern "C" {

int APS5_VABI sceNpProfileDialogClose(void) {
    return g_dialog.Close();
}

int APS5_VABI sceNpProfileDialogGetResult(void* result) {
    return g_dialog.GetResult(result);
}

int APS5_VABI sceNpProfileDialogGetStatus(void) {
    return g_dialog.GetStatus();
}

int APS5_VABI sceNpProfileDialogInitialize(void) {
    return g_dialog.Initialize();
}

int APS5_VABI sceNpProfileDialogOpen() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceNpProfileDialogOpenA(const void* param) {
    return g_dialog.Open(param);
}

int APS5_VABI sceNpProfileDialogTerminate(void) {
    return g_dialog.Terminate();
}

int APS5_VABI sceNpProfileDialogUpdateStatus(void) {
    return g_dialog.GetStatus();
}

}
