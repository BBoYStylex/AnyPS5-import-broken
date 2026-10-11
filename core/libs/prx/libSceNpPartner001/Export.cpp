#include "prx/libc/include/General.hpp"
#include <atomic>

namespace {

constexpr int SCE_NP_PARTNER_ERROR_NOT_INITIALIZED = static_cast<int>(0x819d0001);

std::atomic<bool> g_initialized{false};

}

extern "C" {

int APS5_VABI sceNpEAAccessInitialize(void) {
    g_initialized.store(true, std::memory_order_relaxed);
    return 0;
}

int APS5_VABI sceNpEAAccessTerminate(void) {
    if (!g_initialized.exchange(false, std::memory_order_relaxed)) return SCE_NP_PARTNER_ERROR_NOT_INITIALIZED;
    return 0;
}

}
