#include "prx/libc/include/General.hpp"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <stdexcept>

namespace {

struct Decoder {
    std::uint32_t context = 0;
    int channels = 0;
    void* state = nullptr;
    ~Decoder() = default;
};

std::mutex mutex;
std::unordered_map<std::uint32_t, std::uint32_t*> contexts;
std::unordered_map<void*, std::unique_ptr<Decoder>> decoders;
std::uint32_t nextContext = 1;

}

extern "C" {

int APS5_VABI sceOpusCeltDecInitialize(std::uint32_t* context) {
    if (!context) throw std::runtime_error("OpusCeltDec: null context output");
    std::lock_guard lock(mutex);
    for (const auto& [id, owner] : contexts)
        if (owner == context) throw std::runtime_error("OpusCeltDec: context already initialized");
    if (nextContext == 0) throw std::runtime_error("OpusCeltDec: context identifiers exhausted");
    const auto id = nextContext++;
    contexts.emplace(id, context);
    *context = id;
    return 0;
}

int APS5_VABI sceOpusCeltDecTerminate(std::uint32_t* context) {
    if (!context) throw std::runtime_error("OpusCeltDec: null context");
    std::lock_guard lock(mutex);
    if (!contexts.contains(*context)) throw std::runtime_error("OpusCeltDec: unknown context");
    for (const auto& [state, decoder] : decoders)
        if (decoder->context == *context) throw std::runtime_error("OpusCeltDec: termination with live decoders is not implemented");
    contexts.erase(*context);
    return 0;
}

int APS5_VABI sceOpusCeltDecGetSize(int channels) {
    if (channels != 1 && channels != 2) throw std::runtime_error("OpusCeltDec: only mono and stereo are supported");
    return 640;
}

int APS5_VABI sceOpusCeltDecCreateEx(std::uint32_t* context, void* state, int sampleRate, int channels) {
    if (!context || !state) throw std::runtime_error("OpusCeltDec: null context or state");
    sceOpusCeltDecGetSize(channels);
    if (sampleRate != 48000) throw std::runtime_error("OpusCeltDec: sample rates other than 48000 Hz are not implemented");
    std::lock_guard lock(mutex);
    if (!contexts.contains(*context)) throw std::runtime_error("OpusCeltDec: unknown context");
    if (decoders.contains(state)) throw std::runtime_error("OpusCeltDec: state already owns a decoder");
    auto decoder = std::make_unique<Decoder>();
    decoder->context = *context;
    decoder->channels = channels;
    decoder->state = state;
    decoders.emplace(state, std::move(decoder));
    return 0;
}

int APS5_VABI sceOpusCeltDecDecode(void* state, const std::uint8_t* data, int bytes, std::int16_t* pcm, int capacity) {
    if (!data || bytes <= 0 || !pcm || capacity < 0)
        throw std::runtime_error("OpusCeltDec: invalid packet or PCM buffer");
    std::lock_guard lock(mutex);
    const auto found = decoders.find(state);
    if (found == decoders.end()) throw std::runtime_error("OpusCeltDec: unknown decoder state");
    auto& decoder = *found->second;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceOpusCeltDecDestroy(void* state) {
    std::lock_guard lock(mutex);
    if (decoders.erase(state) == 0) throw std::runtime_error("OpusCeltDec: unknown decoder state");
    return 0;
}

}
