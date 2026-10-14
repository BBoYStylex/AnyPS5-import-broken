#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

extern "C" {
std::int32_t APS5_VABI sceSystemGestureOpen(std::int32_t, const void*);
int APS5_VABI sceSystemGestureGetPrimitiveTouchEvents(std::int32_t, void*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceSystemGestureGetPrimitiveTouchEventsCount(std::int32_t);
int APS5_VABI sceSystemGestureGetTouchEvents(std::int32_t, const void*, void*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceSystemGestureGetPrimitiveTouchEventByIndex(std::int32_t, std::uint32_t, void*);
int APS5_VABI sceSystemGestureAppendTouchRecognizer(std::int32_t, void*);
int APS5_VABI sceSystemGestureRemoveTouchRecognizer(std::int32_t, void*);
int APS5_VABI sceSystemGestureResetTouchRecognizer(std::int32_t, void*);
int APS5_VABI sceSystemGestureUpdateTouchRecognizer(std::int32_t, void*);
int APS5_VABI sceSystemGestureUpdateTouchRecognizerRectangle(std::int32_t, void*, const void*);
int APS5_VABI sceSystemGestureResetPrimitiveTouchRecognizer(std::int32_t);
int APS5_VABI sceSystemGestureUpdateAllTouchRecognizer(std::int32_t);
int APS5_VABI sceSystemGestureUpdatePrimitiveTouchRecognizer(std::int32_t, const void*);
int APS5_VABI sceSystemGestureCreateTouchRecognizer(std::int32_t, SystemGestureTouchRecognizer*, std::int32_t, const SystemGestureRectangle*, const void*);
int APS5_VABI sceSystemGestureGetTouchRecognizerInformation(std::int32_t, const SystemGestureTouchRecognizer*, SystemGestureTouchRecognizerInformation*);
int APS5_VABI sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(std::int32_t, std::uint16_t, void*);
int APS5_VABI sceSystemGestureGetTouchEventByEventID(std::int32_t, const SystemGestureTouchRecognizer*, std::uint32_t, void*);
}

static bool SameRectangle(const SystemGestureRectangle& left, const SystemGestureRectangle& right) {
    return left.x == right.x && left.y == right.y && left.width == right.width && left.height == right.height;
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
    SystemGestureTouchRecognizer recognizer{};
    SystemGestureRectangle rectangle{10.0f, 20.0f, 300.0f, 400.0f, {}};
    constexpr std::int32_t tapType = 1;
    Require(sceSystemGestureCreateTouchRecognizer(handle, &recognizer, tapType, &rectangle, nullptr) == 0);
    count = 7;
    Require(sceSystemGestureGetTouchEvents(handle, &recognizer, buffer.data(), 4, &count) == 0 && count == 0);
    Require(sceSystemGestureGetTouchEvents(handle, &recognizer, buffer.data(), 4, nullptr) == invalidArgument);
    Require(sceSystemGestureGetTouchEvents(handle + 1, &recognizer, buffer.data(), 4, &count) == invalidHandle);
    Require(sceSystemGestureGetPrimitiveTouchEventByIndex(handle, 0, buffer.data()) == indexOutOfArray);
    Require(sceSystemGestureGetPrimitiveTouchEventByIndex(handle + 1, 0, buffer.data()) == invalidHandle);
    Require(sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(handle, 0, buffer.data()) == indexOutOfArray);
    Require(sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(handle + 1, 0, buffer.data()) == invalidHandle);
    Require(sceSystemGestureGetTouchEventByEventID(handle, &recognizer, 0, buffer.data()) == indexOutOfArray);
    Require(sceSystemGestureGetTouchEventByEventID(handle, nullptr, 0, buffer.data()) == invalidArgument);
    Require(sceSystemGestureGetTouchEventByEventID(handle + 1, &recognizer, 0, buffer.data()) == invalidHandle);
    Require(buffer == original);

    SystemGestureTouchRecognizerInformation information;
    std::memset(&information, 0x5a, sizeof(information));
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, &recognizer, &information) == 0);
    Require(information.gesture_type == tapType);
    Require(SameRectangle(information.rectangle, rectangle));
    Require(information.updated_time == 0);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, nullptr, &information) == invalidArgument);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, &recognizer, nullptr) == invalidArgument);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle + 1, &recognizer, &information) == invalidHandle);
    Require(sceSystemGestureCreateTouchRecognizer(handle, &recognizer, tapType, nullptr, nullptr) == invalidArgument);
    Require(sceSystemGestureCreateTouchRecognizer(handle, nullptr, tapType, &rectangle, nullptr) == invalidArgument);
    Require(sceSystemGestureCreateTouchRecognizer(handle + 1, &recognizer, tapType, &rectangle, nullptr) == invalidHandle);

    SystemGestureTouchRecognizer uncreated{};
    bool threw = false;
    try {
        sceSystemGestureGetTouchRecognizerInformation(handle, &uncreated, &information);
    } catch (const std::logic_error&) {
        threw = true;
    }
    Require(threw);

    Require(sceSystemGestureAppendTouchRecognizer(handle, &recognizer) == 0);
    Require(sceSystemGestureRemoveTouchRecognizer(handle, &recognizer) == 0);
    Require(sceSystemGestureResetTouchRecognizer(handle, &recognizer) == 0);
    Require(sceSystemGestureUpdateTouchRecognizer(handle, &recognizer) == 0);
    const SystemGestureRectangle moved{50.0f, 60.0f, 70.0f, 80.0f, {}};
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, &recognizer, &moved) == 0);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, &recognizer, &information) == 0);
    Require(information.gesture_type == tapType);
    Require(SameRectangle(information.rectangle, moved));
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
    return 0;
}
