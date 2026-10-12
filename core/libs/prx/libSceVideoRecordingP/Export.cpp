#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <cstring>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr int SCE_VIDEO_RECORDING_ERROR_INVALID_ARGUMENT = static_cast<int>(0x80B60002);
constexpr int SCE_VIDEO_RECORDING_ERROR_NOT_OPENED = static_cast<int>(0x80B60003);
constexpr int SCE_VIDEO_RECORDING_ERROR_ALREADY_OPENED = static_cast<int>(0x80B60004);
constexpr int SCE_VIDEO_RECORDING_ERROR_NOT_STARTED = static_cast<int>(0x80B60005);
constexpr int SCE_VIDEO_RECORDING_ERROR_ALREADY_STARTED = static_cast<int>(0x80B60006);
constexpr int SCE_VIDEO_RECORDING_STATUS_NOT_STARTED = 0;
constexpr int SCE_VIDEO_RECORDING_STATUS_STARTED = 1;

std::mutex g_mutex;
std::map<int32_t, std::vector<unsigned char>> g_info;
bool g_opened = false;
bool g_started = false;

}

extern "C" {

int APS5_VABI sceVideoRecordingClose(int discard) {
    (void)discard;
    std::lock_guard lock(g_mutex);
    if (!g_opened) return SCE_VIDEO_RECORDING_ERROR_NOT_OPENED;
    g_started = false;
    g_opened = false;
    return 0;
}

int APS5_VABI sceVideoRecordingGetInfo(int32_t info, void* data, size_t size) {
    if (data == nullptr) APS5_INVALID_ARG_EX;
    std::lock_guard lock(g_mutex);
    const auto found = g_info.find(info);
    if (found == g_info.end()) throw std::logic_error(std::string(__func__) + ": info " + std::to_string(info) + " was never set");
    if (size < found->second.size()) APS5_INVALID_ARG_EX;
    std::memcpy(data, found->second.data(), found->second.size());
    return 0;
}

int APS5_VABI sceVideoRecordingGetStatus(void) {
    std::lock_guard lock(g_mutex);
    if (!g_opened) return SCE_VIDEO_RECORDING_ERROR_NOT_OPENED;
    return g_started ? SCE_VIDEO_RECORDING_STATUS_STARTED : SCE_VIDEO_RECORDING_STATUS_NOT_STARTED;
}

int APS5_VABI sceVideoRecordingOpen(const char* path, const void* param, void* heap, int heapSize) {
    (void)path;
    (void)param;
    (void)heap;
    (void)heapSize;
    std::lock_guard lock(g_mutex);
    if (g_opened) return SCE_VIDEO_RECORDING_ERROR_ALREADY_OPENED;
    g_opened = true;
    return 0;
}

int APS5_VABI sceVideoRecordingQueryMemSize(const void* param) {
    if (param == nullptr) return SCE_VIDEO_RECORDING_ERROR_INVALID_ARGUMENT;
    return 0;
}

int APS5_VABI sceVideoRecordingSetInfo(int32_t info, const void* data, size_t size) {
    if (data == nullptr && size != 0) APS5_INVALID_ARG_EX;
    const auto* bytes = static_cast<const unsigned char*>(data);
    std::lock_guard lock(g_mutex);
    g_info[info].assign(bytes, bytes + size);
    return 0;
}

int APS5_VABI sceVideoRecordingStart(void) {
    std::lock_guard lock(g_mutex);
    if (!g_opened) return SCE_VIDEO_RECORDING_ERROR_NOT_OPENED;
    if (g_started) return SCE_VIDEO_RECORDING_ERROR_ALREADY_STARTED;
    g_started = true;
    return 0;
}

int APS5_VABI sceVideoRecordingStop(void) {
    std::lock_guard lock(g_mutex);
    if (!g_opened) return SCE_VIDEO_RECORDING_ERROR_NOT_OPENED;
    if (!g_started) return SCE_VIDEO_RECORDING_ERROR_NOT_STARTED;
    g_started = false;
    return 0;
}

APS5_EXPORT("iQS6DUtLybE", videoRecordingUnknown_iQS6DUtLybE);
int APS5_VABI videoRecordingUnknown_iQS6DUtLybE(void) {
    throw std::runtime_error("videoRecordingUnknown_iQS6DUtLybE (iQS6DUtLybE): unknown function called");
}

}
