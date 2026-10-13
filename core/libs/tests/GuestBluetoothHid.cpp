#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" {
std::int32_t APS5_VABI sceBluetoothHidInit();
std::int32_t APS5_VABI sceBluetoothHidRegisterCallback(void* callback, std::int32_t flags, void* param);
std::int32_t APS5_VABI sceBluetoothHidUnregisterCallback();
std::int32_t APS5_VABI sceBluetoothHidRegisterDevice(std::uint16_t vendorId, std::uint16_t productId);
std::int32_t APS5_VABI sceBluetoothHidUnregisterDevice(std::uint16_t vendorId, std::uint16_t productId);
std::int32_t APS5_VABI sceBluetoothHidGetDeviceName(std::uint64_t handle, void* nameOut);
std::int32_t APS5_VABI sceBluetoothHidGetInputReport(void* report, std::uint32_t reportId);
std::int32_t APS5_VABI sceBluetoothHidSetOutputReport(std::uint64_t handle, std::uint32_t reportId, void* data, std::uint64_t length);
}

namespace {

int callbackCalls = 0;

void APS5_VABI Callback() {
    ++callbackCalls;
}

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "BluetoothHid: %s\n", message);
        std::abort();
    }
}

}

int main() {
    Require(sceBluetoothHidInit() == 0, "initialization failed");
    std::uint8_t param[16]{};
    Require(sceBluetoothHidRegisterCallback(reinterpret_cast<void*>(&Callback), 0, param) == 0, "callback registration failed");
    Require(sceBluetoothHidRegisterDevice(0x44f, 0x1000) == 0, "device registration failed");
    Require(callbackCalls == 0, "a device connection was reported");
    bool threw = false;
    try {
        sceBluetoothHidRegisterCallback(nullptr, 0, param);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Require(threw, "a null callback was accepted");
    threw = false;
    try {
        std::uint8_t report[64]{};
        sceBluetoothHidGetInputReport(report, 1);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "an input report without a device did not throw");
    threw = false;
    try {
        sceBluetoothHidGetInputReport(nullptr, 1);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Require(threw, "a null input report was accepted");
    threw = false;
    try {
        char name[128]{};
        sceBluetoothHidGetDeviceName(0, name);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "a device name without a device did not throw");
    threw = false;
    try {
        sceBluetoothHidGetDeviceName(0, nullptr);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Require(threw, "a null device name output was accepted");
    threw = false;
    try {
        std::uint8_t output[64]{};
        sceBluetoothHidSetOutputReport(0, 1, output, sizeof(output));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "an output report without a device did not throw");
    threw = false;
    try {
        sceBluetoothHidSetOutputReport(0, 1, nullptr, 64);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Require(threw, "a null output report was accepted");
    Require(sceBluetoothHidUnregisterDevice(0x44f, 0x1000) == 0, "device unregistration failed");
    Require(sceBluetoothHidUnregisterCallback() == 0, "callback unregistration failed");
    std::puts("BluetoothHid tests passed");
    return 0;
}
