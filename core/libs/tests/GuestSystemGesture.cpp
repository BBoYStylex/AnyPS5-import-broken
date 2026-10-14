#include "prx/libc/include/general/VabiMacros.hpp"
#include "SceTypes.hpp"

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
int APS5_VABI sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(std::int32_t, std::uint16_t, SystemGesturePrimitiveTouchEvent*);
int APS5_VABI sceSystemGestureGetTouchEventByEventID(std::int32_t, const SystemGestureTouchRecognizer*, std::uint32_t, SystemGestureTouchEvent*);
}

static void Require(bool value) { if (!value) std::abort(); }

template <typename TException, typename TAction>
static bool Throws(TAction action) {
    try {
        action();
    } catch (const TException&) {
        return true;
    }
    return false;
}

static void RecognizerInformation(std::int32_t handle) {
    constexpr int invalidArgument = static_cast<int>(0x80D10002);
    constexpr int invalidHandle = static_cast<int>(0x80D10003);
    constexpr std::int32_t drag = 2;
    auto* recognizer = new SystemGestureTouchRecognizer;
    std::memset(recognizer, 0x5a, sizeof(*recognizer));
    SystemGestureTouchRecognizerInformation information{};

    Require(Throws<std::invalid_argument>([&] { sceSystemGestureGetTouchRecognizerInformation(handle, recognizer, &information); }));
    Require(Throws<std::invalid_argument>([&] {
        const SystemGestureRectangle moved{1.0f, 2.0f, 3.0f, 4.0f, {}};
        sceSystemGestureUpdateTouchRecognizerRectangle(handle, recognizer, &moved);
    }));

    const SystemGestureRectangle rectangle{10.0f, 20.0f, 300.0f, 400.0f, {}};
    Require(sceSystemGestureCreateTouchRecognizer(handle + 1, recognizer, drag, &rectangle, nullptr) == invalidHandle);
    Require(sceSystemGestureCreateTouchRecognizer(handle, recognizer, drag, &rectangle, nullptr) == 0);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle + 1, recognizer, &information) == invalidHandle);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, nullptr, &information) == invalidArgument);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, recognizer, nullptr) == invalidArgument);

    std::memset(&information, 0x5a, sizeof(information));
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, recognizer, &information) == 0);
    Require(information.gesture_type == drag);
    Require(information.rectangle.x == 10.0f && information.rectangle.y == 20.0f);
    Require(information.rectangle.width == 300.0f && information.rectangle.height == 400.0f);
    Require(information.updated_time == 0);
    for (const auto byte : information.reserve) Require(byte == 0);

    const SystemGestureRectangle moved{1.0f, 2.0f, 3.0f, 4.0f, {}};
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, recognizer, &moved) == 0);
    Require(sceSystemGestureResetTouchRecognizer(handle, recognizer) == 0);
    Require(sceSystemGestureUpdateTouchRecognizer(handle, recognizer) == 0);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, recognizer, &information) == 0);
    Require(information.gesture_type == drag);
    Require(information.rectangle.x == 1.0f && information.rectangle.y == 2.0f);
    Require(information.rectangle.width == 3.0f && information.rectangle.height == 4.0f);

    SystemGestureTouchEvent event{};
    Require(sceSystemGestureGetTouchEventByEventID(handle + 1, recognizer, 1, &event) == invalidHandle);
    Require(sceSystemGestureGetTouchEventByEventID(handle, nullptr, 1, &event) == invalidArgument);
    Require(sceSystemGestureGetTouchEventByEventID(handle, recognizer, 1, nullptr) == invalidArgument);
    Require(Throws<std::runtime_error>([&] { sceSystemGestureGetTouchEventByEventID(handle, recognizer, 1, &event); }));

    Require(sceSystemGestureCreateTouchRecognizer(handle, recognizer, drag, nullptr, nullptr) == 0);
    Require(Throws<std::runtime_error>([&] { sceSystemGestureGetTouchRecognizerInformation(handle, recognizer, &information); }));
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, recognizer, &rectangle) == 0);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, recognizer, &information) == 0);
    Require(information.rectangle.width == 300.0f);
    delete recognizer;
}

static void PrimitiveTouchEventById(std::int32_t handle) {
    constexpr int invalidArgument = static_cast<int>(0x80D10002);
    constexpr int invalidHandle = static_cast<int>(0x80D10003);
    SystemGesturePrimitiveTouchEvent event{};
    Require(sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(handle + 1, 0, &event) == invalidHandle);
    Require(sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(handle, 0, nullptr) == invalidArgument);
    Require(Throws<std::runtime_error>([&] { sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(handle, 0, &event); }));
}

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
    auto* created = new SystemGestureTouchRecognizer;
    const SystemGestureRectangle rectangle{};
    Require(sceSystemGestureCreateTouchRecognizer(handle, created, 1, &rectangle, nullptr) == 0);
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
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, created, &rectangle) == 0);
    Require(sceSystemGestureResetPrimitiveTouchRecognizer(handle) == 0);
    Require(sceSystemGestureUpdateAllTouchRecognizer(handle) == 0);
    Require(sceSystemGestureUpdatePrimitiveTouchRecognizer(handle, nullptr) == 0);
    Require(sceSystemGestureAppendTouchRecognizer(handle, nullptr) == invalidArgument);
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, created, nullptr) == invalidArgument);
    Require(sceSystemGestureAppendTouchRecognizer(handle + 1, &recognizer) == invalidHandle);
    Require(sceSystemGestureRemoveTouchRecognizer(handle, nullptr) == invalidArgument);
    Require(sceSystemGestureResetTouchRecognizer(handle, nullptr) == invalidArgument);
    Require(sceSystemGestureUpdateTouchRecognizer(handle, nullptr) == invalidArgument);
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, nullptr, &rectangle) == invalidArgument);
    Require(sceSystemGestureRemoveTouchRecognizer(handle + 1, &recognizer) == invalidHandle);
    Require(sceSystemGestureResetTouchRecognizer(handle + 1, &recognizer) == invalidHandle);
    Require(sceSystemGestureUpdateTouchRecognizer(handle + 1, &recognizer) == invalidHandle);
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle + 1, created, &rectangle) == invalidHandle);
    Require(sceSystemGestureResetPrimitiveTouchRecognizer(handle + 1) == invalidHandle);
    Require(sceSystemGestureUpdateAllTouchRecognizer(handle + 1) == invalidHandle);
    Require(sceSystemGestureUpdatePrimitiveTouchRecognizer(handle + 1, nullptr) == invalidHandle);
    delete created;

    RecognizerInformation(handle);
    PrimitiveTouchEventById(handle);
    return 0;
}
