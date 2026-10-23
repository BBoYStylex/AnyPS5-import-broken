#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>

struct DiscId;
struct DiscId2;

extern "C" {
int APS5_VABI sceDiscIdGet(DiscId* discId);
int APS5_VABI sceDiscIdGet2(DiscId2* discId, std::uint32_t flags);
}

namespace {

void Require(bool condition) {
    if (!condition) std::abort();
}

constexpr std::int32_t kErrNoDisc = static_cast<std::int32_t>(0x80D50002);

}

int main() {
    Require(sceDiscIdGet(nullptr) == kErrNoDisc);
    Require(sceDiscIdGet2(nullptr, 0) == kErrNoDisc);
    Require(sceDiscIdGet2(nullptr, 0xFFFFFFFFu) == kErrNoDisc);

    std::uint8_t buffer[64];
    std::memset(buffer, 0xAB, sizeof(buffer));
    auto* id = reinterpret_cast<DiscId*>(buffer);
    auto* id2 = reinterpret_cast<DiscId2*>(buffer);

    Require(sceDiscIdGet(id) == kErrNoDisc);
    Require(sceDiscIdGet2(id2, 0) == kErrNoDisc);
    Require(sceDiscIdGet2(id2, 1) == kErrNoDisc);
    Require(sceDiscIdGet2(id2, 3) == kErrNoDisc);
    Require(sceDiscIdGet2(id2, 0xFFFFFFFFu) == kErrNoDisc);
    for (std::uint8_t byte : buffer) Require(byte == 0xAB);

    return 0;
}
