#include "SceTypes.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

extern "C" {
int APS5_VABI sceAudio3dPortCreate(std::uint32_t granularity, std::uint32_t rate, std::int64_t reserved, void* id);
int APS5_VABI sceAudio3dPortDestroy(std::uint32_t port_id);
int APS5_VABI sceAudio3dPortGetStatus(std::uint32_t port_id, void* status);
const char* APS5_VABI sceAudio3dStrError(int error_code);
int APS5_VABI sceAudio3dBedWrite(std::uint32_t port_id, std::uint32_t num_channels, std::uint32_t format, const void* buffer, std::uint32_t num_samples);
int APS5_VABI sceAudio3dBedWrite2(std::uint32_t port_id, std::uint32_t num_channels, std::uint32_t format, const void* buffer, std::uint32_t num_samples, std::uint32_t output_route, std::uint8_t restricted);
std::size_t APS5_VABI sceAudio3dGetSpeakerArrayMemorySize(std::uint32_t num_speakers, std::uint8_t is_3d);
int APS5_VABI sceAudio3dCreateSpeakerArray(void** handle, const void* parameters);
int APS5_VABI sceAudio3dDeleteSpeakerArray(void* handle);
int APS5_VABI sceAudio3dGetSpeakerArrayMixCoefficients(void* handle, Audio3dPosition position, float spread, float* coefficients, std::uint32_t num_coefficients);
int APS5_VABI sceAudio3dGetSpeakerArrayMixCoefficients2(void* handle, Audio3dPosition position, float spread, float* coefficients, std::uint32_t num_coefficients, std::uint8_t height_aware, float downmix_spread_radius);
int APS5_VABI sceAudio3dAudioOutOpen(std::uint32_t port_id, int user_id, int type, int index, std::uint32_t length, std::uint32_t frequency, std::uint32_t param);
int APS5_VABI sceAudio3dAudioOutClose(int handle);
int APS5_VABI sceAudio3dAudioOutOutput(int handle, const void* ptr);
int APS5_VABI sceAudio3dAudioOutOutputs(const void* param, std::uint32_t num);
int APS5_VABI sceAudio3dReportRegisterHandler();
int APS5_VABI sceAudio3dReportUnregisterHandler();
}

namespace {

void Require(bool value, const char* message) {
    if (value) return;
    std::fprintf(stderr, "%s\n", message);
    std::abort();
}

template <typename F>
void RequireNotImplemented(const char* name, F call) {
    try {
        call();
    } catch (const std::runtime_error& error) {
        Require(std::string(error.what()) == std::string(name) + " not implemented", name);
        return;
    }
    Require(false, name);
}

void CheckPorts() {
    RequireNotImplemented("sceAudio3dPortCreate", [] { sceAudio3dPortCreate(0, 0, 0, nullptr); });
    RequireNotImplemented("sceAudio3dPortDestroy", [] { sceAudio3dPortDestroy(0); });
    RequireNotImplemented("sceAudio3dPortGetStatus", [] { sceAudio3dPortGetStatus(0, nullptr); });
}

void CheckErrorText() {
    RequireNotImplemented("sceAudio3dStrError", [] { sceAudio3dStrError(0); });
}

void CheckBeds() {
    RequireNotImplemented("sceAudio3dBedWrite", [] { sceAudio3dBedWrite(0, 2, 0, nullptr, 0); });
    RequireNotImplemented("sceAudio3dBedWrite2", [] { sceAudio3dBedWrite2(0, 2, 0, nullptr, 0, 0, 0); });
}

void CheckSpeakerArrays() {
    RequireNotImplemented("sceAudio3dGetSpeakerArrayMemorySize", [] { sceAudio3dGetSpeakerArrayMemorySize(0, 0); });
    RequireNotImplemented("sceAudio3dCreateSpeakerArray", [] { sceAudio3dCreateSpeakerArray(nullptr, nullptr); });
    RequireNotImplemented("sceAudio3dDeleteSpeakerArray", [] { sceAudio3dDeleteSpeakerArray(nullptr); });
    RequireNotImplemented("sceAudio3dGetSpeakerArrayMixCoefficients", [] {
        sceAudio3dGetSpeakerArrayMixCoefficients(nullptr, Audio3dPosition{}, 0.0f, nullptr, 0);
    });
    RequireNotImplemented("sceAudio3dGetSpeakerArrayMixCoefficients2", [] {
        sceAudio3dGetSpeakerArrayMixCoefficients2(nullptr, Audio3dPosition{}, 0.0f, nullptr, 0, 0, 0.0f);
    });
}

void CheckAudioOut() {
    RequireNotImplemented("sceAudio3dAudioOutOpen", [] { sceAudio3dAudioOutOpen(0, 0, 0, 0, 0, 0, 0); });
    RequireNotImplemented("sceAudio3dAudioOutClose", [] { sceAudio3dAudioOutClose(0); });
    RequireNotImplemented("sceAudio3dAudioOutOutput", [] { sceAudio3dAudioOutOutput(0, nullptr); });
    RequireNotImplemented("sceAudio3dAudioOutOutputs", [] { sceAudio3dAudioOutOutputs(nullptr, 0); });
}

void CheckReports() {
    RequireNotImplemented("sceAudio3dReportRegisterHandler", [] { sceAudio3dReportRegisterHandler(); });
    RequireNotImplemented("sceAudio3dReportUnregisterHandler", [] { sceAudio3dReportUnregisterHandler(); });
}

}

int main() {
    CheckPorts();
    CheckErrorText();
    CheckBeds();
    CheckSpeakerArrays();
    CheckAudioOut();
    CheckReports();
}
