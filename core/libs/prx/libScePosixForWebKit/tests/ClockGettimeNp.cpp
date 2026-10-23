#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdio>
#include <cstdlib>
#include <source_location>

extern "C" {
int APS5_VABI clock_gettime_nid_postfix(int, KernelTimespec*);
int APS5_VABI clock_gettime_np(int, KernelTimespec*);
int* APS5_VABI __error_nid_postfix();
}

static void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (!condition) {
        std::fprintf(stderr, "clock_gettime_np check failed at line %u (guest errno %d)\n", location.line(), *__error_nid_postfix());
        std::abort();
    }
}

static bool NotAfter(const KernelTimespec& earlier, const KernelTimespec& later) {
    return earlier.tv_sec < later.tv_sec || (earlier.tv_sec == later.tv_sec && earlier.tv_nsec <= later.tv_nsec);
}

int main() {
    KernelTimespec before{};
    KernelTimespec during{};
    KernelTimespec after{};
    Require(clock_gettime_nid_postfix(4, &before) == 0);
    Require(clock_gettime_np(4, &during) == 0);
    Require(clock_gettime_nid_postfix(4, &after) == 0);
    Require(NotAfter(before, during));
    Require(NotAfter(during, after));
    Require(during.tv_nsec >= 0 && during.tv_nsec < 1000000000);

    *__error_nid_postfix() = 0;
    Require(clock_gettime_np(4, nullptr) == -1);
    Require(*__error_nid_postfix() == 14);

    *__error_nid_postfix() = 0;
    Require(clock_gettime_np(-1, &during) == -1);
    Require(*__error_nid_postfix() == 22);
}
