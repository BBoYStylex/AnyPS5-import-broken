#include "CommonDialog.hpp"
#include "SceTypes.hpp"

namespace {

static bool g_initialized = false;

}

extern "C" {

int APS5_VABI sceCommonDialogInitialize(void) {
    if (g_initialized) {
        return COMMON_DIALOG_ERROR_ALREADY_INITIALIZED;
    }
    g_initialized = true;
    return COMMON_DIALOG_OK;
}

int APS5_VABI sceCommonDialogTerminate(void) {
    if (!g_initialized) {
        return COMMON_DIALOG_ERROR_NOT_SYSTEM_INITIALIZED;
    }
    g_initialized = false;
    return COMMON_DIALOG_OK;
}

bool APS5_VABI sceCommonDialogIsUsed(void) {
    return false;
}

}
