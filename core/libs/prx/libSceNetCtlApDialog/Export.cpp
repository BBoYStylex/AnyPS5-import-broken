#include <atomic>
#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr int STATUS_NONE = 0;
constexpr int STATUS_INITIALIZED = 1;
constexpr int STATUS_RUNNING = 2;
constexpr int STATUS_FINISHED = 3;

constexpr int ERROR_NOT_INITIALIZED = static_cast<int>(0x80412101u);
constexpr int ERROR_BUSY = static_cast<int>(0x8041210Cu);
constexpr int ERROR_INVALID_STATE = static_cast<int>(0x8041210Eu);
constexpr int ERROR_USER_CANCELED = static_cast<int>(0x80412110u);

std::atomic<int> g_status{STATUS_NONE};

}

extern "C" {

int APS5_VABI sceNetCtlApDialogInitialize(void) {
 int expected = STATUS_NONE;
 if (!g_status.compare_exchange_strong(expected, STATUS_INITIALIZED)) return ERROR_INVALID_STATE;
 return 0;
}

int APS5_VABI sceNetCtlApDialogOpen(void) {
 const int status = g_status.load();
 if (status == STATUS_NONE) return ERROR_NOT_INITIALIZED;
 if (status == STATUS_RUNNING) return ERROR_BUSY;
 g_status = STATUS_FINISHED;
 return 0;
}

int APS5_VABI sceNetCtlApDialogGetStatus(void) {
 return g_status.load();
}

int APS5_VABI sceNetCtlApDialogUpdateStatus(void) {
 if (g_status.load() == STATUS_NONE) return ERROR_NOT_INITIALIZED;
 return g_status.load();
}

int APS5_VABI sceNetCtlApDialogGetResult(void) {
 return ERROR_USER_CANCELED;
}

int APS5_VABI sceNetCtlApDialogClose(void) {
 if (g_status.load() == STATUS_NONE) return ERROR_NOT_INITIALIZED;
 return 0;
}

int APS5_VABI sceNetCtlApDialogTerminate(void) {
 if (g_status.exchange(STATUS_NONE) == STATUS_NONE) return ERROR_NOT_INITIALIZED;
 return 0;
}

}
