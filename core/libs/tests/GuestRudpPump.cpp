#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

extern "C" {
int APS5_VABI sceRudpInit_nid_postfix(void*, int);
int APS5_VABI sceRudpTerminate();
int APS5_VABI sceRudpProcessEvents();
int APS5_VABI sceRudpNetFlush();
int APS5_VABI sceRudpGetMaxSegmentSize(std::uint16_t*);
int APS5_VABI sceRudpNetReceived();
}

static void Require(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "rudp check failed at line %d\n", line);
        std::abort();
    }
}
#define CHECK(value) Require((value), __LINE__)

int main() {
    constexpr int notInitialized = static_cast<int>(0x80770001);
    constexpr int invalidArgument = static_cast<int>(0x80770004);
    std::uint16_t segmentSize = 0x5a5a;
    CHECK(sceRudpProcessEvents() == notInitialized);
    CHECK(sceRudpNetFlush() == notInitialized);
    CHECK(sceRudpGetMaxSegmentSize(&segmentSize) == notInitialized);
    CHECK(sceRudpGetMaxSegmentSize(nullptr) == invalidArgument);
    CHECK(segmentSize == 0x5a5a);

    alignas(8) static unsigned char pool[0xC58];
    CHECK(sceRudpInit_nid_postfix(pool, sizeof(pool)) == 0);
    CHECK(sceRudpProcessEvents() == 0);
    CHECK(sceRudpNetFlush() == 0);
    CHECK(sceRudpGetMaxSegmentSize(&segmentSize) == 0);
    CHECK(sceRudpGetMaxSegmentSize(nullptr) == invalidArgument);
    CHECK(segmentSize == 0x5a5a);
    CHECK(sceRudpTerminate() == 0);

    try {
        sceRudpNetReceived();
    } catch (const std::runtime_error& error) {
        CHECK(std::string(error.what()) == "sceRudpNetReceived not implemented");
        return 0;
    }
    std::fprintf(stderr, "sceRudpNetReceived returned instead of reporting that it is not implemented\n");
    return 1;
}
