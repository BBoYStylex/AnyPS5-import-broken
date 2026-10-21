#include "prx/libc/include/General.hpp"
#include "prx/libSceCommonDialog/OfflineDialog.hpp"

namespace {

OfflineDialog g_dialog;

}

extern "C" {

int APS5_VABI sceNpFriendListDialogClose(void) {
    return g_dialog.Close();
}

int APS5_VABI sceNpFriendListDialogGetResult() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceNpFriendListDialogGetResultA(void* result) {
    return g_dialog.GetResult(result);
}

int APS5_VABI sceNpFriendListDialogGetStatus(void) {
    return g_dialog.GetStatus();
}

int APS5_VABI sceNpFriendListDialogInitialize(void) {
    return g_dialog.Initialize();
}

int APS5_VABI sceNpFriendListDialogOpen() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceNpFriendListDialogOpenA(const void* param) {
    return g_dialog.Open(param);
}

int APS5_VABI sceNpFriendListDialogTerminate(void) {
    return g_dialog.Terminate();
}

int APS5_VABI sceNpFriendListDialogUpdateStatus(void) {
    return g_dialog.GetStatus();
}

}
