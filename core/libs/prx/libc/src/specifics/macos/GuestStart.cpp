#ifdef __APPLE__

// Process entry for relinked Mach-O executables. The macOS relinker's entry stub (EntryStub in
// core/relinker/elfpatcher/src/macos/MacOsImage.cpp) builds the FreeBSD initial block [argc][argv...][0][envp...][0]
// on the stack and calls Aps5StartGuest with rdi = guest ELF entry and rsi = that block.
//
// AppKit only allows windows and their events on the process main thread, so the guest runs on a second thread and
// the main thread serves the main dispatch queue; libSceVideoOut sends its window work there (MainThread.hpp).
// Aps5EnterGuest enters the guest like the Linux entry stub (core/relinker/elfpatcher/src/general/EntryStubBuilder.cpp):
// rdi = block, rsi = 0 (no cleanup function), a 16-byte aligned stack and a zero frame link; if the guest entry
// returns, the process exits with its result.

#include "prx/libc/include/general/AsmMacros.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <dispatch/dispatch.h>
#include <pthread.h>

extern "C" [[noreturn]] void Aps5EnterGuest_nid_no_patch(void* entry, void* block);

asm(".text\n"
    ".p2align 4\n" APS5_ASM_FUNCTION("Aps5EnterGuest_nid_no_patch") "    movq %rdi, %rax\n"
                                                                    "    movq %rsi, %rdi\n"
                                                                    "    andq $-16, %rsp\n"
                                                                    "    pushq $0\n"
                                                                    "    pushq $0\n"
                                                                    "    movq %rsp, %rbp\n"
                                                                    "    xorl %esi, %esi\n"
                                                                    "    call *%rax\n"
                                                                    "    movl %eax, %edi\n"
                                                                    "    call " APS5_ASM_SYMBOL("exit") "\n"
                                                                                                   "    ud2\n"
    APS5_ASM_FUNCTION_END("Aps5EnterGuest_nid_no_patch"));

namespace {

constexpr std::size_t GuestMainStackSize = std::size_t{64} << 20;

std::atomic<bool> mainQueueServed{false};

struct GuestStart {
    void* Entry;
    void* Block;
};

void* RunGuestMain(void* argument) {
    const auto start = *static_cast<GuestStart*>(argument);
    pthread_setname_np("main");
    Aps5EnterGuest_nid_no_patch(start.Entry, start.Block);
}

[[noreturn]] void Fatal(const char* message, int error) {
    std::fprintf(stderr, "guest start: %s (%d)\n", message, error);
    std::abort();
}

}

extern "C" bool LibcMainQueueServed_nid_postfix() {
    return mainQueueServed.load(std::memory_order_acquire);
}

// The initial block lives on this thread's stack, which stays valid because this thread never returns.
extern "C" [[noreturn]] void Aps5StartGuest_nid_no_patch(void* entry, void* block) {
    static GuestStart start;
    start = {entry, block};
    pthread_attr_t attributes;
    int result = pthread_attr_init(&attributes);
    if (result != 0) Fatal("pthread_attr_init failed", result);
    result = pthread_attr_setstacksize(&attributes, GuestMainStackSize);
    if (result != 0) Fatal("pthread_attr_setstacksize failed", result);
    result = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
    if (result != 0) Fatal("pthread_attr_setdetachstate failed", result);
    mainQueueServed.store(true, std::memory_order_release);
    pthread_t thread;
    result = pthread_create(&thread, &attributes, RunGuestMain, &start);
    if (result != 0) Fatal("pthread_create failed", result);
    pthread_attr_destroy(&attributes);
    // Not dispatch_main(): it parks this thread and drains the main queue on a worker thread, which AppKit rejects.
    // The main run loop drains the main queue on this thread itself.
    for (;;) CFRunLoopRun();
}

#endif
