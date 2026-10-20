#ifndef CORE_LIBS_PRX_LIBSCEUSBSTORAGE_INCLUDE_USBSTORAGE_HPP
#define CORE_LIBS_PRX_LIBSCEUSBSTORAGE_INCLUDE_USBSTORAGE_HPP

#include <cstddef>
#include <cstdint>
#include <string>

namespace UsbStorage {

constexpr std::size_t InfoSize = 0x338;
constexpr std::size_t NameLength = 256;
constexpr std::size_t VendorOffset = 0x16;
constexpr std::size_t ProductOffset = 0x120;
constexpr std::size_t SerialOffset = 0x228;
constexpr std::size_t FlagsOffset = 0x330;
constexpr int MaxDevices = 8;
constexpr std::int32_t MountEvent = 1;
constexpr std::int32_t UnmountEvent = 2;

struct Device {
    std::uint32_t id = 0;
    std::uint64_t volumeSize = 0;
    std::uint64_t flags = 0;
    std::string vendor;
    std::string product;
    std::string serial;
    std::string mountPoint;
};

struct Event {
    std::uint32_t type = 0;
    std::uint32_t deviceId = 0;
    std::uint64_t flags = 0;
};

using EventCallback = void (*)(const Event* event, void* userData);

}

#endif
