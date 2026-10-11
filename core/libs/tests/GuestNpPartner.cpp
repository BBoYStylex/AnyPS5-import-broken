#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdlib>

extern "C" {
int APS5_VABI sceNpEAAccessInitialize();
int APS5_VABI sceNpEAAccessTerminate();
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

constexpr int kNotInitialized = static_cast<int>(0x819d0001);

}

int main() {
    Require(sceNpEAAccessTerminate() == kNotInitialized);
    Require(sceNpEAAccessInitialize() == 0);
    Require(sceNpEAAccessInitialize() == 0);
    Require(sceNpEAAccessTerminate() == 0);
    Require(sceNpEAAccessTerminate() == kNotInitialized);
    return 0;
}
