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
#include <unistd.h>
#if defined(__linux__)
#include <netpacket/packet.h>
#include <linux/if_link.h>
#include <net/if_arp.h>
#else
#include <net/if_dl.h>
#endif
#endif
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libc/include/Interfaces.hpp"
#include <cstdarg>
#include <cstdlib>
#include <cstring>

#define __error_nid_postfix FixtureError_nid_no_patch
#define getifaddrs_nid_postfix FixtureGetifaddrs_nid_no_patch
#define freeifaddrs_nid_postfix FixtureFreeifaddrs_nid_no_patch

static int guestError;
extern "C" int* APS5_VABI __error_nid_postfix() { return &guestError; }
static const unsigned char mac[] = {2, 3, 4, 5, 6, 7};
static bool includeAll = false;

#ifdef _WIN32
static ULONG FixtureAdapters(ULONG family, ULONG flags, PVOID, IP_ADAPTER_ADDRESSES* adapter, ULONG* size) {
    if (family != AF_UNSPEC) std::abort();
    includeAll = (flags & GAA_FLAG_INCLUDE_ALL_INTERFACES) != 0;
    if (!adapter || *size < sizeof(*adapter)) { *size = sizeof(*adapter); return ERROR_BUFFER_OVERFLOW; }
    *adapter = {};
    adapter->Length = sizeof(*adapter);
    adapter->Luid.Value = 7;
    adapter->IfIndex = 0;
    adapter->IfType = IF_TYPE_ETHERNET_CSMACD;
    adapter->OperStatus = IfOperStatusUp;
    adapter->Mtu = 9000;
    adapter->PhysicalAddressLength = sizeof(mac);
    std::memcpy(adapter->PhysicalAddress, mac, sizeof(mac));
    return NO_ERROR;
}
static NETIO_STATUS FixtureName(const NET_LUID*, PSTR name, SIZE_T capacity) {
    if (capacity < 6) return ERROR_INSUFFICIENT_BUFFER;
    std::strcpy(name, "noip0");
    return NO_ERROR;
}
static NETIO_STATUS FixtureRow(MIB_IF_ROW2* row) {
    row->InterfaceIndex = 7;
    row->Type = IF_TYPE_ETHERNET_CSMACD;
    row->Mtu = 9000;
    row->PhysicalAddressLength = sizeof(mac);
    std::memcpy(row->PhysicalAddress, mac, sizeof(mac));
    row->InUcastPkts = 13;
    row->OutUcastPkts = 17;
    row->InOctets = 101;
    row->OutOctets = 103;
    row->TransmitLinkSpeed = 1000000000;
    return NO_ERROR;
}
#define GetAdaptersAddresses FixtureAdapters
#define ConvertInterfaceLuidToNameA FixtureName
#define GetIfEntry2 FixtureRow
#else
static ifaddrs fixture;
#if defined(__linux__)
static sockaddr_ll linkAddress;
static rtnl_link_stats statistics;
#else
static sockaddr_dl linkAddress;
static if_data statistics;
#endif
static int FixtureAddresses(ifaddrs** list) {
    fixture = {};
    linkAddress = {};
    statistics = {};
    fixture.ifa_name = const_cast<char*>("noip0");
    fixture.ifa_flags = IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST;
    fixture.ifa_addr = reinterpret_cast<sockaddr*>(&linkAddress);
    fixture.ifa_data = &statistics;
#if defined(__linux__)
    linkAddress.sll_family = AF_PACKET;
    linkAddress.sll_ifindex = 7;
    linkAddress.sll_hatype = ARPHRD_ETHER;
    linkAddress.sll_halen = sizeof(mac);
    std::memcpy(linkAddress.sll_addr, mac, sizeof(mac));
    statistics.rx_packets = 13;
    statistics.tx_packets = 17;
    statistics.rx_bytes = 101;
    statistics.tx_bytes = 103;
#else
    linkAddress.sdl_len = sizeof(linkAddress);
    linkAddress.sdl_family = AF_LINK;
    linkAddress.sdl_index = 7;
    linkAddress.sdl_type = 6;
    linkAddress.sdl_nlen = 5;
    linkAddress.sdl_alen = sizeof(mac);
    std::memcpy(linkAddress.sdl_data, "noip0", 5);
    std::memcpy(linkAddress.sdl_data + 5, mac, sizeof(mac));
    statistics.ifi_type = 6;
    statistics.ifi_addrlen = sizeof(mac);
    statistics.ifi_mtu = 9000;
    statistics.ifi_ipackets = 13;
    statistics.ifi_opackets = 17;
    statistics.ifi_ibytes = 101;
    statistics.ifi_obytes = 103;
#endif
    *list = &fixture;
    includeAll = true;
    return 0;
}
static void FixtureFree(ifaddrs* list) { if (list != &fixture) std::abort(); }
static int FixtureSocket(int, int, int) { return 1; }
static int FixtureClose(int descriptor) { if (descriptor != 1) std::abort(); return 0; }
static int FixtureIoctl(int descriptor, unsigned long request, ...) {
    if (descriptor != 1 || request != SIOCGIFMTU) std::abort();
    std::va_list args;
    va_start(args, request);
    auto* value = va_arg(args, ifreq*);
    va_end(args);
    if (std::strcmp(value->ifr_name, "noip0") != 0) std::abort();
    value->ifr_mtu = 9000;
    return 0;
}
#define getifaddrs FixtureAddresses
#define freeifaddrs FixtureFree
#define socket FixtureSocket
#define close FixtureClose
#define ioctl FixtureIoctl
#endif

#include "../prx/libc/src/Interfaces.cpp"

static void Require(bool value) { if (!value) std::abort(); }

int main() {
    GuestInterfaceAddress* list = nullptr;
    Require(getifaddrs_nid_postfix(&list) == 0 && includeAll);
    Require(list && !list->next && std::strcmp(list->name, "noip0") == 0);
    Require(list->netmask == nullptr && list->destination == nullptr && list->data != nullptr);
    const auto* address = static_cast<const std::uint8_t*>(list->address);
    Require(address[0] == 54 && address[1] == 18 && address[2] == 7 && address[3] == 0);
    Require(address[4] == 6 && address[5] == 5 && address[6] == 6 && address[7] == 0);
    Require(std::memcmp(address + 8, "noip0", 5) == 0 && std::memcmp(address + 13, mac, sizeof(mac)) == 0);
    const auto& data = *static_cast<const GuestInterfaceData*>(list->data);
    Require(data.dataLength == 152 && data.mtu == 9000 && data.type == 6 && data.addressLength == 6);
    Require(data.inputPackets == 13 && data.outputPackets == 17 && data.inputBytes == 101 && data.outputBytes == 103);
    Require(reinterpret_cast<std::uintptr_t>(address) % 8 == 0 && reinterpret_cast<std::uintptr_t>(list->data) % 8 == 0);
    freeifaddrs_nid_postfix(list);
    freeifaddrs_nid_postfix(nullptr);
    Require(getifaddrs_nid_postfix(nullptr) == -1 && guestError == 14);
}
