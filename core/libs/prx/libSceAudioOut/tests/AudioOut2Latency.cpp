#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

extern "C" {
int APS5_VABI sceAudioOut2Initialize();
int APS5_VABI sceAudioOut2Set3DLatency(int, std::uint32_t);
int APS5_VABI sceAudioOut2MasteringInit(std::uint32_t);
int APS5_VABI sceAudioOut2MasteringTerm();
int APS5_VABI sceAudioOut2MasteringSetParam(const void*, std::uint32_t, std::uint32_t);
int APS5_VABI sceAudioOut2MasteringGetState(void*, std::uint32_t, std::uint64_t);
int APS5_VABI sceAudioOut2EnableChat();
int APS5_VABI sceAudioOut2DisableChat();
}

static void Require(bool value, const char* message) {
    if (value) return;
    std::fprintf(stderr, "%s\n", message);
    std::abort();
}

template<typename TFunction>
static bool ThrowsRuntimeError(TFunction function) {
    try {
        function();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

namespace {

constexpr int systemUser = 0xFF;
constexpr int user = 0x10000000;

void TestSet3DLatency() {
    Require(sceAudioOut2Set3DLatency(systemUser, 2) == 0, "latency 2 for the system user must be accepted");
    Require(sceAudioOut2Set3DLatency(systemUser, 2) == 0, "latency 2 must be accepted again");
    Require(sceAudioOut2Set3DLatency(systemUser, 1) == 0, "latency 1 for the system user must be accepted");
    Require(ThrowsRuntimeError([] { sceAudioOut2Set3DLatency(systemUser, 0); }), "latency 0 must throw");
    Require(ThrowsRuntimeError([] { sceAudioOut2Set3DLatency(systemUser, 3); }), "latency 3 must throw");
    Require(ThrowsRuntimeError([] { sceAudioOut2Set3DLatency(user, 2); }), "a user other than the system user must throw");
}

void TestMasteringInit() {
    Require(sceAudioOut2MasteringInit(0) == 0, "flags 0 must be accepted");
    Require(ThrowsRuntimeError([] { sceAudioOut2MasteringInit(1); }), "flags 1 must throw");
}

void TestMasteringTerm() {
    Require(sceAudioOut2MasteringInit(0) == 0, "a second initialization must be accepted");
    Require(sceAudioOut2MasteringTerm() == 0, "termination must be accepted");
    Require(sceAudioOut2MasteringTerm() == 0, "termination must be accepted for each initialization");
}

void TestMasteringSetParam() {
    const std::uint32_t params[4] = {1u, 0u, 0u, 0u};
    Require(sceAudioOut2MasteringSetParam(params, 0, 0) == 0, "mastering parameters must be accepted");
    Require(ThrowsRuntimeError([] { sceAudioOut2MasteringSetParam(nullptr, 0, 0); }), "null mastering parameters must throw");
}

static_assert(sizeof(AudioOut2MasteringCompressorStates<3>) == 208);
static_assert(sizeof(AudioOut2MasteringCompressorStates<4>) == 272);
static_assert(sizeof(AudioOut2MasteringLimiterStates) == 112);
static_assert(sizeof(AudioOut2MasteringStates<3>) == 336);
static_assert(sizeof(AudioOut2MasteringStates<4>) == 400);

template <std::uint32_t Bands>
void RequireClearedStates(std::uint32_t statesId, std::uint32_t compressorId) {
    AudioOut2MasteringStates<Bands> states;
    std::memset(&states, 0xAB, sizeof(states));
    states.states_header.states_id = statesId;
    Require(sceAudioOut2MasteringGetState(&states, 0, 0) == 0, "known states ids must be accepted");
    Require(states.states_header.states_id == statesId, "the states id must be kept");
    Require(states.compressor_states.descriptor.id == compressorId, "the compressor descriptor id must be set");
    Require(states.compressor_states.descriptor.size == sizeof(states.compressor_states), "the compressor descriptor size must be set");
    Require(states.limiter_states.descriptor.id == 0x01010003u, "the limiter descriptor id must be set");
    Require(states.limiter_states.descriptor.size == sizeof(states.limiter_states), "the limiter descriptor size must be set");
    Require(states.compressor_states.input_rms[Bands - 1][7] == 0.0f, "the compressor levels must be zero");
    Require(states.limiter_states.gain_peak[7] == 0.0f, "the limiter peaks must be zero");
}

void TestMasteringGetState() {
    RequireClearedStates<3>(1, 0x01010001u);
    RequireClearedStates<4>(2, 0x01020001u);
    Require(sceAudioOut2MasteringGetState(nullptr, 0, 0) == static_cast<int>(0x80268201), "null states must be refused");
    AudioOut2MasteringStatesHeader unknown{3};
    Require(sceAudioOut2MasteringGetState(&unknown, 0, 0) == static_cast<int>(0x80268204), "an unknown states id must be refused");
}

void TestEnableChat() {
    Require(sceAudioOut2EnableChat() == 0, "enabling chat must be accepted");
    Require(sceAudioOut2EnableChat() == 0, "enabling chat again must be accepted");
    Require(sceAudioOut2DisableChat() == 0, "disabling chat must be accepted");
    Require(sceAudioOut2DisableChat() == 0, "disabling chat again must be accepted");
    Require(sceAudioOut2EnableChat() == 0, "enabling chat after disabling it must be accepted");
}

}

int main() {
    Require(sceAudioOut2Initialize() == 0, "initialization must succeed");
    TestSet3DLatency();
    TestMasteringInit();
    TestMasteringTerm();
    TestMasteringSetParam();
    TestMasteringGetState();
    TestEnableChat();
    return 0;
}
