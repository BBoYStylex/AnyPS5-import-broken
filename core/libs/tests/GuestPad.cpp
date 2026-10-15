#include "prx/libScePad/include/Pad.hpp"
#include "prx/libScePad/include/PadInputTypes.hpp"
#include "prx/libScePad/include/PadState.hpp"
#include "prx/libScePad/include/InputCommandFile.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>

extern "C" {
int APS5_VABI scePadOpen_nid_postfix(int, int, int, const void*);
int APS5_VABI scePadOpenExt(int, int, int, const void*);
int APS5_VABI scePadClose_nid_postfix(int);
int APS5_VABI scePadGetHandle(int, int, int);
int APS5_VABI scePadSetVibrationTriggerEffectWeakWhileEmbeddedMicInUse(bool);
int APS5_VABI scePadSetVibrationMode(int, int);
int APS5_VABI scePadInit_nid_postfix(void);
int APS5_VABI scePadRead_nid_postfix(int, PadData*, int);
int APS5_VABI scePadReadState(int, PadData*);
int APS5_VABI scePadSetTiltCorrectionState(int, bool);
int APS5_VABI scePadResetOrientation(int);
int APS5_VABI scePadSetAngularVelocityDeadbandState(int, bool);
int APS5_VABI scePadSetAngularVelocityBiasCorrectionState(int, bool);
int APS5_VABI scePadIsRemoteController(int, bool*);
}

static void Require(bool value) { if (!value) std::abort(); }

static void Require(bool value, const char* message) {
    if (value) return;
    std::fprintf(stderr, "touch contact history: %s\n", message);
    std::abort();
}

static void CheckInputCommands() {
    const auto path = std::filesystem::temp_directory_path() / ("anyps5-pad-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{path};
    const auto write = [&](const char* command) { std::ofstream(path) << command << '\n'; };
    write("1 16384 200");
    Pad::InputCommandFile commands(path.string().c_str());
    Require(commands.Buttons(1000) == 0);
    write("2 16384 200");
    Require(commands.Buttons(101000) == 0x4000);
    Require(commands.Buttons(300999) == 0x4000);
    Require(commands.Buttons(301000) == 0);
    Require(commands.Buttons(401000) == 0);
    write("3 8192 5001");
    Require(commands.Buttons(501000) == 0);
    write("-3 8192 300");
    Require(commands.Buttons(601000) == 0);
    write("3 1 300");
    Require(commands.Buttons(701000) == 0);
    write("3 8192 300 trailing");
    Require(commands.Buttons(801000) == 0);
    write("3 8192 300");
    Require(commands.Buttons(901000) == 0x2000);
    write("4 0 1");
    Require(commands.Buttons(1001000) == 0);
    write("2 16384 200");
    Require(commands.Buttons(1101000) == 0);
    write("5 0 300 0 128 255 64");
    const auto tilted = commands.Poll(1201000);
    Require(tilted.buttons == 0 && tilted.hasSticks);
    Require(tilted.sticks[0] == 0 && tilted.sticks[1] == 128 && tilted.sticks[2] == 255 && tilted.sticks[3] == 64);
    Require(!commands.Poll(1501000).hasSticks);
    write("6 16384 300");
    const auto pressed = commands.Poll(1601000);
    Require(pressed.buttons == 0x4000 && !pressed.hasSticks);
    const Pad::InputCommandFile::Input none;
    write("7 16384 300 128 128");
    Require(commands.Poll(1901000) == none);
    write("7 16384 300 128 128 128 256");
    Require(commands.Poll(2001000) == none);
    write("7 16384 300 128 128 128 128 128");
    Require(commands.Poll(2101000) == none);
    write("7 16384 300 128 128 128 up");
    Require(commands.Poll(2201000) == none);
    write("7 16384 300 +");
    Require(commands.Poll(2301000) == none);
    write("7 16384 300 4294967296");
    Require(commands.Poll(2401000) == none);
    write("7 16384 300 128 128 128 128x");
    Require(commands.Poll(2501000) == none);
    write("7 8192 300 1 2 3 4");
    const auto accepted = commands.Poll(2601000);
    Require(accepted.buttons == 0x2000 && accepted.hasSticks && accepted.sticks[3] == 4);
    std::filesystem::remove(path);
    Require(commands.Buttons(3001000) == 0);
    Pad::InputCommandFile disabled(nullptr);
    Require(disabled.Buttons(0) == 0);
}

static float SettleOrientationW() {
    float w = 1.0f;
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        w = Pad::ReadState().orientation_w;
    }
    return w;
}

static void CheckTiltCorrection(int handle) {
    Require(scePadInit_nid_postfix() == 0);
    PadInputState tilted;
    tilted.hasMotion = true;
    tilted.accel = {9.80665f, 0.0f, 0.0f};
    PadPublishInput_nid_postfix(tilted);
    Require(scePadSetTiltCorrectionState(handle + 1, false) == PAD_ERROR_INVALID_HANDLE);
    Require(scePadSetTiltCorrectionState(handle, false) == PAD_OK);
    Require(scePadResetOrientation(handle) == PAD_OK);
    Require(SettleOrientationW() == 1.0f);
    Require(scePadSetTiltCorrectionState(handle, true) == PAD_OK);
    Require(SettleOrientationW() < 0.999f);
    PadPublishInput_nid_postfix(PadInputState{});
}

static void CheckAngularVelocityDeadband(int handle) {
    PadInputState moving;
    moving.hasMotion = true;
    moving.accel = {0.0f, 9.80665f, 0.0f};
    moving.gyro = {0.01f, 0.0175f, -0.017f};
    PadPublishInput_nid_postfix(moving);
    PadData data{};
    Require(scePadReadState(handle, &data) == PAD_OK);
    Require(data.angular_velocity_x == 0.01f && data.angular_velocity_y == 0.0175f && data.angular_velocity_z == -0.017f);
    Require(scePadSetAngularVelocityDeadbandState(handle, true) == PAD_OK);
    Require(scePadReadState(handle, &data) == PAD_OK);
    Require(data.angular_velocity_x == 0.0f && data.angular_velocity_y == 0.0175f && data.angular_velocity_z == 0.0f);
    Require(scePadSetAngularVelocityDeadbandState(handle, false) == PAD_OK);
    Require(scePadReadState(handle, &data) == PAD_OK);
    Require(data.angular_velocity_x == 0.01f && data.angular_velocity_z == -0.017f);
    PadPublishInput_nid_postfix(PadInputState{});
}

static void CheckTouchContact() {
    PadInputState touch;
    touch.buttons = static_cast<std::uint32_t>(Pad::PadButton::TouchPad);
    touch.touch[0] = {true, 960, 471, 0};
    PadPublishInput_nid_postfix(touch);
    const auto data = Pad::ReadState();
    Require((data.buttons & static_cast<std::uint32_t>(Pad::PadButton::TouchPad)) != 0);
    Require(data.touch_data_touch_num == 1);
    Require(data.touch_data_touch0_x == 960);
    Require(data.touch_data_touch0_y == 471);
    PadPublishInput_nid_postfix(PadInputState{});
    Pad::ReadState();
}

static void CheckTouchHistory(int handle) {
    const auto read = [&] {
        PadData data{};
        Require(scePadReadState(handle, &data) == PAD_OK);
        return data;
    };
    const auto reset = [&] {
        PadPublishInput_nid_postfix(PadInputState{});
        read();
        PadData history[64]{};
        Require(scePadRead_nid_postfix(handle, history, 64) > 0);
    };

    reset();
    PadInputState idle;
    idle.buttons = static_cast<std::uint32_t>(Pad::PadButton::Cross);
    PadPublishInput_nid_postfix(idle);
    auto held = idle;
    held.touch[0] = {true, 120, 240, 0};
    PadPublishInput_nid_postfix(held);
    const auto beforePartial = read();
    const auto repeated = read();
    Require(repeated.touch_data_touch0_id == beforePartial.touch_data_touch0_id, "repeated state reads changed a held contact ID");
    PadData older{};
    Require(scePadRead_nid_postfix(handle, &older, 1) == 1 && older.touch_data_touch_num == 0);
    const auto afterPartial = read();
    Require(afterPartial.touch_data_touch_num == 1 && afterPartial.touch_data_touch0_x == 120 && afterPartial.touch_data_touch0_y == 240);
    Require(afterPartial.touch_data_touch0_id == beforePartial.touch_data_touch0_id, "reading an older sample changed the current held contact ID");
    Require(scePadRead_nid_postfix(handle, &older, 1) == 1 && older.touch_data_touch_num == 1);
    Require(older.touch_data_touch0_id == beforePartial.touch_data_touch0_id, "the queued sample disagrees with the same held contact");

    reset();
    PadInputState first;
    first.touch[0] = {true, 100, 200, 0};
    PadPublishInput_nid_postfix(first);
    const auto firstContact = read();
    PadPublishInput_nid_postfix(PadInputState{});
    auto second = first;
    second.touch[0] = {true, 900, 400, 0};
    PadPublishInput_nid_postfix(second);
    const auto secondContact = read();
    Require(secondContact.touch_data_touch0_id != firstContact.touch_data_touch0_id, "a release and new contact without an intervening read reused the old ID");
    PadData history[64]{};
    Require(scePadRead_nid_postfix(handle, history, 64) == 3);
    Require(history[0].touch_data_touch_num == 1 && history[0].touch_data_touch0_x == 100);
    Require(history[0].touch_data_touch0_id == firstContact.touch_data_touch0_id, "an old contact changed ID after a newer contact was read");
    Require(history[1].touch_data_touch_num == 0 && history[2].touch_data_touch_num == 1 && history[2].touch_data_touch0_x == 900);
    Require(history[2].touch_data_touch0_id == secondContact.touch_data_touch0_id && read().touch_data_touch0_id == secondContact.touch_data_touch0_id, "draining history changed the new held contact ID");

    reset();
    auto both = first;
    both.touch[1] = {true, 900, 400, 0};
    PadPublishInput_nid_postfix(both);
    const auto originalPair = read();
    Require(originalPair.touch_data_touch_num == 2 && originalPair.touch_data_touch0_id != originalPair.touch_data_touch1_id);
    auto survivor = both;
    survivor.touch[0].active = false;
    PadPublishInput_nid_postfix(survivor);
    const auto one = read();
    Require(one.touch_data_touch_num == 1 && one.touch_data_touch0_x == 900);
    Require(one.touch_data_touch0_id == originalPair.touch_data_touch1_id, "releasing one finger changed the surviving finger's ID");
    auto replacement = survivor;
    replacement.touch[0] = {true, 300, 500, 0};
    PadPublishInput_nid_postfix(replacement);
    const auto newPair = read();
    Require(newPair.touch_data_touch_num == 2 && newPair.touch_data_touch0_id != originalPair.touch_data_touch0_id);
    Require(newPair.touch_data_touch1_id == originalPair.touch_data_touch1_id, "replacing another finger changed the surviving finger's ID");
    Require(scePadRead_nid_postfix(handle, history, 64) == 3);
    Require(history[0].touch_data_touch0_id == originalPair.touch_data_touch0_id && history[0].touch_data_touch1_id == originalPair.touch_data_touch1_id, "history reassigned the original two-finger contacts");
    Require(history[1].touch_data_touch0_id == one.touch_data_touch0_id && history[2].touch_data_touch0_id == newPair.touch_data_touch0_id && history[2].touch_data_touch1_id == newPair.touch_data_touch1_id, "history reassigned a surviving or replacement finger");
    const auto afterPairHistory = read();
    Require(afterPairHistory.touch_data_touch0_id == newPair.touch_data_touch0_id && afterPairHistory.touch_data_touch1_id == newPair.touch_data_touch1_id, "two-finger history changed the current IDs");

    reset();
    for (std::uint16_t contact = 0; contact < 40; ++contact) {
        PadPublishInput_nid_postfix(PadInputState{});
        auto input = first;
        input.touch[0].x = static_cast<std::uint16_t>(100 + contact);
        PadPublishInput_nid_postfix(input);
    }
    const auto beforeRollover = read();
    Require(scePadRead_nid_postfix(handle, history, 64) == 64);
    for (int sample = 0; sample < 64; ++sample) {
        Require(history[sample].touch_data_touch_num == (sample % 2 == 0 ? 0 : 1));
        if (sample % 2 == 0) continue;
        Require(history[sample].touch_data_touch0_x == 108 + sample / 2);
        Require(history[sample].touch_data_touch0_id < 128, "a queued contact ID used the reserved high bit");
        if (sample >= 3) Require(history[sample].touch_data_touch0_id != history[sample - 2].touch_data_touch0_id, "consecutive queued contacts shared an ID");
    }
    Require(history[63].touch_data_touch0_id == beforeRollover.touch_data_touch0_id && read().touch_data_touch0_id == beforeRollover.touch_data_touch0_id, "queue rollover changed the latest held contact ID");

    reset();
    auto moving = first;
    PadPublishInput_nid_postfix(moving);
    const auto movingContact = read();
    for (std::uint16_t sample = 0; sample < 80; ++sample) {
        moving.touch[0].x = static_cast<std::uint16_t>(200 + sample);
        PadPublishInput_nid_postfix(moving);
    }
    Require(scePadRead_nid_postfix(handle, history, 64) == 64);
    for (const auto& sample : history) Require(sample.touch_data_touch_num == 1 && sample.touch_data_touch0_id == movingContact.touch_data_touch0_id, "moving a continuously held contact assigned a new ID");
    Require(read().touch_data_touch0_id == movingContact.touch_data_touch0_id, "reading the moving-contact history changed its current ID");
    reset();
    std::puts("touch contact history: partial reads, new contacts, two fingers and queue rollover passed");
}

static void CheckPartialTrigger() {
    PadInputState partial;
    partial.buttons = static_cast<std::uint32_t>(Pad::PadButton::L2) | static_cast<std::uint32_t>(Pad::PadButton::R2);
    partial.analogButtonsL2 = 1;
    partial.analogButtonsR2 = 73;
    PadPublishInput_nid_postfix(partial);
    const auto data = Pad::ReadState();
    Require(data.buttons == partial.buttons);
    Require(data.analog_buttons_l2 == 1 && data.analog_buttons_r2 == 73);
    PadInputState digital;
    digital.buttons = static_cast<std::uint32_t>(Pad::PadButton::L2);
    PadPublishInput_nid_postfix(digital);
    Require(Pad::ReadState().analog_buttons_l2 == 255);
    PadPublishInput_nid_postfix(PadInputState{});
    Pad::ReadState();
}

static void CheckReadQueue(int handle) {
    PadData drained[64]{};
    scePadRead_nid_postfix(handle, drained, 64);
    PadInputState pressed;
    pressed.buttons = static_cast<std::uint32_t>(Pad::PadButton::Cross);
    PadPublishInput_nid_postfix(pressed);
    PadPublishInput_nid_postfix(PadInputState{});
    PadData data[4]{};
    Require(scePadRead_nid_postfix(handle, data, 4) == 2);
    Require((data[0].buttons & pressed.buttons) != 0);
    Require((data[1].buttons & pressed.buttons) == 0);
    Require(data[0].timestamp <= data[1].timestamp);
    Require(scePadRead_nid_postfix(handle, data, 4) == 1);
    Require(data[0].buttons == 0);
}

static void CheckReadStateHandle(int handle) {
    PadData data{};
    Require(scePadReadState(0, &data) == PAD_ERROR_INVALID_HANDLE);
    Require(scePadReadState(handle + 1, &data) == PAD_ERROR_INVALID_HANDLE);
    Require(scePadRead_nid_postfix(0, &data, 1) == PAD_ERROR_INVALID_HANDLE);
    Require(scePadReadState(handle, &data) == PAD_OK);
}

static void CheckRemoteController(int handle) {
    bool remote = true;
    Require(scePadIsRemoteController(handle + 1, &remote) == PAD_ERROR_INVALID_HANDLE);
    Require(scePadIsRemoteController(handle, nullptr) == PAD_ERROR_INVALID_ARG);
    Require(remote);
    Require(scePadIsRemoteController(handle, &remote) == PAD_OK);
    Require(!remote);
}

static void CheckInjectedState(const std::filesystem::path& path) {
    PadInputState physical;
    physical.buttons = 0x2000;
    physical.analogButtonsR2 = 73;
    physical.sticks[0] = 42;
    PadPublishInput_nid_postfix(physical);
    const auto before = Pad::ReadState();
    std::ofstream(path) << "1 16640 300\n";
    PadData pressed{};
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    do {
        pressed = Pad::ReadState();
        if ((pressed.buttons & 0x4000) != 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (std::chrono::steady_clock::now() < deadline);
    Require(pressed.buttons == (0x2000 | 0x4000 | 0x100));
    Require(pressed.analog_buttons_l2 == 255 && pressed.analog_buttons_r2 == 73);
    Require(pressed.left_stick_x == 42 && pressed.timestamp > before.timestamp);
    std::this_thread::sleep_for(std::chrono::milliseconds(310));
    const auto released = Pad::ReadState();
    Require(released.buttons == 0x2000 && released.analog_buttons_l2 == 0);
    Require(released.timestamp > pressed.timestamp);
    physical.sticks[3] = 17;
    PadPublishInput_nid_postfix(physical);
    const auto still = Pad::ReadState();
    std::ofstream(path) << "2 0 300 200 0 128 128\n";
    PadData moved{};
    const auto stickDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    do {
        moved = Pad::ReadState();
        if (moved.left_stick_x == 200) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (std::chrono::steady_clock::now() < stickDeadline);
    Require(moved.left_stick_x == 200 && moved.left_stick_y == 0);
    Require(moved.right_stick_x == 128 && moved.right_stick_y == 128);
    Require(moved.buttons == 0x2000 && moved.timestamp > still.timestamp);
    std::this_thread::sleep_for(std::chrono::milliseconds(310));
    const auto centred = Pad::ReadState();
    Require(centred.left_stick_x == 42 && centred.right_stick_y == 17);
    Require(centred.timestamp > moved.timestamp);
    PadPublishInput_nid_postfix(PadInputState{});
}

int main() {
    CheckInputCommands();
    const auto path = std::filesystem::temp_directory_path() / ("anyps5-pad-state-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{path};
#ifdef _WIN32
    Require(_putenv_s("APS5_PAD_INPUT_FILE", path.string().c_str()) == 0);
#else
    Require(setenv("APS5_PAD_INPUT_FILE", path.string().c_str(), 1) == 0);
#endif
    {
        std::uint32_t sequence = 0;
        PadOutputState snapshot;
        Pad::SetVibration(12, 34);
        PadSubmitAudioHaptics_nid_postfix(100, 150);
        Require(PadFetchOutput_nid_postfix(&sequence, &snapshot));
        Require(snapshot.audioVibrationLarge == 100 && snapshot.audioVibrationSmall == 150);
        Require(snapshot.vibrationLarge == 12 && snapshot.vibrationSmall == 34);
        PadSubmitAudioHaptics_nid_postfix(0, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        Require(PadFetchOutput_nid_postfix(&sequence, &snapshot));
        Require(snapshot.audioVibrationLarge == 0 && snapshot.audioVibrationSmall == 0);
        Require(snapshot.vibrationLarge == 12 && snapshot.vibrationSmall == 34);
        Pad::SetVibration(0, 0);
    }
    constexpr int noHandle = static_cast<int>(0x80920008);
    constexpr int user = 0x10000000;

    Require(scePadGetHandle(user, 0, 0) == noHandle);
    Require(scePadOpen_nid_postfix(user, 1, 0, nullptr) == PAD_ERROR_INVALID_ARG);
    Require(scePadOpen_nid_postfix(user, 0, 1, nullptr) == PAD_ERROR_INVALID_ARG);
    Require(scePadGetHandle(user, 0, 0) == noHandle);
    const unsigned char wheel[16]{0xb7, 0x0e, 0x04, 0x0e, 0x08, 0x6e, 0x01};
    Require(scePadOpenExt(user, PAD_PORT_TYPE_SPECIAL, 0, wheel) == static_cast<int>(0x80920007));
    Require(scePadOpenExt(user, PAD_PORT_TYPE_SPECIAL, 1, wheel) == PAD_ERROR_INVALID_ARG);
    Require(scePadOpenExt(user, PAD_PORT_TYPE_SPECIAL, 0, nullptr) == PAD_ERROR_INVALID_ARG);
    Require(scePadGetHandle(user, 0, 0) == noHandle);
    Require(scePadOpen_nid_postfix(user, PAD_PORT_TYPE_SPECIAL, 0, nullptr) == static_cast<int>(0x80920007));
    Require(scePadGetHandle(user, 0, 0) == noHandle);
    const int handle = scePadOpen_nid_postfix(user, 0, 0, nullptr);
    Require(handle > 0);
    Require(scePadGetHandle(user, 0, 0) == handle);
    Require(scePadOpen_nid_postfix(user, PAD_PORT_TYPE_SPECIAL, 0, nullptr) == static_cast<int>(0x80920007));
    Require(scePadGetHandle(user, PAD_PORT_TYPE_SPECIAL, 0) == noHandle);
    Require(scePadGetHandle(user, 0, 0) == handle);
    CheckTiltCorrection(handle);
    CheckAngularVelocityDeadband(handle);
    CheckTouchContact();
    CheckTouchHistory(handle);
    CheckPartialTrigger();
    CheckReadQueue(handle);
    CheckReadStateHandle(handle);
    CheckRemoteController(handle);
    CheckInjectedState(path);
    Require(scePadSetVibrationMode(handle, 1) == 0);
    Require(scePadSetVibrationMode(handle, 2) == 0);
    Require(scePadSetVibrationMode(handle, 3) == 0);
    Require(scePadSetVibrationMode(handle, 4) == PAD_ERROR_INVALID_ARG);
    Require(scePadSetVibrationMode(handle, 5) == 0);
    Require(scePadSetVibrationMode(handle, 6) == PAD_ERROR_INVALID_ARG);
    Require(scePadSetVibrationMode(handle, -1) == PAD_ERROR_INVALID_ARG);
    Require(scePadGetHandle(0xff, 16, 0) == handle);
    Require(scePadGetHandle(user, 16, 0) == noHandle);
    Require(scePadGetHandle(user, 0, 1) == noHandle);
    Require(scePadClose_nid_postfix(handle) == 0);
    Require(scePadGetHandle(user, 0, 0) == noHandle);
    Require(scePadSetVibrationTriggerEffectWeakWhileEmbeddedMicInUse(true) == 0);
    Require(scePadSetAngularVelocityDeadbandState(handle, false) == 0);
    Require(scePadSetAngularVelocityDeadbandState(handle + 1, false) == PAD_ERROR_INVALID_HANDLE);
    Require(scePadSetAngularVelocityDeadbandState(handle + 1, true) == PAD_ERROR_INVALID_HANDLE);
    Require(scePadSetAngularVelocityBiasCorrectionState(handle, false) == PAD_OK);
    Require(scePadSetAngularVelocityBiasCorrectionState(handle + 1, false) == PAD_ERROR_INVALID_HANDLE);
}
