#include "prx/libc/include/general/VabiMacros.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

extern "C" {
int APS5_VABI sceOpusCeltDecInitialize(std::uint32_t*);
int APS5_VABI sceOpusCeltDecTerminate(std::uint32_t*);
int APS5_VABI sceOpusCeltDecGetSize(int);
int APS5_VABI sceOpusCeltDecCreateEx(std::uint32_t*, void*, int, int);
int APS5_VABI sceOpusCeltDecDecode(void*, const std::uint8_t*, int, std::int16_t*, int);
int APS5_VABI sceOpusCeltDecDestroy(void*);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

template <typename TFunction>
void RequireThrows(TFunction function) {
    try { function(); } catch (const std::runtime_error&) { return; }
    std::abort();
}

} // namespace

int main() {
    std::uint32_t context = 0, otherContext = 0;
    Require(sceOpusCeltDecInitialize(&context) == 0);
    Require(sceOpusCeltDecInitialize(&otherContext) == 0 && context != otherContext);
    RequireThrows([&] { sceOpusCeltDecInitialize(&context); });
    Require(sceOpusCeltDecGetSize(1) == 640 && sceOpusCeltDecGetSize(2) == 640);
    RequireThrows([&] { sceOpusCeltDecGetSize(0); });
    RequireThrows([&] { sceOpusCeltDecGetSize(3); });
    std::array<std::uint8_t, 640> state{}, otherState{}, monoState{};
    Require(sceOpusCeltDecCreateEx(&context, state.data(), 48000, 2) == 0);
    Require(sceOpusCeltDecCreateEx(&otherContext, otherState.data(), 48000, 2) == 0);
    Require(sceOpusCeltDecCreateEx(&context, monoState.data(), 48000, 1) == 0);
    RequireThrows([&] { sceOpusCeltDecTerminate(&context); });
    RequireThrows([&] { sceOpusCeltDecCreateEx(&context, state.data(), 48000, 2); });
    std::array<std::uint8_t, 640> unsupported{};
    for (int rate : {8000, 12000, 16000, 24000})
        RequireThrows([&] { sceOpusCeltDecCreateEx(&context, unsupported.data(), rate, 2); });
    RequireThrows([&] { sceOpusCeltDecCreateEx(&context, nullptr, 48000, 2); });
    RequireThrows([&] { sceOpusCeltDecCreateEx(nullptr, unsupported.data(), 48000, 2); });
    std::array<std::int16_t, 1922> pcm{};
    pcm.fill(12345);
    const std::uint8_t truncated[] = {0x7f};
    RequireThrows([&] { sceOpusCeltDecDecode(state.data(), truncated, 1, pcm.data() + 1, 3840); });
    RequireThrows([&] { sceOpusCeltDecDecode(state.data(), nullptr, 0, pcm.data() + 1, 3840); });
    RequireThrows([&] { sceOpusCeltDecDecode(nullptr, truncated, 1, pcm.data() + 1, 3840); });
    Require(std::all_of(pcm.begin(), pcm.end(), [](auto value) { return value == 12345; }));
    std::array<std::uint8_t, 640> unknown{};
    RequireThrows([&] { sceOpusCeltDecDecode(unknown.data(), truncated, 1, pcm.data(), 3840); });
    RequireThrows([&] { sceOpusCeltDecDestroy(unknown.data()); });
    const std::uint8_t invalidFraming[] = {0x7d, 1};
    std::array<std::int16_t, 3842> rejected;
    rejected.fill(12345);
    RequireThrows([&] { sceOpusCeltDecDecode(state.data(), invalidFraming, 2, rejected.data() + 1, 7680); });
    Require(std::all_of(rejected.begin(), rejected.end(), [](auto value) { return value == 12345; }));
    Require(sceOpusCeltDecDestroy(state.data()) == 0);
    RequireThrows([&] { sceOpusCeltDecDestroy(state.data()); });
    Require(sceOpusCeltDecCreateEx(&context, state.data(), 48000, 2) == 0);
    Require(sceOpusCeltDecDestroy(state.data()) == 0);
    Require(sceOpusCeltDecDestroy(monoState.data()) == 0);
    Require(sceOpusCeltDecTerminate(&context) == 0);
    RequireThrows([&] { sceOpusCeltDecTerminate(&context); });
    RequireThrows([&] { sceOpusCeltDecCreateEx(&context, state.data(), 48000, 2); });
    Require(sceOpusCeltDecDestroy(otherState.data()) == 0);
    Require(sceOpusCeltDecTerminate(&otherContext) == 0);
}
