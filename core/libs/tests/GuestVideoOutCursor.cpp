#include "SceTypes.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include "tests/VideoOutTestEnvironment.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" {
int APS5_VABI sceVideoOutOpen(int userId, int busType, int index, const void* param);
int APS5_VABI sceVideoOutClose(int handle);
int APS5_VABI sceVideoOutCursorEnable(std::int32_t handle, std::int32_t index, const void* address);
int APS5_VABI sceVideoOutCursorDisable(std::int32_t handle, std::int32_t index);
int APS5_VABI sceVideoOutCursorSetImageAddress(std::int32_t handle, std::int32_t index, const void* address);
int APS5_VABI sceVideoOutCursorSetPosition(std::int32_t handle, std::int32_t index, std::uint32_t x, std::uint32_t y);
}

static constexpr int VIDEO_OUT_ERROR_INVALID_HANDLE = -2144796661;
static constexpr int VIDEO_OUT_ERROR_INVALID_INDEX = -2144796662;
static constexpr int VIDEO_OUT_ERROR_INVALID_ADDRESS = -2144796670;
static constexpr int SYSTEM_USER = 255;
static constexpr int MAIN_BUS = 0;
static constexpr int SKIP = 77;

static void Require(bool value) {
    if (!value) std::abort();
}

static const std::uint32_t image[64 * 64] = {};

int main() {
    VideoOutTestEnvironment environment;

    int handle;
    try {
        handle = sceVideoOutOpen(SYSTEM_USER, MAIN_BUS, 0, nullptr);
    } catch (const std::runtime_error&) {
        return SKIP;
    }
    Require(handle > 0);

    Require(sceVideoOutCursorEnable(0, 0, image) == VIDEO_OUT_ERROR_INVALID_HANDLE);
    Require(sceVideoOutCursorDisable(0, 0) == VIDEO_OUT_ERROR_INVALID_HANDLE);
    Require(sceVideoOutCursorSetImageAddress(0, 0, image) == VIDEO_OUT_ERROR_INVALID_HANDLE);
    Require(sceVideoOutCursorSetPosition(0, 0, 0, 0) == VIDEO_OUT_ERROR_INVALID_HANDLE);

    Require(sceVideoOutCursorEnable(handle, -1, image) == VIDEO_OUT_ERROR_INVALID_INDEX);
    Require(sceVideoOutCursorEnable(handle, 2, image) == VIDEO_OUT_ERROR_INVALID_INDEX);
    Require(sceVideoOutCursorDisable(handle, -1) == VIDEO_OUT_ERROR_INVALID_INDEX);
    Require(sceVideoOutCursorSetImageAddress(handle, 2, image) == VIDEO_OUT_ERROR_INVALID_INDEX);
    Require(sceVideoOutCursorSetPosition(handle, 2, 0, 0) == VIDEO_OUT_ERROR_INVALID_INDEX);

    Require(sceVideoOutCursorEnable(handle, 0, nullptr) == VIDEO_OUT_ERROR_INVALID_ADDRESS);
    Require(sceVideoOutCursorSetImageAddress(handle, 0, nullptr) == VIDEO_OUT_ERROR_INVALID_ADDRESS);

    Require(sceVideoOutCursorEnable(handle, 0, image) == 0);
    Require(sceVideoOutCursorEnable(handle, 1, image) == 0);
    Require(sceVideoOutCursorSetImageAddress(handle, 0, image) == 0);
    Require(sceVideoOutCursorSetPosition(handle, 0, 960, 540) == 0);
    Require(sceVideoOutCursorSetPosition(handle, 1, 0, 0) == 0);
    Require(sceVideoOutCursorDisable(handle, 0) == 0);
    Require(sceVideoOutCursorDisable(handle, 1) == 0);

    sceVideoOutClose(handle);
    return 0;
}

    Require(sceVideoOutCursorEnable(0, 0, image) == VIDEO_OUT_ERROR_INVALID_HANDLE);
    Require(sceVideoOutCursorDisable(0, 0) == VIDEO_OUT_ERROR_INVALID_HANDLE);
    Require(sceVideoOutCursorSetImageAddress(0, 0, image) == VIDEO_OUT_ERROR_INVALID_HANDLE);
    Require(sceVideoOutCursorSetPosition(0, 0, 0, 0) == VIDEO_OUT_ERROR_INVALID_HANDLE);

    Require(sceVideoOutCursorEnable(handle, -1, image) == VIDEO_OUT_ERROR_INVALID_INDEX);
    Require(sceVideoOutCursorEnable(handle, 2, image) == VIDEO_OUT_ERROR_INVALID_INDEX);
    Require(sceVideoOutCursorDisable(handle, -1) == VIDEO_OUT_ERROR_INVALID_INDEX);
    Require(sceVideoOutCursorSetImageAddress(handle, 2, image) == VIDEO_OUT_ERROR_INVALID_INDEX);
    Require(sceVideoOutCursorSetPosition(handle, 2, 0, 0) == VIDEO_OUT_ERROR_INVALID_INDEX);

    Require(sceVideoOutCursorEnable(handle, 0, nullptr) == VIDEO_OUT_ERROR_INVALID_ADDRESS);
    Require(sceVideoOutCursorSetImageAddress(handle, 0, nullptr) == VIDEO_OUT_ERROR_INVALID_ADDRESS);

    Require(sceVideoOutCursorEnable(handle, 0, image) == 0);
    Require(sceVideoOutCursorEnable(handle, 1, image) == 0);
    Require(sceVideoOutCursorSetImageAddress(handle, 0, image) == 0);
    Require(sceVideoOutCursorSetPosition(handle, 0, 960, 540) == 0);
    Require(sceVideoOutCursorSetPosition(handle, 1, 0, 0) == 0);
    Require(sceVideoOutCursorDisable(handle, 0) == 0);
    Require(sceVideoOutCursorDisable(handle, 1) == 0);

    sceVideoOutClose(handle);
    return 0;
}

