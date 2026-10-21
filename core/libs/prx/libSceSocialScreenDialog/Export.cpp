#include "prx/libc/include/General.hpp"
#include "prx/libSceCommonDialog/OfflineDialog.hpp"

namespace {

OfflineDialog g_dialog;

}

extern "C" {

int APS5_VABI sceSocialScreenDialogClose(void) {
    return g_dialog.Close();
}

int APS5_VABI sceSocialScreenDialogGetResult(void* result) {
    return g_dialog.GetResult(result);
}

int APS5_VABI sceSocialScreenDialogGetStatus(void) {
    return g_dialog.GetStatus();
}

int APS5_VABI sceSocialScreenDialogInitialize(void) {
    return g_dialog.Initialize();
}

int APS5_VABI sceSocialScreenDialogOpen(const void* param) {
    return g_dialog.Open(param);
}

int APS5_VABI sceSocialScreenDialogTerminate(void) {
    return g_dialog.Terminate();
}

int APS5_VABI sceSocialScreenDialogUpdateStatus(void) {
    return g_dialog.GetStatus();
}

}
