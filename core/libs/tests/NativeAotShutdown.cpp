#include "prx/libc/include/NativeAotBundle.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include <cstdio>
#include <cstdlib>

extern "C" int __cxa_atexit_nid_postfix(void (*)(void*), void*, void*);
extern "C" [[noreturn]] void LibcExitWithoutHandlers_nid_no_patch(int);

namespace {
NativeAotBundleRegistry* bundle;
unsigned finalized = 0;

void Finalize() {
    if (++finalized != 1) std::_Exit(71);
}

void Stop() {
    bundle->Stop();
    bundle->Stop();
    if (finalized != 1) std::_Exit(72);
    std::fputs("native bundle shutdown completed\n", stderr);
}

void GuestExit(void*) {
    std::_Exit(73);
}
}

int main() {
    const void* functions[] = {reinterpret_cast<const void*>(Finalize)};
    const NativeAotLifecycleFunction finalizers[] = {Finalize};
    NativeAotModule module{};
    module.descriptorVersion = 3;
    module.id = "shutdown-test";
    module.moduleVersion = 1;
    module.functions = functions;
    module.functionCount = 1;
    module.finalizers = finalizers;
    module.finalizerCount = 1;
    NativeAotBundleRegistry registry(std::span(&module, 1), 0);
    bundle = &registry;
    registry.Start();
    if (__cxa_atexit_nid_postfix(GuestExit, nullptr, nullptr) != 0) return 74;
    LibcRegisterShutdown_nid_postfix(Stop);
    LibcExitWithoutHandlers_nid_no_patch(0);
}
