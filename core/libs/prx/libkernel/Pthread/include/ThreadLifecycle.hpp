#ifndef CORE_LIBS_PRX_LIBKERNEL_PTHREAD_THREADLIFECYCLE_HPP
#define CORE_LIBS_PRX_LIBKERNEL_PTHREAD_THREADLIFECYCLE_HPP

#include "SceTypes.hpp"
#include <atomic>
#include <stdexcept>

namespace ThreadLifecycleDetail {
template <typename Callback>
void RegisterThreadDtors(std::atomic<Callback>& registered, Callback callback) {
    if (!callback)
        throw std::runtime_error("Thread destructor callback is null");
    Callback expected = nullptr;
    if (!registered.compare_exchange_strong(expected, callback) && expected != callback)
        throw std::runtime_error("Thread destructor callback is already registered");
}
}

class ThreadLifecycle {
public:
    static void SetThreadDtors(thread_dtors_func_t callback);
    static void SetThreadAtexitCount(get_thread_atexit_count_func_t callback);
    static void SetThreadAtexitReport(thread_atexit_report_func_t callback);
    static void RunKeyDtors();
};

extern "C" void APS5_VABI scePthreadExit(void* retval);

#endif
