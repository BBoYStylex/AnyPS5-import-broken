#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <mutex>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr int INVALID_ARGUMENT = static_cast<int>(0x80f40002);
constexpr int NO_DEVICE = static_cast<int>(0x80f40004);
constexpr int CLIENT_NOT_VALID = static_cast<int>(0x80f40009);
constexpr std::uint32_t MAX_DEVICES = 8;

std::mutex usbMutex;
bool initialized = false;

std::string MountPath(std::uint32_t device) {
    return "/mnt/usb" + std::to_string(device);
}

bool DeviceExists(std::uint32_t device) {
    if (device >= MAX_DEVICES) return false;
    try {
        const auto path = ResolvePath_nid_no_patch(MountPath(device).c_str());
        return std::filesystem::is_directory(path);
    } catch (const std::filesystem::filesystem_error& error) {
        if (error.code() == std::errc::no_such_file_or_directory || error.code() == std::errc::not_a_directory) return false;
        throw;
    }
}

}

extern "C" {

int APS5_VABI sceUsbStorageInit(void* attr) {
    if (attr) throw std::runtime_error("sceUsbStorageInit: non-default thread attributes are not implemented");
    std::lock_guard lock(usbMutex);
    if (initialized) throw std::runtime_error("sceUsbStorageInit: repeated initialization is not implemented");
    initialized = true;
    return 0;
}
int APS5_VABI sceUsbStorageTerm(void) {
    std::lock_guard lock(usbMutex);
    if (!initialized) return CLIENT_NOT_VALID;
    initialized = false;
    return 0;
}
int APS5_VABI sceUsbStorageRegisterCallback(int32_t eventType, void* callback, void* arg) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceUsbStorageUnregisterCallback(int32_t eventType, void* callback) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceUsbStorageRegisterCallbackForMapAvailable(uint32_t device, const char* directory, void* callback, void* arg) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceUsbStorageUnregisterCallbackForMapAvailable(uint32_t device, const char* directory, void* callback) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceUsbStorageRegisterCallbackForMapUnavailable(uint32_t device, const char* directory, void* callback, void* arg) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceUsbStorageUnregisterCallbackForMapUnavailable(uint32_t device, const char* directory, void* callback) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceUsbStorageGetDeviceInfo(uint32_t device, void* info) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceUsbStorageGetDeviceList(uint32_t* devices, int32_t* count) {
    std::lock_guard lock(usbMutex);
    if (!initialized) return CLIENT_NOT_VALID;
    if (!devices || !count) return INVALID_ARGUMENT;
    std::array<std::uint32_t, MAX_DEVICES> found{};
    std::int32_t length = 0;
    for (std::uint32_t device = 0; device < MAX_DEVICES; ++device) {
        if (DeviceExists(device)) found[length++] = device;
    }
    std::copy_n(found.data(), length, devices);
    *count = length;
    return 0;
}
int APS5_VABI sceUsbStorageGetMountPointOfShellCore(uint32_t device, char* mountPoint) {
    std::lock_guard lock(usbMutex);
    if (!initialized) return CLIENT_NOT_VALID;
    if (!mountPoint) return INVALID_ARGUMENT;
    if (!DeviceExists(device)) return NO_DEVICE;
    const auto path = MountPath(device);
    std::memcpy(mountPoint, path.c_str(), path.size() + 1);
    return 0;
}
int APS5_VABI sceUsbStorageIsExist(uint32_t device) {
    std::lock_guard lock(usbMutex);
    if (!initialized) return CLIENT_NOT_VALID;
    return DeviceExists(device) ? 1 : 0;
}
int APS5_VABI sceUsbStorageRequestMap(uint32_t device, const char* directory, int32_t flags,
                                      uint64_t volumeSize, char* mountPoint, uint64_t* mountId,
                                      const void* reserved, size_t reservedSize) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceUsbStorageRequestUnmap(uint32_t device, const char* directory) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceUsbStorageGetdentsOpen(uint32_t device, int32_t* fd, uint64_t* bufferSize) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceUsbStorageGetdentsRead(int32_t fd, void* buffer, uint64_t bufferSize, uint64_t* bytesRead) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceUsbStorageGetdentsClose(int32_t fd) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
