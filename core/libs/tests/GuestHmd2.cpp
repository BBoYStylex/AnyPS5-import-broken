#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" {
std::int32_t APS5_VABI sceHmd2Initialize(const void* param);
int APS5_VABI sceHmd2Open();
int APS5_VABI sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign();
int APS5_VABI sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign();
int APS5_VABI sceHmd2ReprojectionQueryBufferSizeAlign();
int APS5_VABI sceHmd2ReprojectionQueryDisplayBufferSizeAlign();
}

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "HMD2: %s\n", message);
        std::abort();
    }
}

}

int main() {
    const std::uint8_t param[16]{};
    Require(sceHmd2Initialize(param) == static_cast<std::int32_t>(0x81110016), "initialization did not report the unsupported feature");
    Require(sceHmd2Initialize(nullptr) == static_cast<std::int32_t>(0x81110016), "initialization without a param did not report the unsupported feature");
    Require(sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign() == 0, "mirroring display buffer size query did not return 0");
    Require(sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign() == 0, "mirroring work memory size query did not return 0");
    Require(sceHmd2ReprojectionQueryBufferSizeAlign() == 0, "reprojection buffer size query did not return 0");
    Require(sceHmd2ReprojectionQueryDisplayBufferSizeAlign() == 0, "reprojection display buffer size query did not return 0");
    bool threw = false;
    try {
        sceHmd2Open();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "open after failed initialization did not throw");
    std::puts("HMD2 tests passed");
    return 0;
}
