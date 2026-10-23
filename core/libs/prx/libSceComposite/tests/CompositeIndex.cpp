#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <thread>

extern "C" int APS5_VABI sceCompositorAllocateIndex();
extern "C" void APS5_VABI sceCompositorReleaseIndex(std::uint32_t id);

namespace {
void Require(bool value) {
    if (!value) std::abort();
}
}

int main() {
    for (int expected = 0; expected < 16; ++expected) {
        Require(sceCompositorAllocateIndex() == expected);
    }
    Require(sceCompositorAllocateIndex() == -1);
    sceCompositorReleaseIndex(16);
    sceCompositorReleaseIndex(UINT32_MAX);
    Require(sceCompositorAllocateIndex() == -1);
    sceCompositorReleaseIndex(7);
    Require(sceCompositorAllocateIndex() == 7);
    sceCompositorReleaseIndex(7);
    sceCompositorReleaseIndex(7);
    Require(sceCompositorAllocateIndex() == 7);
    Require(sceCompositorAllocateIndex() == -1);
    for (std::uint32_t index = 0; index < 16; ++index) sceCompositorReleaseIndex(index);

    std::array<int, 16> indices{};
    std::array<std::thread, 16> threads;
    for (std::size_t index = 0; index < threads.size(); ++index) {
        threads[index] = std::thread([&, index] { indices[index] = sceCompositorAllocateIndex(); });
    }
    for (auto& thread : threads) thread.join();
    std::uint32_t seen = 0;
    for (const int index : indices) {
        Require(index >= 0 && index < 16);
        const auto mask = std::uint32_t{1} << index;
        Require((seen & mask) == 0);
        seen |= mask;
        sceCompositorReleaseIndex(static_cast<std::uint32_t>(index));
    }
    Require(seen == 0xffffu);
}
