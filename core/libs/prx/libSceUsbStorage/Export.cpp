#include "prx/libSceUsbStorage/include/UsbStorage.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestArena.hpp"
#include "SceTypes.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(__linux__)
#include <dirent.h>
#endif

namespace {

constexpr int ErrorInvalidArgument = static_cast<int>(0x80f40002);
constexpr std::size_t GuestHeaderBytes = 8;
constexpr std::size_t GuestMaxName = 255;
constexpr std::uint64_t RecommendedBuffer = 4096;

struct Registration {
    std::int32_t eventType = 0;
    UsbStorage::EventCallback callback = nullptr;
    void* userData = nullptr;
    std::uint32_t device = 0;
    std::string directory;
    bool anyDevice = true;
};

struct Invocation {
    UsbStorage::EventCallback callback = nullptr;
    void* userData = nullptr;
    UsbStorage::Event event;
};

std::mutex stateMutex;
bool initialized = false;
std::vector<UsbStorage::Device> knownDevices;
std::vector<Registration> registrations;
std::map<int, void*> directories;
int nextHandle = 1;

std::string ReadText(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) return {};
    std::string text;
    std::getline(file, text);
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();
    return text;
}

std::filesystem::path ResolvedDevice(const std::filesystem::path& link) {
    std::error_code error;
    if (!std::filesystem::exists(link, error)) return {};
    auto resolved = std::filesystem::weakly_canonical(link, error);
    if (error) return {};
    return resolved;
}

bool IsUsbMassStorage(const std::filesystem::path& resolved) {
    const auto text = resolved.string();
    if (text.find("/usb") == std::string::npos) return false;
    if (text.find("/devices/virtual/") != std::string::npos) return false;
    return true;
}

std::filesystem::path UsbDeviceDirectory(const std::filesystem::path& resolved) {
    auto current = resolved;
    for (int depth = 0; depth < 16; ++depth) {
        std::error_code error;
        if (std::filesystem::exists(current / "idVendor", error)) return current;
        if (!current.has_parent_path() || current.parent_path() == current) break;
        current = current.parent_path();
    }
    return {};
}

std::string Unescape(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    for (std::size_t index = 0; index < text.size(); ++index) {
        if (text[index] != '\\' || index + 3 >= text.size()) {
            result.push_back(text[index]);
            continue;
        }
        const auto digits = text.substr(index + 1, 3);
        if (digits[0] < '0' || digits[0] > '7' || digits[1] < '0' || digits[1] > '7' || digits[2] < '0' || digits[2] > '7') {
            result.push_back(text[index]);
            continue;
        }
        result.push_back(static_cast<char>(std::strtol(digits.c_str(), nullptr, 8)));
        index += 3;
    }
    return result;
}

std::string MountPointFor(const std::string& mountInfoPath, const std::string& name, const std::string& deviceNumbers) {
    std::ifstream file(mountInfoPath);
    if (!file) return {};
    std::string line;
    std::string bySource;
    std::string byNumbers;
    while (std::getline(file, line)) {
        std::vector<std::string> fields;
        std::string current;
        for (const char character : line) {
            if (character == ' ') {
                fields.push_back(current);
                current.clear();
                continue;
            }
            current.push_back(character);
        }
        fields.push_back(current);
        if (fields.size() < 10) continue;
        const auto& numbers = fields[2];
        const auto& mountPoint = fields[4];
        const auto& source = fields[8];
        if (bySource.empty() && source == "/dev/" + name) bySource = Unescape(mountPoint);
        if (byNumbers.empty() && !deviceNumbers.empty() && numbers == deviceNumbers) byNumbers = Unescape(mountPoint);
    }
    return bySource.empty() ? byNumbers : bySource;
}

int ScanDevices(UsbStorage::Device* devices, int capacity) {
    if (devices == nullptr || capacity <= 0) return 0;
    const char* sysRoot = ::getenv("APS5_USB_STORAGE_SYS_ROOT");
    const char* mountInfo = ::getenv("APS5_USB_STORAGE_MOUNT_INFO");
    const std::filesystem::path blockRoot = std::filesystem::path(sysRoot == nullptr ? "/sys" : sysRoot) / "block";
    const std::string mountInfoPath = mountInfo == nullptr ? "/proc/self/mountinfo" : mountInfo;
    std::vector<std::string> names;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(blockRoot, error)) {
        if (error) break;
        if (!entry.is_directory(error)) continue;
        names.push_back(entry.path().filename().string());
    }
    std::sort(names.begin(), names.end());
    int found = 0;
    for (const auto& name : names) {
        if (found >= capacity) break;
        const auto block = blockRoot / name;
        const auto resolved = ResolvedDevice(block / "device");
        if (resolved.empty() || !IsUsbMassStorage(resolved)) continue;
        UsbStorage::Device device;
        device.id = static_cast<std::uint32_t>(found);
        const auto usb = UsbDeviceDirectory(resolved);
        if (!usb.empty()) {
            device.vendor = ReadText(usb / "manufacturer");
            if (device.vendor.empty()) device.vendor = ReadText(usb / "vendor");
            device.product = ReadText(usb / "product");
            device.serial = ReadText(usb / "serial");
        }
        const auto sectors = ReadText(block / "size");
        if (!sectors.empty()) device.volumeSize = std::strtoull(sectors.c_str(), nullptr, 10) * 512u;
        device.mountPoint = MountPointFor(mountInfoPath, name, ReadText(block / "dev"));
        devices[found++] = device;
    }
    return found;
}

bool Matches(const Registration& registration, const UsbStorage::Event& event, const std::string& directory) {
    if (registration.eventType != static_cast<std::int32_t>(event.type)) return false;
    if (!registration.anyDevice && registration.device != event.deviceId) return false;
    if (!registration.directory.empty() && registration.directory != directory) return false;
    return true;
}

const UsbStorage::Device* FindDevice(const std::vector<UsbStorage::Device>& list, std::uint32_t id) {
    for (const auto& device : list) {
        if (device.id == id) return &device;
    }
    return nullptr;
}

void Refresh() {
    UsbStorage::Device current[UsbStorage::MaxDevices];
    const int count = ScanDevices(current, UsbStorage::MaxDevices);
    std::vector<Invocation> invocations;
    {
        std::lock_guard lock(stateMutex);
        for (int index = 0; index < count; ++index) {
            const auto& device = current[index];
            if (FindDevice(knownDevices, device.id) != nullptr) continue;
            for (const auto& registration : registrations) {
                if (!Matches(registration, {UsbStorage::MountEvent, device.id, device.flags}, device.mountPoint)) continue;
                invocations.push_back({registration.callback, registration.userData, {UsbStorage::MountEvent, device.id, device.flags}});
            }
        }
        for (const auto& device : knownDevices) {
            bool present = false;
            for (int index = 0; index < count; ++index) {
                if (current[index].id == device.id) present = true;
            }
            if (present) continue;
            for (const auto& registration : registrations) {
                if (!Matches(registration, {UsbStorage::UnmountEvent, device.id, device.flags}, device.mountPoint)) continue;
                invocations.push_back({registration.callback, registration.userData, {UsbStorage::UnmountEvent, device.id, device.flags}});
            }
        }
        knownDevices.assign(current, current + count);
    }
    for (const auto& invocation : invocations) invocation.callback(&invocation.event, invocation.userData);
}

void WriteString(char* destination, const std::string& text) {
    const GuestArena::HostWrite write(destination, text.size() + 1);
    if (!write.Open()) throw std::invalid_argument("sceUsbStorage: the output buffer is not writable");
    std::memcpy(destination, text.c_str(), text.size() + 1);
}

}

extern "C" {

int APS5_VABI sceUsbStorageInit(void* attr) {
    (void)attr;
    {
        std::lock_guard lock(stateMutex);
        initialized = true;
    }
    Refresh();
    return 0;
}

int APS5_VABI sceUsbStorageTerm(void) {
    std::lock_guard lock(stateMutex);
    initialized = false;
    knownDevices.clear();
    registrations.clear();
#if defined(__linux__)
    for (auto& entry : directories) ::closedir(static_cast<DIR*>(entry.second));
#endif
    directories.clear();
    return 0;
}

int APS5_VABI sceUsbStorageGetDeviceList(std::uint32_t* devices, std::int32_t* count) {
    Refresh();
    if (devices == nullptr || count == nullptr) throw std::invalid_argument("sceUsbStorageGetDeviceList: null output");
    std::uint32_t ids[UsbStorage::MaxDevices];
    int found = 0;
    {
        std::lock_guard lock(stateMutex);
        for (const auto& device : knownDevices) {
            if (found >= UsbStorage::MaxDevices) break;
            ids[found++] = device.id;
        }
    }
    const GuestArena::HostWrite write(devices, sizeof(ids));
    if (!write.Open()) throw std::invalid_argument("sceUsbStorageGetDeviceList: the device buffer is not writable");
    std::memcpy(devices, ids, sizeof(ids));
    *count = found;
    return 0;
}

int APS5_VABI sceUsbStorageGetMountPointOfShellCore(std::uint32_t device, char* mountPoint) {
    Refresh();
    if (mountPoint == nullptr) throw std::invalid_argument("sceUsbStorageGetMountPointOfShellCore: null output");
    std::string path;
    {
        std::lock_guard lock(stateMutex);
        const auto* found = FindDevice(knownDevices, device);
        if (found == nullptr) return ErrorInvalidArgument;
        path = found->mountPoint;
    }
    WriteString(mountPoint, path);
    return 0;
}

int APS5_VABI sceUsbStorageGetDeviceInfo(std::uint32_t device, void* info) {
    Refresh();
    if (info == nullptr) throw std::invalid_argument("sceUsbStorageGetDeviceInfo: null output");
    std::uint8_t report[UsbStorage::InfoSize] = {};
    {
        std::lock_guard lock(stateMutex);
        const auto* found = FindDevice(knownDevices, device);
        if (found == nullptr) return ErrorInvalidArgument;
        std::memcpy(report, &found->volumeSize, sizeof(found->volumeSize));
        const auto copy = [&](std::size_t offset, const std::string& text) {
            const auto length = std::min(text.size(), UsbStorage::NameLength - 1);
            std::memcpy(report + offset, text.data(), length);
        };
        copy(UsbStorage::VendorOffset, found->vendor);
        copy(UsbStorage::ProductOffset, found->product);
        copy(UsbStorage::SerialOffset, found->serial);
        std::memcpy(report + UsbStorage::FlagsOffset, &found->flags, sizeof(found->flags));
    }
    const GuestArena::HostWrite write(info, sizeof(report));
    if (!write.Open()) throw std::invalid_argument("sceUsbStorageGetDeviceInfo: the report buffer is not writable");
    std::memcpy(info, report, sizeof(report));
    return 0;
}

int APS5_VABI sceUsbStorageIsExist(std::uint32_t device) {
    Refresh();
    std::lock_guard lock(stateMutex);
    return FindDevice(knownDevices, device) != nullptr ? 1 : 0;
}

int APS5_VABI sceUsbStorageRequestMap(std::uint32_t device, const char* directory, std::int32_t flags,
                                      std::uint64_t volumeSize, char* mountPoint, std::uint64_t* mountId,
                                      const void* reserved, std::size_t reservedSize) {
    (void)directory;
    (void)flags;
    (void)volumeSize;
    (void)reserved;
    (void)reservedSize;
    Refresh();
    if (mountPoint == nullptr || mountId == nullptr) throw std::invalid_argument("sceUsbStorageRequestMap: null output");
    std::string path;
    {
        std::lock_guard lock(stateMutex);
        const auto* found = FindDevice(knownDevices, device);
        if (found == nullptr || found->mountPoint.empty()) return ErrorInvalidArgument;
        path = found->mountPoint;
    }
    WriteString(mountPoint, path);
    *mountId = static_cast<std::uint64_t>(device) + 1;
    return 0;
}

int APS5_VABI sceUsbStorageRequestUnmap(std::uint32_t device, const char* directory) {
    (void)device;
    (void)directory;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceUsbStorageRegisterCallback(std::int32_t eventType, void* callback, void* arg) {
    if (callback == nullptr) throw std::invalid_argument("sceUsbStorageRegisterCallback: null callback");
    if (eventType != UsbStorage::MountEvent && eventType != UsbStorage::UnmountEvent) {
        throw std::invalid_argument("sceUsbStorageRegisterCallback: unknown event type");
    }
    std::lock_guard lock(stateMutex);
    registrations.push_back({eventType, reinterpret_cast<UsbStorage::EventCallback>(callback), arg, 0, {}, true});
    return 0;
}

int APS5_VABI sceUsbStorageUnregisterCallback(std::int32_t eventType, void* callback) {
    std::lock_guard lock(stateMutex);
    registrations.erase(std::remove_if(registrations.begin(), registrations.end(), [&](const Registration& registration) {
        return registration.eventType == eventType && registration.callback == reinterpret_cast<UsbStorage::EventCallback>(callback);
    }), registrations.end());
    return 0;
}

int APS5_VABI sceUsbStorageRegisterCallbackForMapAvailable(std::uint32_t device, const char* directory, void* callback, void* arg) {
    if (callback == nullptr) throw std::invalid_argument("sceUsbStorageRegisterCallbackForMapAvailable: null callback");
    if (directory == nullptr) throw std::invalid_argument("sceUsbStorageRegisterCallbackForMapAvailable: null directory");
    std::lock_guard lock(stateMutex);
    registrations.push_back({UsbStorage::MountEvent, reinterpret_cast<UsbStorage::EventCallback>(callback), arg, device, directory, false});
    return 0;
}

int APS5_VABI sceUsbStorageUnregisterCallbackForMapAvailable(std::uint32_t device, const char* directory, void* callback) {
    (void)callback;
    std::lock_guard lock(stateMutex);
    registrations.erase(std::remove_if(registrations.begin(), registrations.end(), [&](const Registration& registration) {
        return registration.eventType == UsbStorage::MountEvent && registration.device == device && registration.directory == directory;
    }), registrations.end());
    return 0;
}

int APS5_VABI sceUsbStorageRegisterCallbackForMapUnavailable(std::uint32_t device, const char* directory, void* callback, void* arg) {
    if (callback == nullptr) throw std::invalid_argument("sceUsbStorageRegisterCallbackForMapUnavailable: null callback");
    if (directory == nullptr) throw std::invalid_argument("sceUsbStorageRegisterCallbackForMapUnavailable: null directory");
    std::lock_guard lock(stateMutex);
    registrations.push_back({UsbStorage::UnmountEvent, reinterpret_cast<UsbStorage::EventCallback>(callback), arg, device, directory, false});
    return 0;
}

int APS5_VABI sceUsbStorageUnregisterCallbackForMapUnavailable(std::uint32_t device, const char* directory, void* callback) {
    (void)callback;
    std::lock_guard lock(stateMutex);
    registrations.erase(std::remove_if(registrations.begin(), registrations.end(), [&](const Registration& registration) {
        return registration.eventType == UsbStorage::UnmountEvent && registration.device == device && registration.directory == directory;
    }), registrations.end());
    return 0;
}

int APS5_VABI sceUsbStorageGetdentsOpen(std::uint32_t device, std::int32_t* fd, std::uint64_t* bufferSize) {
    Refresh();
    if (fd == nullptr || bufferSize == nullptr) throw std::invalid_argument("sceUsbStorageGetdentsOpen: null output");
    std::string path;
    {
        std::lock_guard lock(stateMutex);
        const auto* found = FindDevice(knownDevices, device);
        if (found == nullptr || found->mountPoint.empty()) return ErrorInvalidArgument;
        path = found->mountPoint;
    }
#if defined(__linux__)
    DIR* handle = ::opendir(path.c_str());
    if (handle == nullptr) return ErrorInvalidArgument;
    int number = 0;
    {
        std::lock_guard lock(stateMutex);
        number = nextHandle++;
        directories[number] = handle;
    }
    *fd = number;
    *bufferSize = RecommendedBuffer;
    return 0;
#else
    (void)path;
    NotImplemented_nid_no_patch(__func__);
    return 0;
#endif
}

int APS5_VABI sceUsbStorageGetdentsRead(std::int32_t fd, void* buffer, std::uint64_t bufferSize, std::uint64_t* bytesRead) {
    if (buffer == nullptr || bytesRead == nullptr) throw std::invalid_argument("sceUsbStorageGetdentsRead: null output");
#if defined(__linux__)
    DIR* handle = nullptr;
    {
        std::lock_guard lock(stateMutex);
        const auto found = directories.find(fd);
        if (found == directories.end()) return ErrorInvalidArgument;
        handle = static_cast<DIR*>(found->second);
    }
    std::vector<unsigned char> records;
    while (const dirent* entry = ::readdir(handle)) {
        const std::size_t nameLength = std::strlen(entry->d_name);
        if (nameLength > GuestMaxName) continue;
        const auto record = (GuestHeaderBytes + nameLength + 1 + 3) & ~std::size_t{3};
        if (records.size() + record > bufferSize) break;
        const auto fileNumber = static_cast<std::uint32_t>(entry->d_ino);
        const auto recordLength = static_cast<std::uint16_t>(record);
        const auto nameBytes = static_cast<std::uint8_t>(nameLength);
        std::vector<unsigned char> encoded(record, 0);
        std::memcpy(encoded.data(), &fileNumber, sizeof(fileNumber));
        std::memcpy(encoded.data() + 4, &recordLength, sizeof(recordLength));
        encoded[6] = static_cast<unsigned char>(entry->d_type);
        encoded[7] = nameBytes;
        std::memcpy(encoded.data() + GuestHeaderBytes, entry->d_name, nameLength);
        records.insert(records.end(), encoded.begin(), encoded.end());
    }
    const GuestArena::HostWrite write(buffer, records.size());
    if (!write.Open()) throw std::invalid_argument("sceUsbStorageGetdentsRead: the buffer is not writable");
    if (!records.empty()) std::memcpy(buffer, records.data(), records.size());
    *bytesRead = records.size();
    return 0;
#else
    (void)fd;
    (void)buffer;
    (void)bufferSize;
    NotImplemented_nid_no_patch(__func__);
    return 0;
#endif
}

int APS5_VABI sceUsbStorageGetdentsClose(std::int32_t fd) {
#if defined(__linux__)
    std::lock_guard lock(stateMutex);
    const auto found = directories.find(fd);
    if (found == directories.end()) return ErrorInvalidArgument;
    ::closedir(static_cast<DIR*>(found->second));
    directories.erase(found);
    return 0;
#else
    (void)fd;
    NotImplemented_nid_no_patch(__func__);
    return 0;
#endif
}

}
