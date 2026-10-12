#include "prx/libc/include/general/VabiMacros.hpp"
#include "SceTypes.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" {
std::int32_t APS5_VABI sceSystemGestureOpen(std::int32_t, const void*);
int APS5_VABI sceSystemGestureGetPrimitiveTouchEvents(std::int32_t, void*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceSystemGestureGetPrimitiveTouchEventsCount(std::int32_t);
int APS5_VABI sceSystemGestureGetTouchEvents(std::int32_t, const void*, void*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceSystemGestureGetPrimitiveTouchEventByIndex(std::int32_t, std::uint32_t, void*);
int APS5_VABI sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(std::int32_t, std::uint16_t, SystemGesturePrimitiveTouchEvent*);
int APS5_VABI sceSystemGestureGetTouchEventByEventID(std::int32_t, const SystemGestureTouchRecognizer*, std::uint32_t, SystemGestureTouchEvent*);
int APS5_VABI sceSystemGestureGetTouchRecognizerInformation(std::int32_t, const SystemGestureTouchRecognizer*, SystemGestureTouchRecognizerInformation*);
int APS5_VABI sceSystemGestureAppendTouchRecognizer(std::int32_t, void*);
int APS5_VABI sceSystemGestureRemoveTouchRecognizer(std::int32_t, void*);
int APS5_VABI sceSystemGestureResetTouchRecognizer(std::int32_t, void*);
int APS5_VABI sceSystemGestureUpdateTouchRecognizer(std::int32_t, void*);
int APS5_VABI sceSystemGestureUpdateTouchRecognizerRectangle(std::int32_t, void*, const void*);
int APS5_VABI sceSystemGestureResetPrimitiveTouchRecognizer(std::int32_t);
int APS5_VABI sceSystemGestureUpdateAllTouchRecognizer(std::int32_t);
int APS5_VABI sceSystemGestureUpdatePrimitiveTouchRecognizer(std::int32_t, const void*);
}

static void Require(bool value) { if (!value) std::abort(); }

int main() {
    constexpr int invalidArgument = static_cast<int>(0x80D10002);
    constexpr int invalidHandle = static_cast<int>(0x80D10003);
    const std::int32_t handle = sceSystemGestureOpen(0, nullptr);

    std::array<unsigned char, 64> buffer;
    buffer.fill(0x5a);
    const auto original = buffer;
    std::uint32_t count = 7;
    Require(sceSystemGestureGetPrimitiveTouchEvents(handle, buffer.data(), 4, &count) == 0);
    Require(count == 0);
    Require(buffer == original);

    count = 7;
    Require(sceSystemGestureGetPrimitiveTouchEvents(handle, buffer.data(), 4, nullptr) == invalidArgument);
    Require(sceSystemGestureGetPrimitiveTouchEvents(handle + 1, buffer.data(), 4, &count) == invalidHandle);
    Require(count == 7);

    Require(sceSystemGestureGetPrimitiveTouchEventsCount(handle) == 0);
    Require(sceSystemGestureGetPrimitiveTouchEventsCount(handle + 1) == invalidHandle);

    constexpr int indexOutOfArray = static_cast<int>(0x80D10005);
    int recognizer = 0;
    int rectangle = 0;
    count = 7;
    Require(sceSystemGestureGetTouchEvents(handle, &recognizer, buffer.data(), 4, &count) == 0 && count == 0);
    Require(sceSystemGestureGetTouchEvents(handle, &recognizer, buffer.data(), 4, nullptr) == invalidArgument);
    Require(sceSystemGestureGetTouchEvents(handle + 1, &recognizer, buffer.data(), 4, &count) == invalidHandle);
    Require(sceSystemGestureGetPrimitiveTouchEventByIndex(handle, 0, buffer.data()) == indexOutOfArray);
    Require(sceSystemGestureGetPrimitiveTouchEventByIndex(handle + 1, 0, buffer.data()) == invalidHandle);
    Require(buffer == original);

    Require(sceSystemGestureAppendTouchRecognizer(handle, &recognizer) == 0);
    Require(sceSystemGestureRemoveTouchRecognizer(handle, &recognizer) == 0);
    Require(sceSystemGestureResetTouchRecognizer(handle, &recognizer) == 0);
    Require(sceSystemGestureUpdateTouchRecognizer(handle, &recognizer) == 0);
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, &recognizer, &rectangle) == 0);
    Require(sceSystemGestureResetPrimitiveTouchRecognizer(handle) == 0);
    Require(sceSystemGestureUpdateAllTouchRecognizer(handle) == 0);
    Require(sceSystemGestureUpdatePrimitiveTouchRecognizer(handle, nullptr) == 0);
    Require(sceSystemGestureAppendTouchRecognizer(handle, nullptr) == invalidArgument);
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, &recognizer, nullptr) == invalidArgument);
    Require(sceSystemGestureAppendTouchRecognizer(handle + 1, &recognizer) == invalidHandle);
    Require(sceSystemGestureRemoveTouchRecognizer(handle, nullptr) == invalidArgument);
    Require(sceSystemGestureResetTouchRecognizer(handle, nullptr) == invalidArgument);
    Require(sceSystemGestureUpdateTouchRecognizer(handle, nullptr) == invalidArgument);
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, nullptr, &rectangle) == invalidArgument);
    Require(sceSystemGestureRemoveTouchRecognizer(handle + 1, &recognizer) == invalidHandle);
    Require(sceSystemGestureResetTouchRecognizer(handle + 1, &recognizer) == invalidHandle);
    Require(sceSystemGestureUpdateTouchRecognizer(handle + 1, &recognizer) == invalidHandle);
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle + 1, &recognizer, &rectangle) == invalidHandle);
    Require(sceSystemGestureResetPrimitiveTouchRecognizer(handle + 1) == invalidHandle);
    Require(sceSystemGestureUpdateAllTouchRecognizer(handle + 1) == invalidHandle);
    Require(sceSystemGestureUpdatePrimitiveTouchRecognizer(handle + 1, nullptr) == invalidHandle);

    SystemGestureTouchRecognizer touchRecognizer{};
    SystemGesturePrimitiveTouchEvent primitiveEvent{};
    std::memset(&primitiveEvent, 0x5a, sizeof(primitiveEvent));
    primitiveEvent.primitive_id = 42;
    const auto origPrimitive = primitiveEvent;
    Require(sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(handle, 0, &primitiveEvent) == indexOutOfArray);
    Require(std::memcmp(&primitiveEvent, &origPrimitive, sizeof(primitiveEvent)) == 0);
    Require(sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(handle, 12, &primitiveEvent) == indexOutOfArray);
    Require(std::memcmp(&primitiveEvent, &origPrimitive, sizeof(primitiveEvent)) == 0);
    Require(sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(handle, 0xffff, &primitiveEvent) == indexOutOfArray);
    Require(std::memcmp(&primitiveEvent, &origPrimitive, sizeof(primitiveEvent)) == 0);
    Require(sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(handle, 0, nullptr) == invalidArgument);
    Require(sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(handle + 1, 0, &primitiveEvent) == invalidHandle);
    Require(std::memcmp(&primitiveEvent, &origPrimitive, sizeof(primitiveEvent)) == 0);
    Require(sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(handle + 1, 0, nullptr) == invalidHandle);
    Require(sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(-1, 0, &primitiveEvent) == invalidHandle);
    Require(std::memcmp(&primitiveEvent, &origPrimitive, sizeof(primitiveEvent)) == 0);
    Require(sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(0, 0, &primitiveEvent) == invalidHandle);
    Require(std::memcmp(&primitiveEvent, &origPrimitive, sizeof(primitiveEvent)) == 0);

    SystemGestureTouchEvent touchEvent{};
    std::memset(&touchEvent, 0xa5, sizeof(touchEvent));
    const auto origTouch = touchEvent;
    Require(sceSystemGestureGetTouchEventByEventID(handle, &touchRecognizer, 0, &touchEvent) == indexOutOfArray);
    Require(std::memcmp(&touchEvent, &origTouch, sizeof(touchEvent)) == 0);
    Require(sceSystemGestureGetTouchEventByEventID(handle, &touchRecognizer, 100, &touchEvent) == indexOutOfArray);
    Require(std::memcmp(&touchEvent, &origTouch, sizeof(touchEvent)) == 0);
    Require(sceSystemGestureGetTouchEventByEventID(handle, &touchRecognizer, 0xffffffff, &touchEvent) == indexOutOfArray);
    Require(std::memcmp(&touchEvent, &origTouch, sizeof(touchEvent)) == 0);
    Require(sceSystemGestureGetTouchEventByEventID(handle, nullptr, 0, &touchEvent) == invalidArgument);
    Require(std::memcmp(&touchEvent, &origTouch, sizeof(touchEvent)) == 0);
    Require(sceSystemGestureGetTouchEventByEventID(handle, &touchRecognizer, 0, nullptr) == invalidArgument);
    Require(sceSystemGestureGetTouchEventByEventID(handle, nullptr, 0, nullptr) == invalidArgument);
    Require(sceSystemGestureGetTouchEventByEventID(handle + 1, &touchRecognizer, 0, &touchEvent) == invalidHandle);
    Require(std::memcmp(&touchEvent, &origTouch, sizeof(touchEvent)) == 0);
    Require(sceSystemGestureGetTouchEventByEventID(handle + 1, nullptr, 0, &touchEvent) == invalidHandle);
    Require(std::memcmp(&touchEvent, &origTouch, sizeof(touchEvent)) == 0);
    Require(sceSystemGestureGetTouchEventByEventID(handle + 1, &touchRecognizer, 0, nullptr) == invalidHandle);
    Require(sceSystemGestureGetTouchEventByEventID(handle + 1, nullptr, 0, nullptr) == invalidHandle);
    Require(sceSystemGestureGetTouchEventByEventID(-1, &touchRecognizer, 0, &touchEvent) == invalidHandle);
    Require(std::memcmp(&touchEvent, &origTouch, sizeof(touchEvent)) == 0);
    Require(sceSystemGestureGetTouchEventByEventID(0, &touchRecognizer, 0, &touchEvent) == invalidHandle);
    Require(std::memcmp(&touchEvent, &origTouch, sizeof(touchEvent)) == 0);

    SystemGestureTouchRecognizerInformation failInfo{};
    std::memset(&failInfo, 0x3c, sizeof(failInfo));
    const auto origFailInfo = failInfo;
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, nullptr, &failInfo) == invalidArgument);
    Require(std::memcmp(&failInfo, &origFailInfo, sizeof(failInfo)) == 0);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, &touchRecognizer, nullptr) == invalidArgument);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, nullptr, nullptr) == invalidArgument);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle + 1, &touchRecognizer, &failInfo) == invalidHandle);
    Require(std::memcmp(&failInfo, &origFailInfo, sizeof(failInfo)) == 0);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle + 1, nullptr, &failInfo) == invalidHandle);
    Require(std::memcmp(&failInfo, &origFailInfo, sizeof(failInfo)) == 0);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle + 1, &touchRecognizer, nullptr) == invalidHandle);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle + 1, nullptr, nullptr) == invalidHandle);
    Require(sceSystemGestureGetTouchRecognizerInformation(-1, &touchRecognizer, &failInfo) == invalidHandle);
    Require(std::memcmp(&failInfo, &origFailInfo, sizeof(failInfo)) == 0);
    Require(sceSystemGestureGetTouchRecognizerInformation(0, &touchRecognizer, &failInfo) == invalidHandle);
    Require(std::memcmp(&failInfo, &origFailInfo, sizeof(failInfo)) == 0);

    SystemGestureTouchRecognizerInformation info{};
    info.gesture_type = 7;
    info.rectangle.x = 10.0f;
    info.rectangle.y = 20.0f;
    info.rectangle.width = 30.0f;
    info.rectangle.height = 40.0f;
    info.updated_time = 999;
    info.reserve[0] = 0xab;
    info.reserve[255] = 0xcd;
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, &touchRecognizer, &info) == 0);
    Require(info.gesture_type == 0);
    Require(info.rectangle.x == 0.0f);
    Require(info.rectangle.y == 0.0f);
    Require(info.rectangle.width == 0.0f);
    Require(info.rectangle.height == 0.0f);
    Require(info.updated_time == 0);
    Require(std::all_of(std::begin(info.reserve), std::end(info.reserve), [](std::uint8_t byte) { return byte == 0; }));
    Require(std::all_of(std::begin(info.rectangle.reserve), std::end(info.rectangle.reserve), [](std::uint8_t byte) { return byte == 0; }));
    return 0;
}
