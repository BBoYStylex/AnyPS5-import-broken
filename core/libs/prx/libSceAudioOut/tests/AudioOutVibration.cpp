#include "prx/libScePad/include/PadState.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string_view>

extern "C" {
int APS5_VABI sceAudioOutOpen(int, int, int, std::uint32_t, std::uint32_t, std::uint32_t);
int APS5_VABI sceAudioOutClose(int);
int APS5_VABI sceAudioOutOutput(int, const void*);
int APS5_VABI sceAudioOutOutputs(AudioOutOutputParam*, std::uint32_t);
}

namespace {

constexpr int VibrationPort = 10;
constexpr std::uint32_t Frames = 256;
constexpr std::uint32_t Rate = 48000;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct OpenPort {
    int handle = -1;
    ~OpenPort() {
        if (handle > 0) sceAudioOutClose(handle);
    }
};

enum class Case {
    S16,
    F32,
    BatchS16,
    BatchF32,
    PadBridge,
};

Case ParseCase(std::string_view name) {
    if (name == "--s16") return Case::S16;
    if (name == "--f32") return Case::F32;
    if (name == "--batch_s16") return Case::BatchS16;
    if (name == "--batch_f32") return Case::BatchF32;
    if (name == "--pad_control") return Case::PadBridge;
    throw std::runtime_error("unknown audio vibration test case");
}

template<typename TSample>
void FillStereo(std::array<TSample, Frames * 2>& pcm, TSample left, TSample right) {
    for (std::uint32_t frame = 0; frame < Frames; ++frame) {
        pcm[frame * 2] = left;
        pcm[frame * 2 + 1] = right;
    }
}

void Run(Case testCase) {
    Pad::Initialize();
    Pad::SetVibrationMode(1);
    std::uint32_t sequence = 0;
    PadOutputState state{};
    static_cast<void>(PadFetchOutput_nid_postfix(&sequence, &state));

    if (testCase == Case::PadBridge) {
        PadSubmitAudioHaptics_nid_postfix(128, 255);
    } else {
        const bool floating = testCase == Case::F32 || testCase == Case::BatchF32;
        const bool batched = testCase == Case::BatchS16 || testCase == Case::BatchF32;
        OpenPort port;
        port.handle = sceAudioOutOpen(0x10000000, VibrationPort, 0, Frames, Rate, floating ? 4u : 1u);
        Check(port.handle > 0, "could not open a legacy vibration port");

        std::array<std::int16_t, Frames * 2> integerPcm{};
        std::array<float, Frames * 2> floatPcm{};
        FillStereo(integerPcm, static_cast<std::int16_t>(8192), static_cast<std::int16_t>(-16384));
        FillStereo(floatPcm, 0.25f, -0.5f);
        const void* pcm = floating ? static_cast<const void*>(floatPcm.data()) : static_cast<const void*>(integerPcm.data());

        int result = 0;
        if (batched) {
            AudioOutOutputParam output{port.handle, pcm};
            result = sceAudioOutOutputs(&output, 1);
        } else {
            result = sceAudioOutOutput(port.handle, pcm);
        }
        Check(result == static_cast<int>(Frames), "legacy vibration output did not accept its frame count");
    }

    Check(PadFetchOutput_nid_postfix(&sequence, &state), "PCM did not publish a new controller output state");
    Check(state.audioVibrationLarge == 128 && state.audioVibrationSmall == 255,
        "legacy vibration PCM produced unexpected motor levels");
    Check(state.vibrationLarge == 0 && state.vibrationSmall == 0,
        "audio vibration changed the regular motor levels");
}

}

int main(int argc, char** argv) {
    try {
        Check(argc == 2, "expected one test case argument");
        Run(ParseCase(argv[1]));
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
