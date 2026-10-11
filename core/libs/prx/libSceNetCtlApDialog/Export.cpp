#include <cstdint>
#include <cstddef>
#include <mutex>

#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {
constexpr int kStatusNone = 0;
constexpr int kStatusInitialized = 1;
constexpr int kStatusRunning = 2;
constexpr int kStatusFinished = 3;

constexpr int kErrOk = 0;
constexpr int kErrNotInitialized = static_cast<int>(0x80ED0001u);
constexpr int kErrAlreadyInitialized = static_cast<int>(0x80ED0002u);
constexpr int kErrInvalidState = static_cast<int>(0x80ED0005u);

std::mutex g_lock;
int g_status = kStatusNone;
}  // namespace

extern "C" {

int APS5_VABI sceNetCtlApDialogInitialize(void) {
    std::lock_guard g(g_lock);
    if (g_status != kStatusNone) return kErrAlreadyInitialized;
    g_status = kStatusInitialized;
    return kErrOk;
}

int APS5_VABI sceNetCtlApDialogTerminate(void) {
    std::lock_guard g(g_lock);
    if (g_status == kStatusNone) return kErrNotInitialized;
    g_status = kStatusNone;
    return kErrOk;
}

int APS5_VABI sceNetCtlApDialogOpen(void) {
    std::lock_guard g(g_lock);
    if (g_status != kStatusInitialized && g_status != kStatusFinished) return kErrInvalidState;
    g_status = kStatusRunning;
    return kErrOk;
}

int APS5_VABI sceNetCtlApDialogClose(void) {
    std::lock_guard g(g_lock);
    if (g_status != kStatusRunning) return kErrInvalidState;
    g_status = kStatusFinished;
    return kErrOk;
}

int APS5_VABI sceNetCtlApDialogUpdateStatus(void) {
    std::lock_guard g(g_lock);
    if (g_status == kStatusRunning) g_status = kStatusFinished;
    return g_status;
}

int APS5_VABI sceNetCtlApDialogGetResult(void) {
    std::lock_guard g(g_lock);
    if (g_status != kStatusFinished) return kErrInvalidState;
    return kErrOk;
}

}
