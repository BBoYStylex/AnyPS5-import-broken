#include "SceTypes.hpp"
#include <cstdint>
#include <cstdlib>

extern "C" {
int APS5_VABI sceVideoOutUnknown_YNw9Xk5iNWo(int handle, void* status);
}

static void Require(bool value) { if (!value) std::abort(); }

int main() {
    std::uint32_t status = 0;
    Require(sceVideoOutUnknown_YNw9Xk5iNWo(0, nullptr) == 0);
    Require(sceVideoOutUnknown_YNw9Xk5iNWo(1, &status) == 0);
    Require(sceVideoOutUnknown_YNw9Xk5iNWo(67072176, reinterpret_cast<void*>(0x1)) == 0);
    Require(sceVideoOutUnknown_YNw9Xk5iNWo(-1, &status) == 0);
}
