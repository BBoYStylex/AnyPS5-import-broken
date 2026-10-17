#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceVoiceSetThreadsParams(void* params) {
 (void)params;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVoiceGetResourceInfo(VoiceResourceInfo* info) {
    if (info == nullptr) APS5_INVALID_ARG_EX;
    auto& voice = State();
    std::lock_guard lock(voice.mutex);
    if (!voice.initialized) Fail(__func__, "library not initialized");
    info->max_voice_input_ports = 7;
    info->max_voice_output_ports = 7;
    info->max_device_input_ports = 1;
    info->max_device_output_ports = 1;
    info->max_ports = 64;
    return 0;
}

int APS5_VABI sceVoiceEnableChat(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}



int APS5_VABI sceVoiceDisableChat(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
