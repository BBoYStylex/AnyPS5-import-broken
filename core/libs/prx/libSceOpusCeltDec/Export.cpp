#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceOpusDec/include/OpusDecoder.hpp"
#include <cstdint>

namespace {

OpusDecoder decoder("OpusCeltDec", true);

} // namespace

extern "C" {

int APS5_VABI sceOpusCeltDecInitialize(std::uint32_t* context) { return decoder.Initialize(context); }

int APS5_VABI sceOpusCeltDecTerminate(std::uint32_t* context) { return decoder.Terminate(context); }

int APS5_VABI sceOpusCeltDecGetSize(int channels) { return decoder.GetSize(channels); }

int APS5_VABI sceOpusCeltDecCreateEx(std::uint32_t* context, void* state, int sampleRate, int channels) {
    return decoder.CreateEx(context, state, sampleRate, channels);
}

int APS5_VABI sceOpusCeltDecDecode(void* state, const std::uint8_t* data, int bytes, std::int16_t* pcm, int capacity) {
    return decoder.Decode(state, data, bytes, pcm, capacity);
}

int APS5_VABI sceOpusCeltDecDestroy(void* state) { return decoder.Destroy(state); }

}
