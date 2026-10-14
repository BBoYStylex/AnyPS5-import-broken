#include "prx/libSceOpusDec/src/Decoder.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>

namespace {

Opus::Decoders decoders("OpusDec", false);

} // namespace

extern "C" {

int APS5_VABI sceOpusDecInitialize(std::uint32_t* context) {
    return decoders.Initialize(context);
}

int APS5_VABI sceOpusDecTerminate(std::uint32_t* context) {
    return decoders.Terminate(context);
}

int APS5_VABI sceOpusDecGetSize(int channels) {
    return decoders.GetSize(channels);
}

int APS5_VABI sceOpusDecCreateEx(std::uint32_t* context, void* state, int sampleRate, int channels) {
    return decoders.CreateEx(context, state, sampleRate, channels);
}

int APS5_VABI sceOpusDecDecode(void* state, const std::uint8_t* data, int bytes, std::int16_t* pcm, int capacity) {
    return decoders.Decode(state, data, bytes, pcm, capacity);
}

int APS5_VABI sceOpusDecDestroy(void* state) {
    return decoders.Destroy(state);
}

}
