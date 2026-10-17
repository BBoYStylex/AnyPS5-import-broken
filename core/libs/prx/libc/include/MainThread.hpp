#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_MAINTHREAD_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_MAINTHREAD_HPP

#ifdef __APPLE__

#include <dispatch/dispatch.h>
#include <exception>
#include <pthread.h>
#include <type_traits>

// True when the process main thread serves the main dispatch queue: relinked games, where libc's Aps5StartGuest
// (src/specifics/macos/GuestStart.cpp) runs the guest on a second thread. Native programs such as the tests keep
// their own main thread, so work then runs on the calling thread as before.
extern "C" bool LibcMainQueueServed_nid_postfix();

// AppKit only allows windows and their events on the main thread. Runs work there, waits for it, and rethrows its
// exception on the calling thread.
template <class Work>
void RunOnMainThread(Work&& work) {
    if (pthread_main_np() != 0 || !LibcMainQueueServed_nid_postfix()) {
        work();
        return;
    }
    using Callable = std::remove_reference_t<Work>;
    struct Context {
        Callable* Call;
        std::exception_ptr Failure;
    } context{&work, nullptr};
    dispatch_sync_f(dispatch_get_main_queue(), &context, [](void* argument) {
        auto* call = static_cast<Context*>(argument);
        try {
            (*call->Call)();
        } catch (...) {
            call->Failure = std::current_exception();
        }
    });
    if (context.Failure) std::rethrow_exception(context.Failure);
}

#endif

#endif
