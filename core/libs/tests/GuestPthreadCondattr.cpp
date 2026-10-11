#include "SceTypes.hpp"
#include <cstdlib>
#include <stdexcept>

extern "C" {
int APS5_VABI pthread_condattr_init_nid_postfix(PthreadCondattr* attr);
int APS5_VABI pthread_condattr_destroy_nid_postfix(PthreadCondattr* attr);
int APS5_VABI pthread_condattr_setclock_nid_postfix(PthreadCondattr* attr, KernelClockid clock_id);
int APS5_VABI pthread_condattr_getclock_nid_postfix(const PthreadCondattr* attr, KernelClockid* clock_id);
int APS5_VABI pthread_condattr_setpshared_nid_postfix(PthreadCondattr* attr, int pshared);
int APS5_VABI pthread_condattr_getpshared_nid_postfix(const PthreadCondattr* attr, int* pshared);

int APS5_VABI scePthreadCondattrInit(PthreadCondattr* attr);
int APS5_VABI scePthreadCondattrDestroy(PthreadCondattr* attr);
int APS5_VABI scePthreadCondattrSetclock(PthreadCondattr* attr, KernelClockid clockId);
int APS5_VABI scePthreadCondattrGetclock(const PthreadCondattr* attr, KernelClockid* clockId);
int APS5_VABI scePthreadCondattrSetpshared(PthreadCondattr* attr, int pshared);
int APS5_VABI scePthreadCondattrGetpshared(const PthreadCondattr* attr, int* pshared);
}

namespace {

constexpr int GUEST_EINVAL = 22;
constexpr int PROCESS_PRIVATE = 0;
constexpr int PROCESS_SHARED = 1;
constexpr int SCE_OK = 0;
constexpr int SCE_KERNEL_ERROR_EINVAL = static_cast<int>(0x80020016u);

void Require(bool value) {
    if (!value) std::abort();
}

template <typename Call>
bool Throws(Call call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

void CheckPosixClock() {
    PthreadCondattr attr = nullptr;
    Require(pthread_condattr_init_nid_postfix(&attr) == 0);
    KernelClockid clockId = -1;
    Require(pthread_condattr_getclock_nid_postfix(&attr, &clockId) == 0 && clockId == 0);
    for (const KernelClockid set : {1, 2, 0, 4}) {
        Require(pthread_condattr_setclock_nid_postfix(&attr, set) == 0);
        Require(pthread_condattr_getclock_nid_postfix(&attr, &clockId) == 0 && clockId == set);
    }
    Require(Throws([&] { pthread_condattr_getclock_nid_postfix(&attr, nullptr); }));
    Require(pthread_condattr_destroy_nid_postfix(&attr) == 0);
    clockId = -1;
    Require(pthread_condattr_getclock_nid_postfix(&attr, &clockId) == GUEST_EINVAL && clockId == -1);
    Require(pthread_condattr_getclock_nid_postfix(nullptr, &clockId) == GUEST_EINVAL && clockId == -1);
    Require(pthread_condattr_setclock_nid_postfix(&attr, 0) == GUEST_EINVAL);
    Require(pthread_condattr_setclock_nid_postfix(nullptr, 0) == GUEST_EINVAL);
}

void CheckPosixPshared() {
    PthreadCondattr attr = nullptr;
    Require(pthread_condattr_init_nid_postfix(&attr) == 0);
    int pshared = -1;
    Require(pthread_condattr_getpshared_nid_postfix(&attr, &pshared) == 0 && pshared == PROCESS_PRIVATE);
    Require(pthread_condattr_setpshared_nid_postfix(&attr, PROCESS_SHARED) == 0);
    Require(pthread_condattr_getpshared_nid_postfix(&attr, &pshared) == 0 && pshared == PROCESS_SHARED);
    Require(pthread_condattr_setpshared_nid_postfix(&attr, PROCESS_PRIVATE) == 0);
    Require(pthread_condattr_getpshared_nid_postfix(&attr, &pshared) == 0 && pshared == PROCESS_PRIVATE);

    Require(pthread_condattr_setpshared_nid_postfix(&attr, 2) == GUEST_EINVAL);
    Require(pthread_condattr_setpshared_nid_postfix(&attr, -1) == GUEST_EINVAL);
    Require(pthread_condattr_getpshared_nid_postfix(&attr, &pshared) == 0 && pshared == PROCESS_PRIVATE);

    Require(Throws([&] { pthread_condattr_getpshared_nid_postfix(&attr, nullptr); }));
    Require(pthread_condattr_destroy_nid_postfix(&attr) == 0);
    pshared = -1;
    Require(pthread_condattr_getpshared_nid_postfix(&attr, &pshared) == GUEST_EINVAL && pshared == -1);
    Require(pthread_condattr_getpshared_nid_postfix(nullptr, &pshared) == GUEST_EINVAL && pshared == -1);
    Require(pthread_condattr_setpshared_nid_postfix(&attr, PROCESS_PRIVATE) == GUEST_EINVAL);
    Require(pthread_condattr_setpshared_nid_postfix(nullptr, PROCESS_PRIVATE) == GUEST_EINVAL);
}

void CheckSceCondattr() {
    PthreadCondattr attr = nullptr;
    Require(Throws([&] { scePthreadCondattrInit(nullptr); }));
    Require(scePthreadCondattrInit(&attr) == SCE_OK);

    KernelClockid clockId = -1;
    Require(scePthreadCondattrGetclock(&attr, &clockId) == SCE_OK && clockId == 0);
    Require(scePthreadCondattrSetclock(&attr, 1) == SCE_OK);
    Require(scePthreadCondattrGetclock(&attr, &clockId) == SCE_OK && clockId == 1);
    Require(Throws([&] { scePthreadCondattrGetclock(&attr, nullptr); }));

    int pshared = -1;
    Require(scePthreadCondattrGetpshared(&attr, &pshared) == SCE_OK && pshared == PROCESS_PRIVATE);
    Require(scePthreadCondattrSetpshared(&attr, PROCESS_SHARED) == SCE_OK);
    Require(scePthreadCondattrGetpshared(&attr, &pshared) == SCE_OK && pshared == PROCESS_SHARED);
    Require(scePthreadCondattrSetpshared(&attr, 2) == SCE_KERNEL_ERROR_EINVAL);
    Require(scePthreadCondattrSetpshared(&attr, -1) == SCE_KERNEL_ERROR_EINVAL);
    Require(scePthreadCondattrGetpshared(&attr, &pshared) == SCE_OK && pshared == PROCESS_SHARED);
    Require(Throws([&] { scePthreadCondattrGetpshared(&attr, nullptr); }));

    Require(scePthreadCondattrDestroy(&attr) == SCE_OK);
    Require(attr == nullptr);
    Require(Throws([&] { scePthreadCondattrDestroy(&attr); }));
    Require(Throws([&] { scePthreadCondattrDestroy(nullptr); }));
    Require(Throws([&] { scePthreadCondattrGetclock(&attr, &clockId); }));
    Require(Throws([&] { scePthreadCondattrSetclock(&attr, 0); }));
    Require(Throws([&] { scePthreadCondattrGetpshared(&attr, &pshared); }));
    Require(Throws([&] { scePthreadCondattrSetpshared(&attr, 0); }));
}

}

int main() {
    CheckPosixClock();
    CheckPosixPshared();
    CheckSceCondattr();
    return 0;
}
