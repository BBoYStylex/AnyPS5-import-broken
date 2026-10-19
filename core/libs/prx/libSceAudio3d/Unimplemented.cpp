#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceAudio3dPortCreate(std::uint32_t granularity, std::uint32_t rate, std::int64_t reserved, void* id) {
    (void)granularity;
    (void)rate;
    (void)reserved;
    (void)id;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAudio3dPortDestroy(std::uint32_t port_id) {
    (void)port_id;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAudio3dPortGetStatus(std::uint32_t port_id, void* status) {
    (void)port_id;
    (void)status;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

const char* APS5_VABI sceAudio3dStrError(int error_code) {
    (void)error_code;
    NotImplemented_nid_no_patch(__func__);
    return nullptr;
}

int APS5_VABI sceAudio3dBedWrite(std::uint32_t port_id, std::uint32_t num_channels, std::uint32_t format, const void* buffer, std::uint32_t num_samples) {
    (void)port_id;
    (void)num_channels;
    (void)format;
    (void)buffer;
    (void)num_samples;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAudio3dBedWrite2(std::uint32_t port_id, std::uint32_t num_channels, std::uint32_t format, const void* buffer, std::uint32_t num_samples, std::uint32_t output_route, std::uint8_t restricted) {
    (void)port_id;
    (void)num_channels;
    (void)format;
    (void)buffer;
    (void)num_samples;
    (void)output_route;
    (void)restricted;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::size_t APS5_VABI sceAudio3dGetSpeakerArrayMemorySize(std::uint32_t num_speakers, std::uint8_t is_3d) {
    (void)num_speakers;
    (void)is_3d;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAudio3dCreateSpeakerArray(void** handle, const void* parameters) {
    (void)handle;
    (void)parameters;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAudio3dDeleteSpeakerArray(void* handle) {
    (void)handle;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAudio3dGetSpeakerArrayMixCoefficients(void* handle, Audio3dPosition position, float spread, float* coefficients, std::uint32_t num_coefficients) {
    (void)handle;
    (void)position;
    (void)spread;
    (void)coefficients;
    (void)num_coefficients;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAudio3dGetSpeakerArrayMixCoefficients2(void* handle, Audio3dPosition position, float spread, float* coefficients, std::uint32_t num_coefficients, std::uint8_t height_aware, float downmix_spread_radius) {
    (void)handle;
    (void)position;
    (void)spread;
    (void)coefficients;
    (void)num_coefficients;
    (void)height_aware;
    (void)downmix_spread_radius;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAudio3dAudioOutOpen(std::uint32_t port_id, int user_id, int type, int index, std::uint32_t length, std::uint32_t frequency, std::uint32_t param) {
    (void)port_id;
    (void)user_id;
    (void)type;
    (void)index;
    (void)length;
    (void)frequency;
    (void)param;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAudio3dAudioOutClose(int handle) {
    (void)handle;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAudio3dAudioOutOutput(int handle, const void* ptr) {
    (void)handle;
    (void)ptr;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAudio3dAudioOutOutputs(const void* param, std::uint32_t num) {
    (void)param;
    (void)num;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAudio3dReportRegisterHandler() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAudio3dReportUnregisterHandler() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
