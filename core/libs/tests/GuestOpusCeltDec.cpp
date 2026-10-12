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

const std::uint8_t CELT_STEREO[] = {
    0x51, 0x00, 0xFC, 0x17, 0x6C, 0x91, 0xA1, 0xB0, 0x25, 0xBC, 0x67, 0x0D, 0xB7, 0xDD, 0xDE, 0xB0, 0x59, 0xC8, 0x52, 0x5C, 0xEC, 0x60, 0x8E, 0x1C,
    0x07, 0x58, 0xCA, 0x16, 0x78, 0xE0, 0xD2, 0xCC, 0x5D, 0xFD, 0xCC, 0xF5, 0x1E, 0xB9, 0x30, 0x65, 0x11, 0x60, 0x13, 0x4E, 0x31, 0x04, 0xDB, 0x3A,
    0x28, 0xBB, 0x2D, 0x76, 0x59, 0x4E, 0x52, 0x0E, 0xC8, 0xF7, 0x9E, 0x3A, 0x46, 0x26, 0xC2, 0xD3, 0x78, 0xC4, 0xD5, 0xDE, 0x79, 0xB0, 0x67, 0x03,
    0xDB, 0x38, 0x6E, 0xE0, 0x80, 0x15, 0x7C, 0x14, 0x3F, 0xE4, 0x55, 0x51, 0x00, 0xFC, 0x70, 0x5F, 0x28, 0xB0, 0xD0, 0xA3, 0xB4, 0x5D, 0xB6, 0x86,
    0xBC, 0x81, 0xF1, 0xE2, 0xFB, 0xE4, 0xDD, 0xC1, 0xA5, 0x8A, 0xEA, 0x72, 0x77, 0x33, 0xB0, 0xF8, 0x17, 0x0F, 0x4D, 0x9F, 0xCF, 0xAF, 0x69, 0xB6,
    0x72, 0xFC, 0x1A, 0x0A, 0xC2, 0x3C, 0x16, 0x3F, 0xD7, 0xD1, 0x0F, 0xF0, 0xD1, 0x2D, 0xEB, 0xE9, 0x41, 0xA4, 0x14, 0xC0, 0x63, 0x9F, 0xD4, 0xF1,
    0xCA, 0xF3, 0x5C, 0x4F, 0x8B, 0xB9, 0xDA, 0x3A, 0x22, 0xB9, 0x7A, 0xEC, 0x49, 0xB7, 0x64, 0x46, 0x85, 0xFC, 0x14, 0x32, 0xAD, 0xF5, 0x51, 0x00,
    0xFC, 0x16, 0x11, 0xDD, 0xDE, 0xDC, 0xA9, 0xA4, 0x3B, 0x27, 0x2F, 0xD1, 0xFE, 0x18, 0x06, 0x7D, 0x3E, 0x87, 0x1B, 0xA3, 0x11, 0x2D, 0xCC, 0xBB,
    0x0A, 0x09, 0x49, 0x65, 0xCB, 0x3C, 0xF7, 0x2C, 0x22, 0x51, 0x91, 0x04, 0xDD, 0xD8, 0xAD, 0xF2, 0x73, 0x54, 0xF1, 0xC3, 0x37, 0xA3, 0x5D, 0x85,
    0xC9, 0xD2, 0x41, 0x37, 0xC6, 0xEC, 0xE3, 0x02, 0x07, 0xF7, 0x4B, 0x3D, 0x85, 0xB5, 0xB6, 0xC7, 0x0F, 0xDC, 0xD0, 0xED, 0x35, 0xE8, 0x8D, 0x6B,
    0x4E, 0xCD, 0x77, 0xFF, 0xA3, 0x3F, 0x0D, 0x40, 0xA0,
};

} // namespace

int main() {
    std::uint32_t context = 0, otherContext = 0;
    Require(sceOpusCeltDecInitialize(&context) == 0);
    Require(sceOpusCeltDecInitialize(&otherContext) == 0 && context != otherContext);
    RequireThrows([&] { sceOpusCeltDecInitialize(&context); });
    Require(sceOpusCeltDecGetSize(1) == 640 && sceOpusCeltDecGetSize(2) == 640);
    RequireThrows([&] { sceOpusCeltDecGetSize(3); });
    std::array<std::uint8_t, 640> state{}, otherState{}, monoState{}, unsupported{};
    Require(sceOpusCeltDecCreateEx(&context, state.data(), 48000, 2) == 0);
    Require(sceOpusCeltDecCreateEx(&otherContext, otherState.data(), 48000, 2) == 0);
    Require(sceOpusCeltDecCreateEx(&context, monoState.data(), 48000, 1) == 0);
    RequireThrows([&] { sceOpusCeltDecTerminate(&context); });
    RequireThrows([&] { sceOpusCeltDecCreateEx(&context, state.data(), 48000, 2); });
    RequireThrows([&] { sceOpusCeltDecCreateEx(&context, unsupported.data(), 24000, 2); });
    std::array<std::int16_t, 1922> pcm{}, reference{};
    pcm.fill(12345);
    const std::uint8_t hybrid[] = {0x7C, 0x87};
    const std::uint8_t silk[] = {0x0C, 0x87};
    RequireThrows([&] { sceOpusCeltDecDecode(state.data(), hybrid, 2, pcm.data() + 1, 3840); });
    RequireThrows([&] { sceOpusCeltDecDecode(state.data(), silk, 2, pcm.data() + 1, 3840); });
    RequireThrows([&] { sceOpusCeltDecDecode(state.data(), CELT_STEREO + 2, CELT_STEREO[0], pcm.data() + 1, 3839); });
    RequireThrows([&] { sceOpusCeltDecDecode(state.data(), nullptr, 0, pcm.data() + 1, 3840); });
    Require(std::all_of(pcm.begin(), pcm.end(), [](auto value) { return value == 12345; }));
    std::size_t offset = 0;
    for (int frame = 0; frame < 3; ++frame) {
        const int bytes = CELT_STEREO[offset] | (CELT_STEREO[offset + 1] << 8);
        const auto* packet = CELT_STEREO + offset + 2;
        Require(packet[0] >> 3 >= 16);
        Require(sceOpusCeltDecDecode(state.data(), packet, bytes, pcm.data() + 1, 3841) == 3840);
        Require(pcm.front() == 12345 && pcm.back() == 12345);
        Require(sceOpusCeltDecDecode(otherState.data(), packet, bytes, reference.data() + 1, 3840) == 3840);
        Require(std::equal(pcm.begin() + 1, pcm.end() - 1, reference.begin() + 1));
        if (frame > 0) {
            int peak = 0;
            for (auto it = pcm.begin() + 1; it != pcm.end() - 1; ++it) peak = std::max(peak, std::abs(static_cast<int>(*it)));
            Require(peak > 2500 && peak < 4500);
        }
        std::array<std::int16_t, 962> mono{};
        mono.front() = mono.back() = 12345;
        Require(sceOpusCeltDecDecode(monoState.data(), packet, bytes, mono.data() + 1, 1921) == 1920);
        Require(mono.front() == 12345 && mono.back() == 12345);
        Require(std::any_of(mono.begin() + 1, mono.end() - 1, [](auto value) { return value != 0; }));
        offset += bytes + 2;
    }
    Require(sceOpusCeltDecDestroy(state.data()) == 0);
    RequireThrows([&] { sceOpusCeltDecDestroy(state.data()); });
    RequireThrows([&] { sceOpusCeltDecDecode(state.data(), CELT_STEREO + 2, CELT_STEREO[0], pcm.data(), 3840); });
    Require(sceOpusCeltDecDestroy(monoState.data()) == 0);
    Require(sceOpusCeltDecTerminate(&context) == 0);
    RequireThrows([&] { sceOpusCeltDecTerminate(&context); });
    RequireThrows([&] { sceOpusCeltDecCreateEx(&context, state.data(), 48000, 2); });
    Require(sceOpusCeltDecDestroy(otherState.data()) == 0);
    Require(sceOpusCeltDecTerminate(&otherContext) == 0);
}
