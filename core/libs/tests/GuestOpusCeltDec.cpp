#include "prx/libc/include/general/VabiMacros.hpp"
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
    std::uint32_t context = 0;
    Require(sceOpusCeltDecInitialize(&context) == 0);
    RequireThrows([&] { sceOpusCeltDecInitialize(&context); });
    Require(sceOpusCeltDecGetSize(1) == 640 && sceOpusCeltDecGetSize(2) == 640);
    RequireThrows([] { sceOpusCeltDecGetSize(3); });
    std::array<std::uint8_t, 640> state{};
    Require(sceOpusCeltDecCreateEx(&context, state.data(), 48000, 1) == 0);
    RequireThrows([&] { sceOpusCeltDecTerminate(&context); });
    RequireThrows([&] { sceOpusCeltDecCreateEx(&context, state.data(), 48000, 1); });
    std::array<std::int16_t, 960> pcm{};
    const std::uint8_t celtSilence[] = {0xf8, 0xff, 0xfe};
    const std::uint8_t nonCelt[] = {0x00, 0x00};
    RequireThrows([&] { sceOpusCeltDecDecode(state.data(), nonCelt, sizeof(nonCelt), pcm.data(), sizeof(pcm)); });
    RequireThrows([&] { sceOpusCeltDecDecode(state.data(), celtSilence, sizeof(celtSilence), pcm.data(), sizeof(pcm) - 1); });
    Require(sceOpusCeltDecDecode(state.data(), celtSilence, sizeof(celtSilence), pcm.data(), sizeof(pcm)) == 1920);
    Require(sceOpusCeltDecDestroy(state.data()) == 0);
    RequireThrows([&] { sceOpusCeltDecDestroy(state.data()); });
    Require(sceOpusCeltDecTerminate(&context) == 0);
    RequireThrows([&] { sceOpusCeltDecTerminate(&context); });
    return 0;
}
