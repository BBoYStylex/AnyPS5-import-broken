#include "prx/libc/include/general/VabiMacros.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" {
int APS5_VABI sceVrTrackerGetTime(std::uint64_t* time);
int APS5_VABI sceVrTrackerGpuSubmit(const void* param);
int APS5_VABI sceVrTrackerGpuWait(const void* param);
int APS5_VABI sceVrTrackerInit();
}

namespace {

constexpr int NotInit = static_cast<int>(0x81260801u);
constexpr std::uint8_t Fill = 0xAA;
constexpr std::uint64_t FilledTime = 0xAAAAAAAAAAAAAAAAull;

using Buffer = std::array<std::uint8_t, 0x200>;

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "VrTracker: %s\n", message);
        std::abort();
    }
}

bool Untouched(const Buffer& buffer) {
    for (std::uint8_t byte : buffer)
        if (byte != Fill) return false;
    return true;
}

template <typename TCall>
bool Throws(TCall call) {
    try {
        call();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

}

int main() {
    Require(sceVrTrackerGetTime(nullptr) == NotInit, "time null");
    std::uint64_t time = FilledTime;
    Require(sceVrTrackerGetTime(&time) == NotInit, "time return");
    Require(time == FilledTime, "time untouched");

    Buffer param{};
    param.fill(Fill);
    Require(sceVrTrackerGpuSubmit(nullptr) == NotInit, "submit null");
    Require(sceVrTrackerGpuSubmit(param.data()) == NotInit, "submit return");
    Require(Untouched(param), "submit leaves param");
    Require(sceVrTrackerGpuWait(nullptr) == NotInit, "wait null");
    Require(sceVrTrackerGpuWait(param.data()) == NotInit, "wait return");
    Require(Untouched(param), "wait leaves param");

    Require(Throws([] { sceVrTrackerInit(); }), "init throws");
    return 0;
}
