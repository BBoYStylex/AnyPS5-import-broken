#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <vector>

extern "C" {
int APS5_VABI sceVideoRecordingQueryMemSize(const void* param);
int APS5_VABI sceVideoRecordingOpen(const char* path, const void* param, void* heap, int heapSize);
int APS5_VABI sceVideoRecordingStart(void);
int APS5_VABI sceVideoRecordingStop(void);
int APS5_VABI sceVideoRecordingClose(int discard);
int APS5_VABI sceVideoRecordingGetStatus(void);
}

static void Require(bool value) { if (!value) std::abort(); }

template<typename TException, typename TCall>
static void RequireThrows(TCall call) {
    try {
        call();
    } catch (const TException&) {
        return;
    }
    std::abort();
}

int main() {
    const char param[64]{};
    RequireThrows<std::invalid_argument>([] { sceVideoRecordingQueryMemSize(nullptr); });
    const int heapSize = sceVideoRecordingQueryMemSize(param);
    Require(heapSize > 0);
    Require(sceVideoRecordingGetStatus() == 0);

    std::vector<unsigned char> heap(static_cast<std::size_t>(heapSize));
    RequireThrows<std::logic_error>([&] { sceVideoRecordingStart(); });
    Require(sceVideoRecordingOpen("/app0/clip.mp4", param, heap.data(), heapSize) == 0);
    Require(sceVideoRecordingGetStatus() == 1);
    RequireThrows<std::logic_error>([&] {
        sceVideoRecordingOpen("/app0/clip2.mp4", param, heap.data(), heapSize);
    });
    Require(sceVideoRecordingStart() == 0);
    Require(sceVideoRecordingGetStatus() == 2);
    RequireThrows<std::logic_error>([] { sceVideoRecordingStart(); });
    Require(sceVideoRecordingStop() == 0);
    Require(sceVideoRecordingGetStatus() == 1);
    Require(sceVideoRecordingClose(0) == 0);
    Require(sceVideoRecordingGetStatus() == 0);
    RequireThrows<std::logic_error>([] { sceVideoRecordingClose(0); });
    return 0;
}
