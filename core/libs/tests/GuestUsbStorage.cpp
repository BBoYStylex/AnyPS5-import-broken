#include "prx/libSceUsbStorage/include/UsbStorage.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#if defined(__linux__)
#include <stdlib.h>
#endif

extern "C" {
int APS5_VABI sceUsbStorageInit(void* attr);
int APS5_VABI sceUsbStorageTerm(void);
int APS5_VABI sceUsbStorageGetDeviceList(std::uint32_t* devices, std::int32_t* count);
int APS5_VABI sceUsbStorageGetMountPointOfShellCore(std::uint32_t device, char* mountPoint);
int APS5_VABI sceUsbStorageGetDeviceInfo(std::uint32_t device, void* info);
int APS5_VABI sceUsbStorageIsExist(std::uint32_t device);
int APS5_VABI sceUsbStorageRequestMap(std::uint32_t device, const char* directory, std::int32_t flags,
                                      std::uint64_t volumeSize, char* mountPoint, std::uint64_t* mountId,
                                      const void* reserved, std::size_t reservedSize);
int APS5_VABI sceUsbStorageRegisterCallback(std::int32_t eventType, void* callback, void* arg);
int APS5_VABI sceUsbStorageUnregisterCallback(std::int32_t eventType, void* callback);
int APS5_VABI sceUsbStorageGetdentsOpen(std::uint32_t device, std::int32_t* fd, std::uint64_t* bufferSize);
int APS5_VABI sceUsbStorageGetdentsRead(std::int32_t fd, void* buffer, std::uint64_t bufferSize, std::uint64_t* bytesRead);
int APS5_VABI sceUsbStorageGetdentsClose(std::int32_t fd);
}

namespace {

constexpr int ErrorInvalidArgument = static_cast<int>(0x80f40002);
constexpr std::size_t GuestHeaderBytes = 8;

void Require(bool value, const char* message) {
    if (value) return;
    std::fprintf(stderr, "guest usb storage: %s\n", message);
    std::abort();
}

template <typename F>
void RequireThrows(F call, const char* message) {
    try {
        call();
    } catch (const std::exception&) {
        return;
    }
    Require(false, message);
}

int eventCalls = 0;

void OnEvent(const UsbStorage::Event* event, void* userData) {
    (void)event;
    (void)userData;
    ++eventCalls;
}

void Write(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path);
    file << text;
}

#if defined(__linux__)

std::filesystem::path BuildFixture() {
    const auto root = std::filesystem::temp_directory_path() / "anyps5-usb-storage-fixture";
    std::filesystem::remove_all(root);
    const auto sys = root / "sys";

    const auto usb = sys / "devices" / "pci0000:00" / "0000:00:14.0" / "usb1" / "1-1";
    Write(usb / "idVendor", "048d\n");
    Write(usb / "idProduct", "1234\n");
    Write(usb / "manufacturer", "Kingston\n");
    Write(usb / "product", "DataTraveler\n");
    Write(usb / "serial", "0123456789\n");
    const auto block = usb / "1-1:1.0" / "host0" / "target0:0:0" / "1:0:0:0" / "block" / "sda";
    std::filesystem::create_directories(block);

    const auto stick = sys / "block" / "sda";
    std::filesystem::create_directories(stick);
    std::filesystem::create_symlink("../../devices/pci0000:00/0000:00:14.0/usb1/1-1/1-1:1.0/host0/target0:0:0/1:0:0:0/block/sda", stick / "device");
    Write(stick / "size", "1953525168\n");
    Write(stick / "dev", "8:0\n");

    const auto nvme = sys / "devices" / "pci0000:00" / "0000:00:02.3" / "0000:04:00.0" / "nvme" / "nvme0";
    std::filesystem::create_directories(nvme);
    const auto disk = sys / "block" / "nvme0n1";
    std::filesystem::create_directories(disk);
    std::filesystem::create_symlink("../../devices/pci0000:00/0000:00:02.3/0000:04:00.0/nvme/nvme0", disk / "device");
    Write(disk / "size", "1000215216\n");
    Write(disk / "dev", "259:0\n");

    const auto compressed = sys / "block" / "zram0";
    std::filesystem::create_directories(compressed);
    std::filesystem::create_directories(sys / "devices" / "virtual" / "block" / "zram0" / "device");
    std::filesystem::create_symlink("../../devices/virtual/block/zram0/device", compressed / "device");
    Write(compressed / "size", "16384\n");
    Write(compressed / "dev", "251:0\n");

    const auto mountInfo = root / "mountinfo";
    Write(mountInfo,
          "27 1 259:2 / /mnt rw,relatime shared:2 - btrfs /dev/nvme0n1p2 rw,ssd\n"
          "36 35 8:0 / /media/user/USB rw,relatime shared:1 - ext4 /dev/sda rw\n"
          "41 25 0:44 / /run/user/1000 rw,nosuid,nodev,relatime shared:9 - tmpfs tmpfs rw\n");
    return root;
}

std::string Field(const std::vector<unsigned char>& report, std::size_t offset) {
    const auto* name = reinterpret_cast<const char*>(report.data() + offset);
    return std::string(name, ::strnlen(name, UsbStorage::NameLength));
}

void CheckFixture() {
    const auto root = BuildFixture();
    const auto sys = (root / "sys").string();
    const auto mountInfo = (root / "mountinfo").string();
    ::setenv("APS5_USB_STORAGE_SYS_ROOT", sys.c_str(), 1);
    ::setenv("APS5_USB_STORAGE_MOUNT_INFO", mountInfo.c_str(), 1);

    Require(sceUsbStorageInit(nullptr) == 0, "init failed");

    std::uint32_t ids[UsbStorage::MaxDevices] = {};
    std::int32_t count = -1;
    Require(sceUsbStorageGetDeviceList(ids, &count) == 0, "getDeviceList failed");
    Require(count == 1, "the scan did not find exactly the usb device");
    Require(ids[0] == 0, "the first device did not get the id 0");

    std::vector<char> mountPoint(512, 'x');
    Require(sceUsbStorageGetMountPointOfShellCore(ids[0], mountPoint.data()) == 0, "getMountPoint failed");
    Require(std::string(mountPoint.data()) == "/media/user/USB", "the mount point is wrong");

    std::vector<unsigned char> report(UsbStorage::InfoSize, 0xff);
    Require(sceUsbStorageGetDeviceInfo(ids[0], report.data()) == 0, "getDeviceInfo failed");
    Require(Field(report, UsbStorage::VendorOffset) == "Kingston", "the vendor is wrong");
    Require(Field(report, UsbStorage::ProductOffset) == "DataTraveler", "the product is wrong");
    Require(Field(report, UsbStorage::SerialOffset) == "0123456789", "the serial is wrong");
    std::uint64_t volumeSize = 0;
    std::memcpy(&volumeSize, report.data(), sizeof(volumeSize));
    Require(volumeSize == 1953525168ull * 512ull, "the volume size is wrong");

    std::uint64_t mountId = 0;
    Require(sceUsbStorageRequestMap(ids[0], "/mnt/usb0", 0, 0, mountPoint.data(), &mountId, nullptr, 0) == 0,
            "requestMap failed for a listed device");
    Require(mountId != 0, "requestMap did not report a mount id");

    std::int32_t handle = 0;
    std::uint64_t bufferSize = 0;
    Require(sceUsbStorageGetdentsOpen(ids[0], &handle, &bufferSize) == ErrorInvalidArgument,
            "getdentsOpen opened a directory that does not exist");

    Require(sceUsbStorageTerm() == 0, "term failed");
    ::unsetenv("APS5_USB_STORAGE_SYS_ROOT");
    ::unsetenv("APS5_USB_STORAGE_MOUNT_INFO");
    std::filesystem::remove_all(root);
}

#endif

}

int main() {
#if defined(__linux__)
    CheckFixture();
#endif

    Require(sceUsbStorageInit(nullptr) == 0, "init failed");

    std::uint32_t ids[UsbStorage::MaxDevices] = {};
    std::int32_t count = -1;
    Require(sceUsbStorageGetDeviceList(ids, &count) == 0, "getDeviceList failed");
    Require(count >= 0 && count <= UsbStorage::MaxDevices, "getDeviceList reported an impossible count");

    std::vector<char> mountPoint(512, 'x');
    std::vector<unsigned char> report(UsbStorage::InfoSize, 0xff);
    for (std::int32_t index = 0; index < count; ++index) {
        const auto device = ids[index];
        Require(sceUsbStorageIsExist(device) == 1, "a listed device does not exist");
        Require(sceUsbStorageGetMountPointOfShellCore(device, mountPoint.data()) == 0, "getMountPoint failed");
        Require(sceUsbStorageGetDeviceInfo(device, report.data()) == 0, "getDeviceInfo failed");
        Require(report[UsbStorage::VendorOffset + UsbStorage::NameLength - 1] == 0, "the vendor is not terminated");
        Require(report[UsbStorage::ProductOffset + UsbStorage::NameLength - 1] == 0, "the product is not terminated");
        Require(report[UsbStorage::SerialOffset + UsbStorage::NameLength - 1] == 0, "the serial is not terminated");
    }

    Require(sceUsbStorageIsExist(0xFFFFu) == 0, "an unknown device exists");
    Require(sceUsbStorageGetDeviceInfo(0xFFFFu, report.data()) == ErrorInvalidArgument, "getDeviceInfo accepted an unknown device");
    Require(sceUsbStorageGetMountPointOfShellCore(0xFFFFu, mountPoint.data()) == ErrorInvalidArgument,
            "getMountPoint accepted an unknown device");
    std::int32_t handle = 0;
    std::uint64_t bufferSize = 0;
    Require(sceUsbStorageGetdentsOpen(0xFFFFu, &handle, &bufferSize) == ErrorInvalidArgument,
            "getdentsOpen accepted an unknown device");

    Require(sceUsbStorageRegisterCallback(UsbStorage::MountEvent, reinterpret_cast<void*>(&OnEvent), nullptr) == 0,
            "registerCallback failed");
    Require(sceUsbStorageUnregisterCallback(UsbStorage::MountEvent, reinterpret_cast<void*>(&OnEvent)) == 0,
            "unregisterCallback failed");
    RequireThrows([] { sceUsbStorageRegisterCallback(UsbStorage::MountEvent, nullptr, nullptr); },
                  "registerCallback accepted a null callback");
    RequireThrows([] { sceUsbStorageRegisterCallback(99, reinterpret_cast<void*>(&OnEvent), nullptr); },
                  "registerCallback accepted an unknown event type");
    Require(eventCalls == 0, "a callback ran without a device");

    Require(sceUsbStorageTerm() == 0, "term failed");
}
