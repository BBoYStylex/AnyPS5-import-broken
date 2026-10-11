#include "SceTypes.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <source_location>

extern "C" {
int APS5_VABI sceAudioOut2PortCreate(AudioOut2ContextHandle, const AudioOut2PortParam*, AudioOut2PortHandle*);
int APS5_VABI sceAudioOut2PortDestroy(AudioOut2PortHandle);
int APS5_VABI sceAudioOut2PortGetState(AudioOut2PortHandle, AudioOut2PortState*);
}

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (!condition) {
        std::fprintf(stderr, "AudioOut2 port state check failed at line %u\n", location.line());
        std::abort();
    }
}

int main() {
    AudioOut2PortParam params{};
    params.port_type = 0;
    params.data_format = 0x200;
    params.sampling_freq = 48000;
    AudioOut2PortHandle port{};
    Require(sceAudioOut2PortCreate(static_cast<AudioOut2ContextHandle>(1), &params, &port) == 0);
    Require(port != 0);
    AudioOut2PortState state{};
    Require(sceAudioOut2PortGetState(port, &state) == 0);
    Require(state.output == 0x1);
    Require(state.num_channels == 2);
    Require(sceAudioOut2PortDestroy(port) == 0);
    AudioOut2PortState after{};
    Require(sceAudioOut2PortGetState(port, &after) != 0);
    return 0;
}
