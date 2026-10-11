#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#ifdef __linux__
#include <linux/if_packet.h>
#include <unistd.h>
#endif
#include <cerrno>
#endif
#include "prx/libc/include/general/VabiMacros.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

extern "C" int* APS5_VABI __error_nid_postfix();

struct GuestInterfaceAddress {
    GuestInterfaceAddress* next;
    char* name;
    std::uint32_t flags;
    void* address;
    void* netmask;
    void* destination;
    void* data;
};
static_assert(sizeof(GuestInterfaceAddress) == 56);
static_assert(offsetof(GuestInterfaceAddress, flags) == 16);
static_assert(offsetof(GuestInterfaceAddress, address) == 24);
static_assert(offsetof(GuestInterfaceAddress, netmask) == 32);
static_assert(offsetof(GuestInterfaceAddress, destination) == 40);
static_assert(offsetof(GuestInterfaceAddress, data) == 48);

namespace {
constexpr std::uint32_t guestUp = 0x1;
constexpr std::uint32_t guestBroadcast = 0x2;
constexpr std::uint32_t guestLoopback = 0x8;
constexpr std::uint32_t guestPointToPoint = 0x10;
constexpr std::uint32_t guestRunning = 0x40;
constexpr std::uint32_t guestMulticast = 0x8000;
constexpr std::size_t guestNameCapacity = 16;
constexpr int guestAfInet = 2;
constexpr int guestAfInet6 = 28;
constexpr int guestAfLink = 12;
constexpr std::size_t sockaddrDlBytes = 54;
constexpr std::size_t ifDataBytes = 152;
constexpr std::uint8_t iftEther = 6;
constexpr std::uint8_t iftLoopback = 24;
constexpr std::uint8_t ethernetHeader = 14;

struct Address {
    int family;
    std::vector<std::uint8_t> address;
    std::vector<std::uint8_t> netmask;
    std::vector<std::uint8_t> destination;
};

struct Entry {
    std::string name;
    std::uint32_t flags = 0;
    bool hasLink = false;
    std::uint16_t index = 0;
    std::uint8_t addrlen = 0;
    std::uint8_t type = 0;
    std::uint32_t mtu = 0;
    std::uint8_t mac[8] = {};
    std::vector<Address> v4;
    std::vector<Address> v6;
};

std::size_t Aligned(std::size_t size) {
    return (size + 7) & ~static_cast<std::size_t>(7);
}

std::string CheckedName(const char* name) {
    const std::string checked(name);
    if (checked.empty() || checked.size() >= guestNameCapacity)
        throw std::runtime_error("getifaddrs: host interface name does not fit the guest's 16-byte limit: " + checked);
    return checked;
}

std::vector<std::uint8_t> GuestIpv4(const std::uint8_t* bytes) {
    std::vector<std::uint8_t> guest(16, 0);
    guest[0] = 16;
    guest[1] = 2;
    std::memcpy(guest.data() + 4, bytes, 4);
    return guest;
}

std::vector<std::uint8_t> GuestIpv6(const std::uint8_t* bytes, std::uint32_t scope) {
    std::vector<std::uint8_t> guest(28, 0);
    guest[0] = 28;
    guest[1] = 28;
    std::memcpy(guest.data() + 8, bytes, 16);
    std::memcpy(guest.data() + 24, &scope, 4);
    return guest;
}

std::vector<std::uint8_t> BroadcastAddress(const std::vector<std::uint8_t>& address, const std::vector<std::uint8_t>& netmask) {
    std::vector<std::uint8_t> broadcast = address;
    for (std::size_t index = 4; index < 8; ++index)
        broadcast[index] = static_cast<std::uint8_t>(address[index] | static_cast<std::uint8_t>(~netmask[index]));
    return broadcast;
}

std::vector<std::uint8_t> GuestSockaddrDl(const Entry& entry) {
    std::vector<std::uint8_t> blob(sockaddrDlBytes, 0);
    const std::size_t nlen = std::min(entry.name.size(), std::size_t(46 - entry.addrlen));
    blob[0] = static_cast<std::uint8_t>(sockaddrDlBytes);
    blob[1] = static_cast<std::uint8_t>(guestAfLink);
    blob[2] = static_cast<std::uint8_t>(entry.index & 0xff);
    blob[3] = static_cast<std::uint8_t>((entry.index >> 8) & 0xff);
    blob[4] = entry.type;
    blob[5] = static_cast<std::uint8_t>(nlen);
    blob[6] = entry.addrlen;
    if (nlen != 0) std::memcpy(blob.data() + 8, entry.name.c_str(), nlen);
    if (entry.addrlen != 0) std::memcpy(blob.data() + 8 + nlen, entry.mac, entry.addrlen);
    return blob;
}

std::vector<std::uint8_t> GuestIfData(const Entry& entry) {
    std::vector<std::uint8_t> blob(ifDataBytes, 0);
    blob[0] = entry.type;
    blob[2] = entry.addrlen;
    blob[3] = (entry.type == iftEther ? ethernetHeader : 0);
    blob[4] = (entry.flags & guestUp) ? 1 : 0;
    blob[6] = static_cast<std::uint8_t>(ifDataBytes & 0xff);
    blob[7] = static_cast<std::uint8_t>((ifDataBytes >> 8) & 0xff);
    blob[8] = static_cast<std::uint8_t>(entry.mtu & 0xff);
    blob[9] = static_cast<std::uint8_t>((entry.mtu >> 8) & 0xff);
    blob[10] = static_cast<std::uint8_t>((entry.mtu >> 16) & 0xff);
    blob[11] = static_cast<std::uint8_t>((entry.mtu >> 24) & 0xff);
    return blob;
}

void SetType(Entry& entry) {
    entry.type = (entry.flags & guestLoopback) ? iftLoopback : iftEther;
}

#ifdef _WIN32
std::uint32_t InterfaceFlags(const IP_ADAPTER_ADDRESSES* adapter) {
    std::uint32_t flags = 0;
    if (adapter->OperStatus == IfOperStatusUp) flags |= guestUp | guestRunning;
    if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) flags |= guestLoopback;
    else if (adapter->IfType != IF_TYPE_PPP && adapter->IfType != IF_TYPE_TUNNEL) flags |= guestBroadcast;
    if (!(adapter->Flags & IP_ADAPTER_NO_MULTICAST)) flags |= guestMulticast;
    return flags;
}

std::string InterfaceName(const IP_ADAPTER_ADDRESSES* adapter) {
    char name[NDIS_IF_MAX_STRING_SIZE + 1] = {};
    const auto status = ConvertInterfaceLuidToNameA(&adapter->Luid, name, sizeof(name));
    if (status != NO_ERROR) throw std::runtime_error("getifaddrs: ConvertInterfaceLuidToNameA failed with " + std::to_string(status));
    if (std::strlen(name) < guestNameCapacity) return CheckedName(name);
    return "if" + std::to_string(adapter->IfIndex ? adapter->IfIndex : adapter->Ipv6IfIndex);
}

std::vector<std::uint8_t> PrefixMask(int family, unsigned prefix) {
    const unsigned bits = family == AF_INET ? 32 : 128;
    if (prefix > bits) prefix = bits;
    std::uint8_t mask[16] = {};
    for (unsigned bit = 0; bit < prefix; ++bit) mask[bit / 8] |= static_cast<std::uint8_t>(0x80 >> (bit % 8));
    return family == AF_INET ? GuestIpv4(mask) : GuestIpv6(mask, 0);
}

int HostEntries(std::vector<Entry>& entries) {
    const ULONG options = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG size = 0;
    std::vector<std::uint8_t> storage;
    ULONG result = GetAdaptersAddresses(AF_UNSPEC, options, nullptr, nullptr, &size);
    for (unsigned calls = 1; result == ERROR_BUFFER_OVERFLOW && calls < 4; ++calls) {
        storage.resize(size);
        result = GetAdaptersAddresses(AF_UNSPEC, options, nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(storage.data()), &size);
    }
    if (result == ERROR_NO_DATA) return 0;
    if (result == ERROR_BUFFER_OVERFLOW) { *__error_nid_postfix() = 55; return -1; }
    if (result == ERROR_NOT_ENOUGH_MEMORY) { *__error_nid_postfix() = 12; return -1; }
    if (result != NO_ERROR) throw std::runtime_error("getifaddrs: GetAdaptersAddresses failed with " + std::to_string(result));
    for (auto* adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(storage.data()); adapter; adapter = adapter->Next) {
        Entry entry;
        entry.name = InterfaceName(adapter);
        entry.flags = InterfaceFlags(adapter);
        entry.hasLink = true;
        entry.index = static_cast<std::uint16_t>(adapter->IfIndex ? adapter->IfIndex : adapter->Ipv6IfIndex);
        if (adapter->PhysicalAddressLength <= 8) {
            entry.addrlen = static_cast<std::uint8_t>(adapter->PhysicalAddressLength);
            if (entry.addrlen != 0) std::memcpy(entry.mac, adapter->PhysicalAddress, entry.addrlen);
        }
        entry.mtu = static_cast<std::uint32_t>(adapter->Mtu);
        SetType(entry);
        for (auto* unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next) {
            const sockaddr* address = unicast->Address.lpSockaddr;
            if (!address || (address->sa_family != AF_INET && address->sa_family != AF_INET6)) continue;
            Address addr;
            addr.family = address->sa_family == AF_INET ? guestAfInet : guestAfInet6;
            addr.address = address->sa_family == AF_INET
                ? GuestIpv4(reinterpret_cast<const std::uint8_t*>(&reinterpret_cast<const sockaddr_in*>(address)->sin_addr))
                : GuestIpv6(reinterpret_cast<const std::uint8_t*>(&reinterpret_cast<const sockaddr_in6*>(address)->sin6_addr), reinterpret_cast<const sockaddr_in6*>(address)->sin6_scope_id);
            addr.netmask = PrefixMask(address->sa_family, unicast->OnLinkPrefixLength);
            if (address->sa_family == AF_INET && (entry.flags & guestBroadcast))
                addr.destination = BroadcastAddress(addr.address, addr.netmask);
            (address->sa_family == AF_INET ? entry.v4 : entry.v6).push_back(std::move(addr));
        }
        entries.push_back(std::move(entry));
    }
    return 0;
}
#else
std::uint32_t InterfaceFlags(unsigned int host) {
    constexpr std::pair<unsigned int, std::uint32_t> flags[] = {
        {IFF_UP, guestUp}, {IFF_BROADCAST, guestBroadcast}, {IFF_LOOPBACK, guestLoopback},
        {IFF_POINTOPOINT, guestPointToPoint}, {IFF_RUNNING, guestRunning}, {IFF_NOARP, 0x80},
        {IFF_PROMISC, 0x100}, {IFF_ALLMULTI, 0x200}, {IFF_MULTICAST, guestMulticast},
    };
    std::uint32_t guest = 0;
    for (const auto& [hostFlag, guestFlag] : flags)
        if (host & hostFlag) guest |= guestFlag;
    return guest;
}

Entry& FindOrAdd(std::vector<Entry>& entries, const char* name, std::uint32_t flags) {
    for (auto& entry : entries)
        if (entry.name == name) return entry;
    entries.push_back(Entry());
    auto& entry = entries.back();
    entry.name = CheckedName(name);
    entry.flags = flags;
    return entry;
}

int HostEntries(std::vector<Entry>& entries) {
    ifaddrs* host = nullptr;
    if (::getifaddrs(&host) != 0) {
        const int hostError = errno;
        switch (hostError) {
        case ENOMEM: *__error_nid_postfix() = 12; return -1;
        case ENOBUFS: *__error_nid_postfix() = 55; return -1;
        case EMFILE: *__error_nid_postfix() = 24; return -1;
        case ENFILE: *__error_nid_postfix() = 23; return -1;
        default: throw std::runtime_error(std::string("getifaddrs: host enumeration failed: ") + std::strerror(hostError));
        }
    }
    try {
        for (auto* item = host; item; item = item->ifa_next) {
            if (!item->ifa_name || !item->ifa_addr) continue;
            const int family = item->ifa_addr->sa_family;
            const std::uint32_t flags = InterfaceFlags(item->ifa_flags);
#ifdef __linux__
            if (family == AF_PACKET) {
                const auto* link = reinterpret_cast<const sockaddr_ll*>(item->ifa_addr);
                Entry& entry = FindOrAdd(entries, item->ifa_name, flags);
                entry.hasLink = true;
                entry.index = static_cast<std::uint16_t>(link->sll_ifindex);
                if (link->sll_halen <= 8) {
                    entry.addrlen = static_cast<std::uint8_t>(link->sll_halen);
                    if (entry.addrlen != 0) std::memcpy(entry.mac, link->sll_addr, entry.addrlen);
                }
                continue;
            }
#endif
            if (family != AF_INET && family != AF_INET6) continue;
            if (!item->ifa_netmask) throw std::runtime_error(std::string("getifaddrs: host address without a netmask on ") + item->ifa_name);
            Entry& entry = FindOrAdd(entries, item->ifa_name, flags);
            Address addr;
            if (family == AF_INET) {
                addr.family = guestAfInet;
                addr.address = GuestIpv4(reinterpret_cast<const std::uint8_t*>(&reinterpret_cast<const sockaddr_in*>(item->ifa_addr)->sin_addr));
                addr.netmask = GuestIpv4(reinterpret_cast<const std::uint8_t*>(&reinterpret_cast<const sockaddr_in*>(item->ifa_netmask)->sin_addr));
                if (item->ifa_flags & IFF_POINTOPOINT) {
                    if (item->ifa_dstaddr && item->ifa_dstaddr->sa_family == AF_INET)
                        addr.destination = GuestIpv4(reinterpret_cast<const std::uint8_t*>(&reinterpret_cast<const sockaddr_in*>(item->ifa_dstaddr)->sin_addr));
                    else entry.flags &= ~guestPointToPoint;
                } else if (item->ifa_flags & IFF_BROADCAST) {
                    addr.destination = item->ifa_broadaddr && item->ifa_broadaddr->sa_family == AF_INET
                        ? GuestIpv4(reinterpret_cast<const std::uint8_t*>(&reinterpret_cast<const sockaddr_in*>(item->ifa_broadaddr)->sin_addr))
                        : BroadcastAddress(addr.address, addr.netmask);
                }
                entry.v4.push_back(std::move(addr));
            } else {
                addr.family = guestAfInet6;
                addr.address = GuestIpv6(reinterpret_cast<const std::uint8_t*>(&reinterpret_cast<const sockaddr_in6*>(item->ifa_addr)->sin6_addr), reinterpret_cast<const sockaddr_in6*>(item->ifa_addr)->sin6_scope_id);
                addr.netmask = GuestIpv6(reinterpret_cast<const std::uint8_t*>(&reinterpret_cast<const sockaddr_in6*>(item->ifa_netmask)->sin6_addr), 0);
                entry.v6.push_back(std::move(addr));
            }
        }
    } catch (...) {
        ::freeifaddrs(host);
        throw;
    }
    ::freeifaddrs(host);
#ifdef __linux__
    const int descriptor = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (descriptor >= 0) {
        for (auto& entry : entries) {
            ifreq info{};
            std::strncpy(info.ifr_name, entry.name.c_str(), IFNAMSIZ - 1);
            if (::ioctl(descriptor, SIOCGIFMTU, &info) == 0) entry.mtu = static_cast<std::uint32_t>(info.ifr_mtu);
        }
        ::close(descriptor);
    }
#endif
    for (auto& entry : entries)
        SetType(entry);
    return 0;
}
#endif
}

extern "C" {

int APS5_VABI getifaddrs_nid_postfix(GuestInterfaceAddress** list) {
    if (!list) { *__error_nid_postfix() = 14; return -1; }
    try {
        std::vector<Entry> entries;
        if (HostEntries(entries) != 0) return -1;
        std::size_t itemCount = 0;
        std::size_t size = 0;
        for (const auto& entry : entries) {
            const std::size_t nameSize = Aligned(entry.name.size() + 1);
            if (entry.hasLink) {
                ++itemCount;
                size += nameSize + Aligned(sockaddrDlBytes) + Aligned(ifDataBytes);
            }
            for (const auto& address : entry.v4) {
                ++itemCount;
                size += nameSize + Aligned(address.address.size()) + Aligned(address.netmask.size()) + (address.destination.empty() ? 0 : Aligned(address.destination.size()));
            }
            for (const auto& address : entry.v6) {
                ++itemCount;
                size += nameSize + Aligned(address.address.size()) + Aligned(address.netmask.size()) + (address.destination.empty() ? 0 : Aligned(address.destination.size()));
            }
        }
        if (itemCount == 0) {
            *list = nullptr;
            return 0;
        }
        size += itemCount * sizeof(GuestInterfaceAddress);
        auto* block = static_cast<std::uint8_t*>(std::malloc(size));
        if (!block) {
            *__error_nid_postfix() = 12;
            return -1;
        }
        auto* items = reinterpret_cast<GuestInterfaceAddress*>(block);
        std::size_t offset = itemCount * sizeof(GuestInterfaceAddress);
        const auto place = [&](const void* source, std::size_t length) {
            void* target = block + offset;
            std::memcpy(target, source, length);
            offset += Aligned(length);
            return target;
        };
        std::size_t index = 0;
        const auto emit = [&](const Entry& entry, const std::vector<std::uint8_t>& address, const std::vector<std::uint8_t>& netmask, const std::vector<std::uint8_t>& destination, const std::vector<std::uint8_t>& data) {
            GuestInterfaceAddress& item = items[index];
            item.next = index + 1 < itemCount ? &items[index + 1] : nullptr;
            item.name = static_cast<char*>(place(entry.name.c_str(), entry.name.size() + 1));
            std::uint32_t flags = entry.flags;
            if (data.empty() && destination.empty()) flags &= ~guestPointToPoint;
            item.flags = flags;
            item.address = place(address.data(), address.size());
            item.netmask = netmask.empty() ? nullptr : place(netmask.data(), netmask.size());
            item.destination = destination.empty() ? nullptr : place(destination.data(), destination.size());
            item.data = data.empty() ? nullptr : place(data.data(), data.size());
            ++index;
        };
        for (const auto& entry : entries) {
            if (entry.hasLink) emit(entry, GuestSockaddrDl(entry), {}, {}, GuestIfData(entry));
            for (const auto& address : entry.v4) emit(entry, address.address, address.netmask, address.destination, {});
            for (const auto& address : entry.v6) emit(entry, address.address, address.netmask, address.destination, {});
        }
        *list = items;
        return 0;
    } catch (const std::bad_alloc&) { *__error_nid_postfix() = 12; return -1; }
}

void APS5_VABI freeifaddrs_nid_postfix(GuestInterfaceAddress* list) {
    std::free(list);
}

}
