#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

extern "C" {
int APS5_VABI sceVideoRecordingGetStatus(void);
int APS5_VABI sceVideoRecordingQueryMemSize(const void* param);
int APS5_VABI sceVideoRecordingOpen(const char* path, const void* param, void* heap, int heapSize);
int APS5_VABI sceVideoRecordingStart(void);
int APS5_VABI sceVideoRecordingStop(void);
int APS5_VABI sceVideoRecordingClose(int discard);
int APS5_VABI sceVideoRecordingGetInfo(std::int32_t info, void* data, std::size_t size);
int APS5_VABI sceVideoRecordingSetInfo(std::int32_t info, const void* data, std::size_t size);
}

namespace {

void Require(bool value) {
    if (!value) std::abort();
}

template <typename Function>
void RequireThrows(Function function) {
    try {
        function();
    } catch (const std::logic_error&) {
        return;
    } catch (const std::runtime_error&) {
        return;
    }
    std::abort();
}

}

int main() {
    RequireThrows([] { sceVideoRecordingGetStatus(); });
    RequireThrows([] { sceVideoRecordingQueryMemSize(nullptr); });
    RequireThrows([] { sceVideoRecordingOpen(nullptr, nullptr, nullptr, 0); });
    RequireThrows([] { sceVideoRecordingStart(); });
    RequireThrows([] { sceVideoRecordingStop(); });
    RequireThrows([] { sceVideoRecordingClose(0); });
    RequireThrows([] { sceVideoRecordingClose(1); });

    const std::uint8_t blob[] = {1, 2, 3, 4};
    Require(sceVideoRecordingSetInfo(7, blob, sizeof(blob)) == 0);
    std::uint8_t read[4]{};
    Require(sceVideoRecordingGetInfo(7, read, sizeof(read)) == 0 && std::memcmp(read, blob, sizeof(blob)) == 0);
    RequireThrows([] { std::uint8_t small[3]{}; sceVideoRecordingGetInfo(7, small, sizeof(small)); });
    RequireThrows([] { std::uint8_t out[4]{}; sceVideoRecordingGetInfo(8, out, sizeof(out)); });
    RequireThrows([] { sceVideoRecordingGetInfo(7, nullptr, 4); });
}
