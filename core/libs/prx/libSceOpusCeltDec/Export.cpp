#include "prx/libSceOpusDec/src/Decoder.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>

namespace {

Opus::Decoders decoders("OpusCeltDec", true);

} // namespace

extern "C" {

int APS5_VABI sceOpusCeltDecInitialize(std::uint32_t* context) {
    return decoders.Initialize(context);
}

int APS5_VABI sceOpusCeltDecTerminate(std::uint32_t* context) {
    return decoders.Terminate(context);
}

int APS5_VABI sceOpusCeltDecGetSize(int channels) {
    return decoders.GetSize(channels);
}

int APS5_VABI sceOpusCeltDecCreateEx(std::uint32_t* context, void* state, int sampleRate, int channels) {
    return decoders.CreateEx(context, state, sampleRate, channels);
}

int APS5_VABI sceOpusCeltDecDecode(void* state, const std::uint8_t* data, int bytes, std::int16_t* pcm, int capacity) {
    return decoders.Decode(state, data, bytes, pcm, capacity);
}

int APS5_VABI sceOpusCeltDecDestroy(void* state) {
    return decoders.Destroy(state);
}

}
