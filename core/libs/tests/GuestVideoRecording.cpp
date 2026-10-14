#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

extern "C" {
int APS5_VABI sceVideoRecordingGetStatus(void);
int APS5_VABI sceVideoRecordingQueryMemSize(const void* param);
int APS5_VABI sceVideoRecordingOpen(const char* path, const void* param, void* heap, int heapSize);
int APS5_VABI sceVideoRecordingStart(void);
int APS5_VABI sceVideoRecordingStop(void);
int APS5_VABI sceVideoRecordingClose(int discard);
}

namespace {

struct VideoRecordingParam {
    std::size_t size;
    std::int32_t ringSec;
    std::int32_t format;
};

void Require(bool value) { if (!value) std::abort(); }

template <typename TException, typename TAction>
bool Throws(TAction action) {
    try {
        action();
    } catch (const TException&) {
        return true;
    }
    return false;
}

}

int main() {
    VideoRecordingParam param{sizeof(VideoRecordingParam), 15 * 60, 0};
    Require(sceVideoRecordingGetStatus() == 0);
    Require(sceVideoRecordingQueryMemSize(&param) == 4 * 1024);
    Require(Throws<std::invalid_argument>([] { sceVideoRecordingQueryMemSize(nullptr); }));

    alignas(16) unsigned char heap[4 * 1024];
    Require(Throws<std::invalid_argument>([&] { sceVideoRecordingOpen(nullptr, &param, heap, sizeof(heap)); }));
    Require(Throws<std::invalid_argument>([&] { sceVideoRecordingOpen("/data/clip.mp4", nullptr, heap, sizeof(heap)); }));
    Require(Throws<std::runtime_error>([&] { sceVideoRecordingOpen("/data/clip.mp4", &param, heap, sizeof(heap)); }));
    Require(sceVideoRecordingGetStatus() == 0);

    Require(Throws<std::logic_error>([] { sceVideoRecordingStart(); }));
    Require(Throws<std::logic_error>([] { sceVideoRecordingStop(); }));
    Require(Throws<std::logic_error>([] { sceVideoRecordingClose(0); }));
    Require(Throws<std::logic_error>([] { sceVideoRecordingClose(1); }));
}
