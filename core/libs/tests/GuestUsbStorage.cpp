#include "prx/libc/include/General.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>

extern "C" {
int APS5_VABI sceUsbStorageInit(void* attr);
int APS5_VABI sceUsbStorageTerm();
int APS5_VABI sceUsbStorageGetDeviceList(std::uint32_t* devices, std::int32_t* count);
int APS5_VABI sceUsbStorageGetMountPointOfShellCore(std::uint32_t device, char* mountPoint);
int APS5_VABI sceUsbStorageIsExist(std::uint32_t device);
}

namespace {

constexpr int INVALID_ARGUMENT = static_cast<int>(0x80f40002);
constexpr int NO_DEVICE = static_cast<int>(0x80f40004);
constexpr int CLIENT_NOT_VALID = static_cast<int>(0x80f40009);

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "USB storage check failed at line %d\n", line);
        std::abort();
    }
}

#define Require(value) Check((value), __LINE__)

template <typename TCall> bool Throws(TCall call) {
    try {
        call();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

}

int main() {
    const auto previous = std::filesystem::current_path();
    const auto root = std::filesystem::temp_directory_path() / ("anyps5-usb-storage-" + std::to_string(std::random_device{}()));
    Require(std::filesystem::create_directory(root));
    std::filesystem::current_path(root);

    std::array<std::uint32_t, 10> devices;
    devices.fill(0xA5A5A5A5u);
    std::int32_t count = 123;
    std::array<char, 130> mount;
    mount.fill('Z');
    Require(sceUsbStorageGetDeviceList(devices.data(), &count) == CLIENT_NOT_VALID && count == 123 && devices[0] == 0xA5A5A5A5u);
    Require(sceUsbStorageGetMountPointOfShellCore(0, mount.data() + 1) == CLIENT_NOT_VALID && mount[1] == 'Z');
    Require(sceUsbStorageIsExist(0) == CLIENT_NOT_VALID);
    Require(sceUsbStorageTerm() == CLIENT_NOT_VALID);
    int attr = 0;
    Require(Throws([&] { sceUsbStorageInit(&attr); }));
    Require(sceUsbStorageInit(nullptr) == 0);
    Require(Throws([] { sceUsbStorageInit(nullptr); }));

    Require(sceUsbStorageGetDeviceList(nullptr, &count) == INVALID_ARGUMENT && count == 123);
    Require(sceUsbStorageGetDeviceList(devices.data(), nullptr) == INVALID_ARGUMENT && devices[0] == 0xA5A5A5A5u);
    Require(sceUsbStorageGetDeviceList(devices.data(), &count) == 0 && count == 0 && devices[0] == 0xA5A5A5A5u);
    Require(sceUsbStorageIsExist(0) == 0 && sceUsbStorageIsExist(8) == 0);
    Require(sceUsbStorageIsExist(std::numeric_limits<std::uint32_t>::max()) == 0);
    Require(sceUsbStorageGetMountPointOfShellCore(0, nullptr) == INVALID_ARGUMENT);
    Require(sceUsbStorageGetMountPointOfShellCore(0, mount.data() + 1) == NO_DEVICE && mount[1] == 'Z');

    std::filesystem::create_directories(root / "mnt" / "usb0");
    std::filesystem::create_directory(root / "mnt" / "usb7");
    std::filesystem::create_directory(root / "mnt" / "usb8");
    std::ofstream(root / "mnt" / "usb1").put('x');
    Require(sceUsbStorageGetDeviceList(devices.data(), &count) == 0 && count == 2);
    Require(devices[0] == 0 && devices[1] == 7 && devices[2] == 0xA5A5A5A5u);
    Require(sceUsbStorageIsExist(0) == 1 && sceUsbStorageIsExist(7) == 1 && sceUsbStorageIsExist(1) == 0);
    Require(sceUsbStorageIsExist(8) == 0);
    Require(sceUsbStorageGetMountPointOfShellCore(7, mount.data() + 1) == 0);
    Require(std::strcmp(mount.data() + 1, "/mnt/usb7") == 0 && mount[0] == 'Z' && mount[11] == 'Z');
    mount.fill('Z');
    Require(sceUsbStorageGetMountPointOfShellCore(1, mount.data() + 1) == NO_DEVICE && mount[1] == 'Z');
    Require(sceUsbStorageGetMountPointOfShellCore(8, mount.data() + 1) == NO_DEVICE && mount[1] == 'Z');

    const auto external = root / "external";
    std::filesystem::create_directory(external);
    const auto externalUtf8 = external.u8string();
    AddPathAlias_nid_no_patch("/mnt/usb2", reinterpret_cast<const char*>(externalUtf8.c_str()));
    Require(sceUsbStorageGetDeviceList(devices.data(), &count) == 0 && count == 3 && devices[1] == 2 && devices[2] == 7);
    Require(sceUsbStorageGetMountPointOfShellCore(2, mount.data() + 1) == 0 && std::strcmp(mount.data() + 1, "/mnt/usb2") == 0);
    RemovePathAlias_nid_no_patch("/mnt/usb2");
    Require(sceUsbStorageIsExist(2) == 0);
    Require(std::filesystem::remove(root / "mnt" / "usb0"));
    Require(sceUsbStorageGetDeviceList(devices.data(), &count) == 0 && count == 1 && devices[0] == 7);
    Require(sceUsbStorageIsExist(0) == 0);

    Require(std::filesystem::remove(root / "mnt" / "usb1"));
    for (std::uint32_t device = 0; device < 8; ++device) {
        std::filesystem::create_directories(root / "mnt" / ("usb" + std::to_string(device)));
    }
    devices.fill(0xA5A5A5A5u);
    Require(sceUsbStorageGetDeviceList(devices.data(), &count) == 0 && count == 8);
    for (std::uint32_t device = 0; device < 8; ++device) Require(devices[device] == device);
    Require(devices[8] == 0xA5A5A5A5u && devices[9] == 0xA5A5A5A5u);

    Require(sceUsbStorageTerm() == 0);
    count = 123;
    Require(sceUsbStorageGetDeviceList(devices.data(), &count) == CLIENT_NOT_VALID && count == 123);
    Require(sceUsbStorageGetMountPointOfShellCore(7, mount.data()) == CLIENT_NOT_VALID);
    Require(sceUsbStorageIsExist(7) == CLIENT_NOT_VALID);
    Require(sceUsbStorageInit(nullptr) == 0);
    Require(sceUsbStorageGetDeviceList(devices.data(), &count) == 0 && count == 8 && devices[0] == 0 && devices[7] == 7);
    Require(sceUsbStorageTerm() == 0);
    Require(std::filesystem::is_directory(root / "mnt" / "usb7"));

    std::filesystem::current_path(previous);
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
}
