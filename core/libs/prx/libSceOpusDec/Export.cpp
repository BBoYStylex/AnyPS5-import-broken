#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceOpusDec/include/OpusDecoder.hpp"
#include <cstdint>

namespace {

OpusDecoder decoder("OpusDec", false);

} // namespace

extern "C" {

int APS5_VABI sceOpusDecInitialize(std::uint32_t* context) { return decoder.Initialize(context); }

int APS5_VABI sceOpusDecTerminate(std::uint32_t* context) { return decoder.Terminate(context); }

int APS5_VABI sceOpusDecGetSize(int channels) { return decoder.GetSize(channels); }

int APS5_VABI sceOpusDecCreateEx(std::uint32_t* context, void* state, int sampleRate, int channels) {
    return decoder.CreateEx(context, state, sampleRate, channels);
}

int APS5_VABI sceOpusDecDecode(void* state, const std::uint8_t* data, int bytes, std::int16_t* pcm, int capacity) {
    return decoder.Decode(state, data, bytes, pcm, capacity);
}

int APS5_VABI sceOpusDecDestroy(void* state) { return decoder.Destroy(state); }

}
