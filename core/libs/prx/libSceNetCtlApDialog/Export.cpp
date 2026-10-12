#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <atomic>
#include <stdexcept>
#include <string>

namespace {

constexpr int COMMON_DIALOG_STATUS_NONE = 0;
constexpr int COMMON_DIALOG_STATUS_FINISHED = 3;
constexpr int COMMON_DIALOG_RESULT_USER_CANCELED = 1;
constexpr int COMMON_DIALOG_ERROR_NOT_INITIALIZED = static_cast<int>(0x80B80003u);
constexpr int COMMON_DIALOG_ERROR_NOT_FINISHED = static_cast<int>(0x80B80005u);
constexpr int COMMON_DIALOG_ERROR_BUSY = static_cast<int>(0x80B80007u);
constexpr int COMMON_DIALOG_ERROR_ARG_NULL = static_cast<int>(0x80B8000Du);

std::atomic<int> g_status{COMMON_DIALOG_STATUS_NONE};

}

extern "C" {

int APS5_VABI sceNetCtlApDialogInitialize(void) {
    int expected = COMMON_DIALOG_STATUS_NONE;
    if (!g_status.compare_exchange_strong(expected, 1)) throw std::logic_error(std::string(__func__) + ": already initialized");
    return 0;
}

int APS5_VABI sceNetCtlApDialogTerminate(void) {
    int expected = 1;
    if (!g_status.compare_exchange_strong(expected, COMMON_DIALOG_STATUS_NONE)) throw std::logic_error(std::string(__func__) + ": not initialized or still running");
    return 0;
}

int APS5_VABI sceNetCtlApDialogOpen(void) {
    const int status = g_status.load();
    if (status == COMMON_DIALOG_STATUS_NONE) return COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    if (status == 2) return COMMON_DIALOG_ERROR_BUSY;
    g_status = COMMON_DIALOG_STATUS_FINISHED;
    return 0;
}

int APS5_VABI sceNetCtlApDialogClose(void) {
    if (g_status.load() == COMMON_DIALOG_STATUS_NONE) return COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    return 0;
}

int APS5_VABI sceNetCtlApDialogUpdateStatus(void) {
    return g_status.load();
}

int APS5_VABI sceNetCtlApDialogGetResult(void* result) {
    const int status = g_status.load();
    if (status == COMMON_DIALOG_STATUS_NONE) return COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    if (result == nullptr) return COMMON_DIALOG_ERROR_ARG_NULL;
    if (status != COMMON_DIALOG_STATUS_FINISHED) return COMMON_DIALOG_ERROR_NOT_FINISHED;
    *static_cast<std::int32_t*>(result) = COMMON_DIALOG_RESULT_USER_CANCELED;
    return 0;
}

}
