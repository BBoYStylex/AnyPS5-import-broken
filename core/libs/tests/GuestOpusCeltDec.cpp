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

const std::uint8_t CELT_STEREO[4][60] = {
    {0xFC, 0x9F, 0xF9, 0x33, 0x84, 0xFB, 0x41, 0x76, 0xA8, 0x21, 0xCE, 0x57, 0xBB, 0x16, 0x49, 0x05, 0x21, 0x3F, 0x2A, 0xC4,
     0x79, 0xD0, 0x73, 0x62, 0x89, 0x7E, 0xF9, 0x29, 0x75, 0x1E, 0xAC, 0x80, 0xAE, 0x30, 0xF1, 0x46, 0x18, 0x1F, 0xF8, 0x8B,
     0xD0, 0xC4, 0x37, 0xF1, 0x12, 0x47, 0x01, 0xF9, 0x43, 0xA4, 0x1A, 0x8C, 0x80, 0x40, 0xF0, 0x0A, 0xC3, 0xFD, 0x4C, 0xB1},
    {0xFC, 0x9E, 0xA4, 0xC8, 0xDF, 0x47, 0x28, 0xAE, 0x34, 0xFD, 0x32, 0x11, 0xB9, 0x69, 0x99, 0x7B, 0x61, 0xC6, 0x21, 0x94,
     0x0E, 0x20, 0xED, 0xC2, 0x95, 0x07, 0x78, 0x0D, 0x00, 0x92, 0x42, 0x12, 0xD4, 0xE9, 0xEC, 0x5E, 0xDC, 0xF4, 0xB9, 0x7A,
     0x2E, 0x23, 0xA1, 0xC6, 0x4C, 0xD7, 0x69, 0xEC, 0x34, 0x8F, 0x78, 0x21, 0x35, 0xF1, 0x1B, 0xB0, 0xF0, 0xF5, 0x5C, 0xD1},
    {0xFC, 0x9E, 0xA6, 0x12, 0xDC, 0x36, 0xC3, 0x4B, 0x2C, 0x3D, 0x85, 0x17, 0x32, 0xE1, 0x56, 0x62, 0xFE, 0x39, 0xDB, 0x85,
     0xFD, 0x83, 0x0D, 0x90, 0xF9, 0x32, 0xEB, 0xEB, 0x38, 0xFD, 0x3D, 0x96, 0x26, 0x21, 0xE6, 0x38, 0x6A, 0x8F, 0xDD, 0x1F,
     0x68, 0xBD, 0xAF, 0x4F, 0x9D, 0x08, 0x16, 0xED, 0x7B, 0x2A, 0x62, 0x4C, 0x7A, 0x85, 0xAB, 0x7B, 0x0F, 0x3D, 0x43, 0xD1},
    {0xFC, 0x7F, 0xDF, 0x09, 0x18, 0xD2, 0x71, 0xA1, 0xE6, 0x6C, 0x70, 0xED, 0xCA, 0x2F, 0x47, 0x79, 0xA7, 0xEA, 0xE3, 0xD1,
     0x9B, 0xCD, 0x9E, 0x87, 0xE5, 0xE0, 0x0A, 0x86, 0x0A, 0xEE, 0x8E, 0x63, 0xE4, 0x77, 0x45, 0xE9, 0xF0, 0x82, 0xF8, 0x2A,
     0x13, 0xD2, 0xDB, 0x17, 0x11, 0x2E, 0x36, 0x10, 0x03, 0x30, 0x20, 0x00, 0xF7, 0x42, 0xD8, 0x71, 0x7E, 0xB0, 0xF3, 0x3C},
};

const std::uint8_t CELT_MONO[4][20] = {
    {0xD0, 0x7C, 0xD7, 0x29, 0x41, 0xBE, 0xA0, 0xCB, 0x2A, 0xD6, 0xD4, 0xDA, 0x71, 0x33, 0xA5, 0x94, 0x16, 0x78, 0x4C, 0x08},
    {0xD0, 0x9D, 0x41, 0xE7, 0xEC, 0x1D, 0xAD, 0x93, 0x26, 0x3F, 0x29, 0xF8, 0x09, 0x9E, 0x25, 0x02, 0x3E, 0x10, 0x40, 0xD1},
    {0xD0, 0x9D, 0x6B, 0x86, 0x37, 0x48, 0x9D, 0x4C, 0x47, 0x49, 0xAB, 0xAD, 0x22, 0xCB, 0xF8, 0x5E, 0xDD, 0xD5, 0xB4, 0xD1},
    {0xD0, 0x7F, 0xDE, 0x50, 0xEF, 0x65, 0x1A, 0x62, 0x45, 0x17, 0x30, 0x1E, 0x30, 0x4F, 0x85, 0x89, 0x02, 0x4F, 0x4E, 0xEB},
};

void RequireTone(const std::int16_t* pcm, int samples, int channels) {
    for (int channel = 0; channel < channels; ++channel) {
        int peak = 0, crossings = 0;
        for (int sample = 0; sample < samples; ++sample) {
            const int value = pcm[sample * channels + channel];
            peak = std::max(peak, std::abs(value));
            if (sample > 0 && (value < 0) != (pcm[(sample - 1) * channels + channel] < 0)) ++crossings;
        }
        Require(peak > 100 && peak < 200);
        Require(crossings >= samples / 24 - 4 && crossings <= samples / 24 + 4);
    }
}

} // namespace

int main() {
    std::uint32_t context = 0, otherContext = 0;
    Require(sceOpusCeltDecInitialize(&context) == 0);
    Require(sceOpusCeltDecInitialize(&otherContext) == 0 && context != otherContext);
    RequireThrows([&] { sceOpusCeltDecInitialize(&context); });
    Require(sceOpusCeltDecGetSize(1) == 640 && sceOpusCeltDecGetSize(2) == 640);
    RequireThrows([&] { sceOpusCeltDecGetSize(3); });
    std::array<std::uint8_t, 640> state{}, otherState{}, monoState{}, unsupported{};
    for (int rate : {8000, 12000, 16000, 24000})
        RequireThrows([&] { sceOpusCeltDecCreateEx(&context, unsupported.data(), rate, 2); });
    Require(sceOpusCeltDecCreateEx(&context, state.data(), 48000, 2) == 0);
    Require(sceOpusCeltDecCreateEx(&otherContext, otherState.data(), 48000, 2) == 0);
    Require(sceOpusCeltDecCreateEx(&context, monoState.data(), 48000, 1) == 0);
    RequireThrows([&] { sceOpusCeltDecCreateEx(&context, state.data(), 48000, 2); });
    RequireThrows([&] { sceOpusCeltDecTerminate(&context); });

    std::array<std::int16_t, 1922> pcm{}, reference{};
    pcm.fill(12345);
    const std::uint8_t silk[] = {0x08, 0x00};
    const std::uint8_t hybrid[] = {0x7C, 0x00};
    RequireThrows([&] { sceOpusCeltDecDecode(state.data(), silk, sizeof(silk), pcm.data() + 1, 3840); });
    RequireThrows([&] { sceOpusCeltDecDecode(state.data(), hybrid, sizeof(hybrid), pcm.data() + 1, 3840); });
    RequireThrows([&] { sceOpusCeltDecDecode(state.data(), CELT_STEREO[0], 60, pcm.data() + 1, 3839); });
    RequireThrows([&] { sceOpusCeltDecDecode(state.data(), nullptr, 0, pcm.data() + 1, 3840); });
    Require(std::all_of(pcm.begin(), pcm.end(), [](auto value) { return value == 12345; }));

    for (int frame = 0; frame < 4; ++frame) {
        Require(sceOpusCeltDecDecode(state.data(), CELT_STEREO[frame], 60, pcm.data() + 1, 3841) == 3840);
        Require(pcm.front() == 12345 && pcm.back() == 12345);
        Require(sceOpusCeltDecDecode(otherState.data(), CELT_STEREO[frame], 60, reference.data() + 1, 3840) == 3840);
        Require(std::equal(pcm.begin() + 1, pcm.end() - 1, reference.begin() + 1));
        if (frame == 1 || frame == 2) RequireTone(pcm.data() + 1, 960, 2);
    }

    std::array<std::int16_t, 482> mono{};
    for (int frame = 0; frame < 4; ++frame) {
        mono.fill(12345);
        Require(sceOpusCeltDecDecode(monoState.data(), CELT_MONO[frame], 20, mono.data() + 1, 960) == 960);
        Require(mono.front() == 12345 && mono.back() == 12345);
        if (frame == 1 || frame == 2) RequireTone(mono.data() + 1, 480, 1);
    }

    Require(sceOpusCeltDecDestroy(state.data()) == 0);
    RequireThrows([&] { sceOpusCeltDecDestroy(state.data()); });
    RequireThrows([&] { sceOpusCeltDecDecode(state.data(), CELT_STEREO[0], 60, pcm.data(), 3840); });
    Require(sceOpusCeltDecDestroy(monoState.data()) == 0);
    Require(sceOpusCeltDecTerminate(&context) == 0);
    RequireThrows([&] { sceOpusCeltDecTerminate(&context); });
    RequireThrows([&] { sceOpusCeltDecCreateEx(&context, state.data(), 48000, 2); });
    Require(sceOpusCeltDecDestroy(otherState.data()) == 0);
    Require(sceOpusCeltDecTerminate(&otherContext) == 0);
}
