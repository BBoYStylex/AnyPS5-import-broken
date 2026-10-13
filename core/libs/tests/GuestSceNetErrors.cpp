#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
int APS5_VABI sceNetInit_nid_postfix(void);
int APS5_VABI sceNetSocket(const char*, int, int, int);
int APS5_VABI sceNetBind_nid_postfix(int, const void*, std::uint32_t);
std::int64_t APS5_VABI sceNetSend(int, const void*, std::size_t, int);
std::int64_t APS5_VABI sceNetSendto(int, const void*, std::size_t, int, const void*, std::uint32_t);
int APS5_VABI sceNetSocketClose(int);
int* APS5_VABI sceNetErrnoLoc(void);
}

namespace {

constexpr int Stream = 1;
constexpr int Datagram = 2;
constexpr int ErrorDestinationRequired = 39;
constexpr int ErrorProtocolType = 41;
constexpr int ErrorProtocolNotSupported = 43;
constexpr int ErrorAddressNotAvailable = 49;

void Require(bool condition, const char* what) {
    if (condition) return;
    std::fprintf(stderr, "%s (errno %d)\n", what, *sceNetErrnoLoc());
    std::abort();
}

bool Fails(std::int64_t result, int error) {
    return result < 0 && *sceNetErrnoLoc() == error;
}

}

int main() {
    Require(sceNetInit_nid_postfix() == 0, "sceNetInit failed");
    Require(Fails(sceNetSocket("errors", 2, 99, 0), ErrorProtocolType), "a socket of an unknown type and protocol 0 must fail with EPROTOTYPE (41)");
    Require(Fails(sceNetSocket("errors", 2, 99, 6), ErrorProtocolNotSupported), "a socket of an unknown type and a protocol must fail with EPROTONOSUPPORT (43)");
    Require(Fails(sceNetSocket("errors", 2, Stream, 17), ErrorProtocolNotSupported), "a stream socket with the UDP protocol must fail with EPROTONOSUPPORT (43)");

    const int datagram = sceNetSocket("errors", 2, Datagram, 0);
    Require(datagram >= 0, "datagram socket failed");
    Require(Fails(sceNetSend(datagram, "x", 1, 0), ErrorDestinationRequired), "sceNetSend on an unconnected datagram socket must fail with EDESTADDRREQ (39)");
    Require(Fails(sceNetSendto(datagram, "x", 1, 0, nullptr, 0), ErrorDestinationRequired), "sceNetSendto without an address on an unconnected datagram socket must fail with EDESTADDRREQ (39)");

    std::array<std::uint8_t, 16> foreign{16, 2};
    foreign[4] = 192;
    foreign[5] = 0;
    foreign[6] = 2;
    foreign[7] = 1;
    Require(Fails(sceNetBind_nid_postfix(datagram, foreign.data(), foreign.size()), ErrorAddressNotAvailable), "binding an address that is not local must fail with EADDRNOTAVAIL (49)");
    Require(sceNetSocketClose(datagram) == 0, "close failed");
    return 0;
}
