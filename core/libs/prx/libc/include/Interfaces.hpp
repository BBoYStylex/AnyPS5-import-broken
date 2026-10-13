#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_INTERFACES_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_INTERFACES_HPP

#include <cstddef>
#include <cstdint>

struct GuestInterfaceAddress {
    GuestInterfaceAddress* next;
    char* name;
    std::uint32_t flags;
    void* address;
    void* netmask;
    void* destination;
    void* data;
};

struct GuestInterfaceData {
    std::uint8_t type;
    std::uint8_t physical;
    std::uint8_t addressLength;
    std::uint8_t headerLength;
    std::uint8_t linkState;
    std::uint8_t virtualHostId;
    std::uint16_t dataLength;
    std::uint32_t mtu;
    std::uint32_t metric;
    std::uint64_t baudrate;
    std::uint64_t inputPackets;
    std::uint64_t inputErrors;
    std::uint64_t outputPackets;
    std::uint64_t outputErrors;
    std::uint64_t collisions;
    std::uint64_t inputBytes;
    std::uint64_t outputBytes;
    std::uint64_t inputMulticasts;
    std::uint64_t outputMulticasts;
    std::uint64_t inputDrops;
    std::uint64_t outputDrops;
    std::uint64_t unsupportedProtocols;
    std::uint64_t hardwareAssist;
    std::int64_t epoch;
    std::int64_t lastChangeSeconds;
    std::int64_t lastChangeMicroseconds;
};

static_assert(sizeof(GuestInterfaceAddress) == 56);
static_assert(offsetof(GuestInterfaceAddress, flags) == 16);
static_assert(offsetof(GuestInterfaceAddress, address) == 24);
static_assert(offsetof(GuestInterfaceAddress, netmask) == 32);
static_assert(offsetof(GuestInterfaceAddress, destination) == 40);
static_assert(offsetof(GuestInterfaceAddress, data) == 48);
static_assert(sizeof(GuestInterfaceData) == 152);
static_assert(offsetof(GuestInterfaceData, dataLength) == 6);
static_assert(offsetof(GuestInterfaceData, mtu) == 8);
static_assert(offsetof(GuestInterfaceData, baudrate) == 16);
static_assert(offsetof(GuestInterfaceData, inputPackets) == 24);
static_assert(offsetof(GuestInterfaceData, lastChangeSeconds) == 136);

#endif
