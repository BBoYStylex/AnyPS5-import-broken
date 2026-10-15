#include <cstdint>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr int ErrorMasteringInvalidApiParam = static_cast<int>(0x80268201);
constexpr int ErrorMasteringInvalidStatesId = static_cast<int>(0x80268204);
constexpr std::uint32_t StatesIdDefault = 1;
constexpr std::uint32_t StatesIdV2 = 2;
constexpr std::uint32_t CompressorBandsDefault = 3;
constexpr std::uint32_t CompressorBandsV2 = 4;
constexpr std::uint32_t CompressorStructIdDefault = 0x01010001;
constexpr std::uint32_t CompressorStructIdV2 = 0x01020001;
constexpr std::uint32_t LimiterStructId = 0x01010003;

template <std::uint32_t Bands>
void ClearStates(AudioOut2MasteringStatesHeader* header, std::uint32_t statesId, std::uint32_t compressorStructId) {
    auto* states = reinterpret_cast<AudioOut2MasteringStates<Bands>*>(header);
    *states = {};
    states->states_header.states_id = statesId;
    states->compressor_states.descriptor = {compressorStructId, sizeof(states->compressor_states)};
    states->limiter_states.descriptor = {LimiterStructId, sizeof(states->limiter_states)};
}

}

extern "C" {

int APS5_VABI sceAudioOut2MasteringInit(uint32_t flags) {
    if (flags != 0) NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAudioOut2MasteringTerm(void) {
    return 0;
}

int APS5_VABI sceAudioOut2MasteringGetState(AudioOut2MasteringStatesHeader* state, uint32_t output, AudioOut2UserHandle user) {
    (void)output;
    (void)user;
    if (state == nullptr) return ErrorMasteringInvalidApiParam;
    switch (state->states_id) {
    case StatesIdDefault:
        ClearStates<CompressorBandsDefault>(state, StatesIdDefault, CompressorStructIdDefault);
        return 0;
    case StatesIdV2:
        ClearStates<CompressorBandsV2>(state, StatesIdV2, CompressorStructIdV2);
        return 0;
    default:
        return ErrorMasteringInvalidStatesId;
    }
}

int APS5_VABI sceAudioOut2MasteringSetParam(const AudioOut2MasteringParamsHeader* param, uint32_t output, uint32_t flags) {
    (void)output;
    (void)flags;
    if (param == nullptr) NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
