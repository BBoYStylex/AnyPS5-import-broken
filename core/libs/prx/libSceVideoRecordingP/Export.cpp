#include <algorithm>
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
std::mutex g_infoMutex;
std::map<int32_t, std::vector<unsigned char>> g_info;
constexpr int SceVideoRecordingStatusNone = 0;
}

extern "C" {

void APS5_VABI _sceVideoRecordingQueryParam_nid_postfix(void* param) {
 (void)param;
 NotImplemented_nid_no_patch("_sceVideoRecordingQueryParam");
}

int APS5_VABI sceVideoRecordingClose(int discard) {
 (void)discard;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingCloseByHandle(int handle, int discard) {
 (void)handle;
 (void)discard;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingCopyBGRAtoNV12(void* dst, const void* src, uint32_t width, uint32_t height) {
    if (dst == nullptr || src == nullptr || width == 0 || height == 0 || (width % 2) != 0 || (height % 2) != 0) {
        APS5_INVALID_ARG_EX;
    }
    const auto* srcBytes = static_cast<const uint8_t*>(src);
    auto* dstY = static_cast<uint8_t*>(dst);
    auto* dstUV = dstY + (static_cast<size_t>(width) * height);
    const size_t srcStride = static_cast<size_t>(width) * 4;

    for (uint32_t y = 0; y < height; y += 2) {
        const auto* row0 = srcBytes + (static_cast<size_t>(y) * srcStride);
        const auto* row1 = row0 + srcStride;
        auto* yOut0 = dstY + (static_cast<size_t>(y) * width);
        auto* yOut1 = yOut0 + width;
        auto* uvOut = dstUV + (static_cast<size_t>(y / 2) * width);

        for (uint32_t x = 0; x < width; x += 2) {
            const size_t px00 = static_cast<size_t>(x) * 4;
            const size_t px01 = px00 + 4;

            const int32_t b00 = row0[px00];
            const int32_t g00 = row0[px00 + 1];
            const int32_t r00 = row0[px00 + 2];

            const int32_t b01 = row0[px01];
            const int32_t g01 = row0[px01 + 1];
            const int32_t r01 = row0[px01 + 2];

            const int32_t b10 = row1[px00];
            const int32_t g10 = row1[px00 + 1];
            const int32_t r10 = row1[px00 + 2];

            const int32_t b11 = row1[px01];
            const int32_t g11 = row1[px01 + 1];
            const int32_t r11 = row1[px01 + 2];

            const int32_t y00 = ((66 * r00 + 129 * g00 + 25 * b00 + 128) >> 8) + 16;
            const int32_t y01 = ((66 * r01 + 129 * g01 + 25 * b01 + 128) >> 8) + 16;
            const int32_t y10 = ((66 * r10 + 129 * g10 + 25 * b10 + 128) >> 8) + 16;
            const int32_t y11 = ((66 * r11 + 129 * g11 + 25 * b11 + 128) >> 8) + 16;

            yOut0[x] = static_cast<uint8_t>(std::clamp(y00, 0, 255));
            yOut0[x + 1] = static_cast<uint8_t>(std::clamp(y01, 0, 255));
            yOut1[x] = static_cast<uint8_t>(std::clamp(y10, 0, 255));
            yOut1[x + 1] = static_cast<uint8_t>(std::clamp(y11, 0, 255));

            const int32_t rAvg = (r00 + r01 + r10 + r11 + 2) >> 2;
            const int32_t gAvg = (g00 + g01 + g10 + g11 + 2) >> 2;
            const int32_t bAvg = (b00 + b01 + b10 + b11 + 2) >> 2;

            const int32_t u = ((-38 * rAvg - 74 * gAvg + 112 * bAvg + 128) >> 8) + 128;
            const int32_t v = ((112 * rAvg - 94 * gAvg - 18 * bAvg + 128) >> 8) + 128;

            uvOut[x] = static_cast<uint8_t>(std::clamp(u, 0, 255));
            uvOut[x + 1] = static_cast<uint8_t>(std::clamp(v, 0, 255));
        }
    }
    return 0;
}

int APS5_VABI sceVideoRecordingGetInfo(int32_t info, void* data, size_t size) {
    if (data == nullptr) APS5_INVALID_ARG_EX;
    std::lock_guard lock(g_infoMutex);
    const auto found = g_info.find(info);
    if (found == g_info.end()) throw std::logic_error(std::string(__func__) + ": info " + std::to_string(info) + " was never set");
    if (size < found->second.size()) APS5_INVALID_ARG_EX;
    std::memcpy(data, found->second.data(), found->second.size());
    return 0;
}

int APS5_VABI sceVideoRecordingGetInfoByHandle(int handle, int32_t info, void* data, size_t size) {
 (void)handle;
 (void)info;
 (void)data;
 (void)size;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingGetStatus(void) {
 return SceVideoRecordingStatusNone;
}

int APS5_VABI sceVideoRecordingGetSupportInfo(void* info) {
 (void)info;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingGetVideoOutMode(void* mode) {
 (void)mode;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingInit(void) {
 return 0;
}

int APS5_VABI sceVideoRecordingInitialize(void* param) {
 (void)param;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingOpen(const char* path, const void* param, void* heap, int heapSize) {
 (void)path;
 (void)param;
 (void)heap;
 (void)heapSize;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingOpen2(const char* path, const void* param, void* heap, int heapSize) {
 (void)path;
 (void)param;
 (void)heap;
 (void)heapSize;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingQueryHddSize2(const void* param) {
 (void)param;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingQueryMemSize(const void* param) {
 (void)param;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingQueryMemSize2(const void* param) {
 (void)param;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingRestModeProcessGetStatus(void* status) {
 (void)status;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingRestModeProcessStart(void* param) {
 (void)param;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingSendCommand(int cmd, const void* param) {
 (void)cmd;
 (void)param;
 return 0;
}

int APS5_VABI sceVideoRecordingSetInfo(int32_t info, const void* data, size_t size) {
    if (data == nullptr && size != 0) APS5_INVALID_ARG_EX;
    const auto* bytes = static_cast<const unsigned char*>(data);
    std::lock_guard lock(g_infoMutex);
    g_info[info].assign(bytes, bytes + size);
    return 0;
}

int APS5_VABI sceVideoRecordingStart(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingStop(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingTerm(void) {
 return 0;
}

APS5_EXPORT("iQS6DUtLybE", videoRecordingUnknown_iQS6DUtLybE);
int APS5_VABI videoRecordingUnknown_iQS6DUtLybE(void) {
 NotImplemented_nid_no_patch("iQS6DUtLybE");
 return 0;
}

}
