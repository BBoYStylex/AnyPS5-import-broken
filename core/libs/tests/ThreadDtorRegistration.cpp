#include "prx/libkernel/Pthread/include/ThreadLifecycle.hpp"

#include <atomic>
#include <cstdlib>
#include <stdexcept>

namespace {

void APS5_VABI FirstCallback() {}
void APS5_VABI OtherCallback() {}

}

int main() {
    std::atomic<thread_dtors_func_t> registered{nullptr};
    ThreadLifecycleDetail::RegisterThreadDtors(registered, FirstCallback);
    ThreadLifecycleDetail::RegisterThreadDtors(registered, FirstCallback);

    try {
        ThreadLifecycleDetail::RegisterThreadDtors(registered, OtherCallback);
    } catch (const std::runtime_error&) {
        return 0;
    }
    return EXIT_FAILURE;
}
