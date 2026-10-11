#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#ifdef __linux__
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif
#endif
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

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

extern "C" {
int APS5_VABI getifaddrs_nid_postfix(GuestInterfaceAddress** list);
void APS5_VABI freeifaddrs_nid_postfix(GuestInterfaceAddress* list);
int* APS5_VABI __error_nid_postfix();
}

static void Require(bool value) { if (!value) std::abort(); }

constexpr std::uint32_t knownFlags = 0x1 | 0x2 | 0x8 | 0x10 | 0x40 | 0x80 | 0x100 | 0x200 | 0x8000;
constexpr std::uint32_t loopbackFlag = 0x8;

struct HostLink {
    std::string name;
    std::uint32_t flags;
    std::uint16_t index;
    std::uint8_t type;
    std::uint32_t mtu;
    std::uint8_t mac[8];
    std::uint8_t addrlen;
    bool consumed;
};

struct HostAddress {
    std::string name;
    int family;
    std::uint8_t address[16];
    std::uint8_t netmask[16];
    std::uint32_t scope;
    bool consumed;
};

struct HostSnapshot {
    std::vector<HostLink> link;
    std::vector<HostAddress> address;
};

static std::uint32_t FlagsFromHost(unsigned int host) {
    // Must mirror libc/src/Interfaces.cpp InterfaceFlags exactly (same IFF_* -> guest
    // mapping) so the snapshot's flags equal the guest's flags on every platform.
    const std::pair<unsigned int, std::uint32_t> map[] = {
        {IFF_UP, 0x1}, {IFF_BROADCAST, 0x2}, {IFF_LOOPBACK, 0x8}, {IFF_POINTOPOINT, 0x10},
        {IFF_RUNNING, 0x40}, {IFF_NOARP, 0x80}, {IFF_PROMISC, 0x100}, {IFF_ALLMULTI, 0x200},
        {IFF_MULTICAST, 0x8000},
    };
    std::uint32_t guest = 0;
    for (const auto& [bit, flag] : map)
        if (host & bit) guest |= flag;
    return guest;
}

static void AddHostAddress(HostSnapshot& snapshot, const char* name, int family, const sockaddr* address, const sockaddr* netmask) {
    HostAddress entry;
    entry.name = name;
    entry.family = family;
    entry.scope = 0;
    entry.consumed = false;
    std::memset(entry.address, 0, sizeof(entry.address));
    std::memset(entry.netmask, 0, sizeof(entry.netmask));
    if (family == AF_INET) {
        std::memcpy(entry.address, &reinterpret_cast<const sockaddr_in*>(address)->sin_addr, 4);
        if (netmask) std::memcpy(entry.netmask, &reinterpret_cast<const sockaddr_in*>(netmask)->sin_addr, 4);
    } else {
        const auto* v6 = reinterpret_cast<const sockaddr_in6*>(address);
        std::memcpy(entry.address, &v6->sin6_addr, 16);
        entry.scope = v6->sin6_scope_id;
        if (netmask) std::memcpy(entry.netmask, &reinterpret_cast<const sockaddr_in6*>(netmask)->sin6_addr, 16);
    }
    snapshot.address.push_back(std::move(entry));
}

static HostSnapshot TakeHostSnapshot() {
    HostSnapshot snapshot;
#ifdef _WIN32
    const ULONG options = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG size = 0;
    std::vector<std::uint8_t> storage;
    ULONG result = GetAdaptersAddresses(AF_UNSPEC, options, nullptr, nullptr, &size);
    for (unsigned calls = 1; result == ERROR_BUFFER_OVERFLOW && calls < 4; ++calls) {
        storage.resize(size);
        result = GetAdaptersAddresses(AF_UNSPEC, options, nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(storage.data()), &size);
    }
    if (result == ERROR_NO_DATA) return snapshot;
    Require(result == NO_ERROR);
    for (auto* adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(storage.data()); adapter; adapter = adapter->Next) {
        char name[NDIS_IF_MAX_STRING_SIZE + 1] = {};
        const auto status = ConvertInterfaceLuidToNameA(&adapter->Luid, name, sizeof(name));
        Require(status == NO_ERROR);
        HostLink link;
        link.name = name;
        std::uint32_t flags = 0;
        if (adapter->OperStatus == IfOperStatusUp) flags |= 0x1 | 0x40;
        if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) flags |= loopbackFlag;
        else if (adapter->IfType != IF_TYPE_PPP && adapter->IfType != IF_TYPE_TUNNEL) flags |= 0x2;
        if (!(adapter->Flags & IP_ADAPTER_NO_MULTICAST)) flags |= 0x8000;
        link.flags = flags;
        link.index = static_cast<std::uint16_t>(adapter->IfIndex ? adapter->IfIndex : adapter->Ipv6IfIndex);
        link.type = (flags & loopbackFlag) ? 24 : 6;
        link.mtu = adapter->Mtu;
        link.addrlen = static_cast<std::uint8_t>(adapter->PhysicalAddressLength <= 8 ? adapter->PhysicalAddressLength : 0);
        std::memset(link.mac, 0, sizeof(link.mac));
        if (link.addrlen != 0) std::memcpy(link.mac, adapter->PhysicalAddress, link.addrlen);
        link.consumed = false;
        snapshot.link.push_back(std::move(link));
        for (auto* unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next) {
            const sockaddr* address = unicast->Address.lpSockaddr;
            if (!address || (address->sa_family != AF_INET && address->sa_family != AF_INET6)) continue;
            std::uint8_t mask[16] = {};
            const unsigned bits = address->sa_family == AF_INET ? 32 : 128;
            unsigned prefix = unicast->OnLinkPrefixLength;
            if (prefix > bits) prefix = bits;
            for (unsigned bit = 0; bit < prefix; ++bit) mask[bit / 8] |= static_cast<std::uint8_t>(0x80 >> (bit % 8));
            HostAddress entry;
            entry.name = name;
            entry.family = address->sa_family;
            entry.scope = 0;
            entry.consumed = false;
            std::memset(entry.address, 0, sizeof(entry.address));
            std::memset(entry.netmask, 0, sizeof(entry.netmask));
            if (address->sa_family == AF_INET) {
                std::memcpy(entry.address, &reinterpret_cast<const sockaddr_in*>(address)->sin_addr, 4);
                std::memcpy(entry.netmask, mask, 4);
            } else {
                std::memcpy(entry.address, &reinterpret_cast<const sockaddr_in6*>(address)->sin6_addr, 16);
                entry.scope = reinterpret_cast<const sockaddr_in6*>(address)->sin6_scope_id;
                std::memcpy(entry.netmask, mask, 16);
            }
            snapshot.address.push_back(std::move(entry));
        }
    }
    return snapshot;
#else
    ifaddrs* host = nullptr;
    Require(::getifaddrs(&host) == 0);
    try {
        for (auto* item = host; item; item = item->ifa_next) {
            if (!item->ifa_name || !item->ifa_addr) continue;
            const int family = item->ifa_addr->sa_family;
#ifdef __linux__
            if (family == AF_PACKET) {
                const auto* link = reinterpret_cast<const sockaddr_ll*>(item->ifa_addr);
                HostLink entry;
                entry.name = item->ifa_name;
                entry.flags = FlagsFromHost(item->ifa_flags);
                entry.index = static_cast<std::uint16_t>(link->sll_ifindex);
                entry.type = (entry.flags & loopbackFlag) ? 24 : 6;
                entry.mtu = 0;
                entry.addrlen = static_cast<std::uint8_t>(link->sll_halen <= 8 ? link->sll_halen : 0);
                std::memset(entry.mac, 0, sizeof(entry.mac));
                if (entry.addrlen != 0) std::memcpy(entry.mac, link->sll_addr, entry.addrlen);
                entry.consumed = false;
                snapshot.link.push_back(std::move(entry));
                continue;
            }
#endif
            if (family != AF_INET && family != AF_INET6) continue;
            Require(item->ifa_netmask != nullptr);
            AddHostAddress(snapshot, item->ifa_name, family, item->ifa_addr, item->ifa_netmask);
        }
#ifdef __linux__
        const int descriptor = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (descriptor >= 0) {
            for (auto& entry : snapshot.link) {
                ifreq info{};
                std::strncpy(info.ifr_name, entry.name.c_str(), IFNAMSIZ - 1);
                if (::ioctl(descriptor, SIOCGIFMTU, &info) == 0) entry.mtu = static_cast<std::uint32_t>(info.ifr_mtu);
            }
            ::close(descriptor);
        }
#endif
    } catch (...) {
        ::freeifaddrs(host);
        throw;
    }
    ::freeifaddrs(host);
#endif
    return snapshot;
}

int main() {
    const std::uint8_t loopbackAddress[] = {127, 0, 0, 1};
    const std::uint8_t loopbackMask[] = {255, 0, 0, 0};
    bool matched = false;
    for (unsigned attempt = 0; attempt < 3 && !matched; ++attempt) {
        HostSnapshot host = TakeHostSnapshot();
        GuestInterfaceAddress* list = nullptr;
        Require(getifaddrs_nid_postfix(&list) == 0);
        std::size_t items = 0;
        std::size_t linkItems = 0;
        bool valid = true;
        bool loopback = false;
        for (auto* item = list; item; item = item->next) {
            Require(++items < 100000);
            Require(item->name != nullptr);
            const std::size_t nameLength = std::strlen(item->name);
            Require(nameLength >= 1 && nameLength <= 15);
            Require((item->flags & ~knownFlags) == 0);
            Require(item->address != nullptr);
            const auto* address = static_cast<const std::uint8_t*>(item->address);
            Require(reinterpret_cast<std::uintptr_t>(address) % 8 == 0);
            const auto* netmask = static_cast<const std::uint8_t*>(item->netmask);
            if (address[1] == 12) {
                Require(++linkItems <= host.link.size());
                const auto* data = static_cast<const std::uint8_t*>(item->data);
                Require(data != nullptr && reinterpret_cast<std::uintptr_t>(data) % 8 == 0);
                bool found = false;
                for (auto& expected : host.link) {
                    if (expected.consumed) continue;
                    if (std::strcmp(item->name, expected.name.c_str()) != 0) continue;
                    const std::uint16_t index = static_cast<std::uint16_t>(address[2] | (static_cast<std::uint16_t>(address[3]) << 8));
                    const std::size_t nlen = address[5];
                    const std::uint8_t alen = address[6];
                    const std::uint32_t mtu = static_cast<std::uint32_t>(data[8] | (static_cast<std::uint32_t>(data[9]) << 8) | (static_cast<std::uint32_t>(data[10]) << 16) | (static_cast<std::uint32_t>(data[11]) << 24));
                    if (index != expected.index || address[4] != expected.type || alen != expected.addrlen) continue;
                    if (nlen != expected.name.size()) continue;
                    if (std::memcmp(address + 8, expected.name.c_str(), nlen) != 0) continue;
                    if (std::memcmp(address + 8 + nlen, expected.mac, alen) != 0) continue;
                    if (data[0] != expected.type || data[2] != expected.addrlen || mtu != expected.mtu || item->flags != expected.flags) continue;
                    expected.consumed = true;
                    found = true;
                    break;
                }
                valid = valid && found;
                continue;
            }
            Require(address[1] == 2 || address[1] == 28);
            Require(netmask != nullptr && reinterpret_cast<std::uintptr_t>(netmask) % 8 == 0);
            Require(netmask[0] == address[0] && netmask[1] == address[1]);
            Require(address[2] == 0 && address[3] == 0 && netmask[2] == 0 && netmask[3] == 0);
            const bool ipv4 = address[1] == 2;
            const std::size_t offset = ipv4 ? 4 : 8;
            const std::size_t length = ipv4 ? 4 : 16;
            if (ipv4) {
                for (std::size_t index = 8; index < 16; ++index) Require(address[index] == 0 && netmask[index] == 0);
            } else {
                for (std::size_t index = 4; index < 8; ++index) Require(address[index] == 0 && netmask[index] == 0);
            }
            if (item->destination) {
                const auto* destination = static_cast<const std::uint8_t*>(item->destination);
                Require(reinterpret_cast<std::uintptr_t>(destination) % 8 == 0);
                Require(destination[0] == address[0] && destination[1] == address[1]);
            }
            if (ipv4 && (item->flags & 0x2)) Require(item->destination != nullptr);
            if (item->flags & 0x10) Require(item->destination != nullptr);
            bool found = false;
            for (auto& expected : host.address) {
                if (expected.consumed || expected.family != (ipv4 ? AF_INET : AF_INET6)) continue;
                if (std::strcmp(item->name, expected.name.c_str()) != 0) continue;
                if (std::memcmp(address + offset, expected.address, length) != 0) continue;
                if (std::memcmp(netmask + offset, expected.netmask, length) != 0) continue;
                if (!ipv4) {
                    std::uint32_t scope;
                    std::memcpy(&scope, address + 24, 4);
                    if (scope != expected.scope) continue;
                }
                expected.consumed = true;
                found = true;
                break;
            }
            valid = valid && found;
            if (ipv4 && std::memcmp(address + 4, loopbackAddress, 4) == 0)
                loopback = loopback || (std::memcmp(netmask + 4, loopbackMask, 4) == 0 && (item->flags & 0x9) == 0x9 && !(item->flags & 0x2));
        }
        for (const auto& entry : host.address) valid = valid && entry.consumed;
#ifdef __linux__
        for (const auto& entry : host.link) valid = valid && entry.consumed;
#endif
        valid = valid && items == host.link.size() + host.address.size();
        freeifaddrs_nid_postfix(list);
        matched = valid && loopback;
    }
    Require(matched);
    freeifaddrs_nid_postfix(nullptr);
    *__error_nid_postfix() = 0;
    Require(getifaddrs_nid_postfix(nullptr) == -1);
    Require(*__error_nid_postfix() == 14);
}
