#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdlib>

extern "C" {
int APS5_VABI sceAcm_FFT();
int APS5_VABI sceAcm_Panner();
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

}

int main() {
    Require(sceAcm_FFT() == 0);
    Require(sceAcm_Panner() == 0);
}
