#include "prx/libc/include/general/VabiMacros.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <vector>

extern "C" {
void APS5_VABI _sceVideoRecordingQueryParam_nid_postfix(void* param);
int APS5_VABI sceVideoRecordingGetStatus(void);
int APS5_VABI sceVideoRecordingQueryMemSize(const void* param);
int APS5_VABI sceVideoRecordingOpen(const char* path, const void* param, void* heap, int heapSize);
int APS5_VABI sceVideoRecordingStart(void);
int APS5_VABI sceVideoRecordingStop(void);
int APS5_VABI sceVideoRecordingClose(int discard);
int APS5_VABI sceVideoRecordingCopyBGRAtoNV12(void* dst, const void* src, uint32_t width, uint32_t height);
}

namespace {

constexpr int StatusNone = 0;
constexpr std::uint8_t Sentinel = 0xA5;

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "VideoRecording: %s\n", message);
        std::abort();
    }
}

template <typename TCall>
bool Throws(TCall call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

void TestCopyBGRAtoNV12InvalidArguments() {
    std::uint8_t src[16] = {};
    std::uint8_t dst[24] = {};

    Require(Throws([&] { sceVideoRecordingCopyBGRAtoNV12(nullptr, src, 2, 2); }), "null dst did not throw");
    Require(Throws([&] { sceVideoRecordingCopyBGRAtoNV12(dst, nullptr, 2, 2); }), "null src did not throw");
    Require(Throws([&] { sceVideoRecordingCopyBGRAtoNV12(dst, src, 0, 2); }), "zero width did not throw");
    Require(Throws([&] { sceVideoRecordingCopyBGRAtoNV12(dst, src, 2, 0); }), "zero height did not throw");
    Require(Throws([&] { sceVideoRecordingCopyBGRAtoNV12(dst, src, 1, 2); }), "odd width did not throw");
    Require(Throws([&] { sceVideoRecordingCopyBGRAtoNV12(dst, src, 3, 2); }), "odd width did not throw");
    Require(Throws([&] { sceVideoRecordingCopyBGRAtoNV12(dst, src, 2, 1); }), "odd height did not throw");
    Require(Throws([&] { sceVideoRecordingCopyBGRAtoNV12(dst, src, 2, 3); }), "odd height did not throw");
}

void TestCopyBGRAtoNV12Canary() {
    constexpr std::uint32_t width = 4;
    constexpr std::uint32_t height = 4;
    constexpr std::size_t nv12Size = (width * height * 3) / 2;
    constexpr std::size_t canarySize = 32;
    constexpr std::uint8_t canaryVal = 0xAA;

    std::vector<std::uint8_t> src(width * height * 4, 0);
    std::vector<std::uint8_t> dstWithCanary(canarySize + nv12Size + canarySize, canaryVal);

    std::uint8_t* actualDst = dstWithCanary.data() + canarySize;
    const int res = sceVideoRecordingCopyBGRAtoNV12(actualDst, src.data(), width, height);
    Require(res == 0, "copy returned error");

    for (std::size_t i = 0; i < canarySize; ++i) {
        Require(dstWithCanary[i] == canaryVal, "leading canary was overwritten");
    }
    for (std::size_t i = canarySize + nv12Size; i < dstWithCanary.size(); ++i) {
        Require(dstWithCanary[i] == canaryVal, "trailing canary was overwritten");
    }
}

void TestCopyBGRAtoNV12Colors() {
    constexpr std::uint32_t width = 2;
    constexpr std::uint32_t height = 2;
    constexpr std::size_t nv12Size = (width * height * 3) / 2;

    auto testColor = [](std::uint8_t b, std::uint8_t g, std::uint8_t r, std::uint8_t a,
                        std::uint8_t expY, std::uint8_t expU, std::uint8_t expV) {
        std::uint8_t src[16];
        for (int i = 0; i < 4; ++i) {
            src[i * 4] = b;
            src[i * 4 + 1] = g;
            src[i * 4 + 2] = r;
            src[i * 4 + 3] = a;
        }
        std::uint8_t dst[nv12Size] = {};
        const int res = sceVideoRecordingCopyBGRAtoNV12(dst, src, width, height);
        Require(res == 0, "conversion failed");

        for (int i = 0; i < 4; ++i) {
            Require(dst[i] == expY, "mismatched Y sample");
        }
        Require(dst[4] == expU, "mismatched U sample");
        Require(dst[5] == expV, "mismatched V sample");
    };

    testColor(0, 0, 0, 255, 16, 128, 128);
    testColor(255, 255, 255, 255, 235, 128, 128);
    testColor(0, 0, 255, 255, 82, 90, 240);
    testColor(0, 255, 0, 255, 144, 54, 34);
    testColor(255, 0, 0, 255, 41, 240, 110);
}

void TestCopyBGRAtoNV12Quadrant() {
    constexpr std::uint32_t width = 4;
    constexpr std::uint32_t height = 4;
    constexpr std::size_t nv12Size = (width * height * 3) / 2;

    std::uint8_t src[width * height * 4] = {};

    auto setPixel = [&](std::uint32_t x, std::uint32_t y, std::uint8_t b, std::uint8_t g, std::uint8_t r) {
        const std::size_t offset = (y * width + x) * 4;
        src[offset] = b;
        src[offset + 1] = g;
        src[offset + 2] = r;
        src[offset + 3] = 255;
    };

    for (std::uint32_t y = 0; y < 2; ++y) {
        for (std::uint32_t x = 0; x < 2; ++x) setPixel(x, y, 0, 0, 255);
        for (std::uint32_t x = 2; x < 4; ++x) setPixel(x, y, 0, 255, 0);
    }
    for (std::uint32_t y = 2; y < 4; ++y) {
        for (std::uint32_t x = 0; x < 2; ++x) setPixel(x, y, 255, 0, 0);
        for (std::uint32_t x = 2; x < 4; ++x) setPixel(x, y, 255, 255, 255);
    }

    std::uint8_t dst[nv12Size] = {};
    const int res = sceVideoRecordingCopyBGRAtoNV12(dst, src, width, height);
    Require(res == 0, "quadrant conversion failed");

    for (std::uint32_t y = 0; y < 2; ++y) {
        Require(dst[y * width + 0] == 82, "quad 0 Y mismatch");
        Require(dst[y * width + 1] == 82, "quad 0 Y mismatch");
        Require(dst[y * width + 2] == 144, "quad 1 Y mismatch");
        Require(dst[y * width + 3] == 144, "quad 1 Y mismatch");
    }
    for (std::uint32_t y = 2; y < 4; ++y) {
        Require(dst[y * width + 0] == 41, "quad 2 Y mismatch");
        Require(dst[y * width + 1] == 41, "quad 2 Y mismatch");
        Require(dst[y * width + 2] == 235, "quad 3 Y mismatch");
        Require(dst[y * width + 3] == 235, "quad 3 Y mismatch");
    }

    const std::uint8_t* uv = dst + 16;
    Require(uv[0] == 90, "quad 0 U mismatch");
    Require(uv[1] == 240, "quad 0 V mismatch");
    Require(uv[2] == 54, "quad 1 U mismatch");
    Require(uv[3] == 34, "quad 1 V mismatch");

    Require(uv[4] == 240, "quad 2 U mismatch");
    Require(uv[5] == 110, "quad 2 V mismatch");
    Require(uv[6] == 128, "quad 3 U mismatch");
    Require(uv[7] == 128, "quad 3 V mismatch");
}

void TestCopyBGRAtoNV12Resolutions() {
    constexpr std::pair<std::uint32_t, std::uint32_t> kResolutions[] = {
        {16, 16},
        {128, 128},
        {1280, 720},
        {1920, 1080},
        {3840, 2160}
    };
    for (const auto& [w, h] : kResolutions) {
        std::vector<std::uint8_t> src(static_cast<std::size_t>(w) * h * 4, 128);
        std::vector<std::uint8_t> dst((static_cast<std::size_t>(w) * h * 3) / 2, 0);
        const int res = sceVideoRecordingCopyBGRAtoNV12(dst.data(), src.data(), w, h);
        Require(res == 0, "resolution test failed");
        Require(dst[0] != 0, "first luma byte untouched");
        Require(dst[dst.size() - 1] != 0, "last chroma byte untouched");
    }
}

}

int main() {
    Require(sceVideoRecordingGetStatus() == StatusNone, "a recorder that was never opened reported a status");
    Require(sceVideoRecordingGetStatus() == StatusNone, "the status changed between two reads");

    std::array<std::uint8_t, 24> param{};
    param.fill(Sentinel);
    std::array<std::uint8_t, 64> heap{};
    heap.fill(Sentinel);

    Require(Throws([&] { _sceVideoRecordingQueryParam_nid_postfix(param.data()); }), "the default parameters were produced");
    Require(Throws([&] { sceVideoRecordingQueryMemSize(param.data()); }), "a heap size was reported");
    Require(Throws([&] { sceVideoRecordingQueryMemSize(nullptr); }), "a heap size was reported for no parameters");
    Require(Throws([&] { sceVideoRecordingOpen("clip.mp4", param.data(), heap.data(), static_cast<int>(heap.size())); }), "the recorder opened");
    Require(Throws([&] { sceVideoRecordingStart(); }), "the encoder started");
    Require(Throws([&] { sceVideoRecordingStop(); }), "the encoder stopped");
    Require(Throws([&] { sceVideoRecordingClose(0); }), "the recorder closed");
    Require(Throws([&] { sceVideoRecordingClose(1); }), "the recorder closed with discard");

    for (const std::uint8_t value : param) Require(value == Sentinel, "the parameters were written");
    for (const std::uint8_t value : heap) Require(value == Sentinel, "the heap was written");
    Require(sceVideoRecordingGetStatus() == StatusNone, "a failed open changed the status");

    TestCopyBGRAtoNV12InvalidArguments();
    TestCopyBGRAtoNV12Canary();
    TestCopyBGRAtoNV12Colors();
    TestCopyBGRAtoNV12Quadrant();
    TestCopyBGRAtoNV12Resolutions();

    std::puts("VideoRecording tests passed");
    return 0;
}
