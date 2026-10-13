#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" {
int APS5_VABI socket_nid_postfix(int, int, int);
int APS5_VABI close_nid_postfix(int);
int* APS5_VABI __error_nid_postfix();
}

namespace {

constexpr int Stream = 1;
constexpr int Datagram = 2;
constexpr int ErrorProtocolType = 41;
constexpr int ErrorProtocolNotSupported = 43;
constexpr int ErrorAddressFamilyNotSupported = 47;

void Require(bool condition, const char* what) {
    if (condition) return;
    std::fprintf(stderr, "%s (errno %d)\n", what, *__error_nid_postfix());
    std::abort();
}

bool Fails(int result, int error) { return result == -1 && *__error_nid_postfix() == error; }

}

int main() {
    Require(Fails(socket_nid_postfix(2, 99, 0), ErrorProtocolType), "an unknown type with protocol 0 must fail with EPROTOTYPE (41)");
    Require(Fails(socket_nid_postfix(28, 99, 0), ErrorProtocolType), "an unknown type with protocol 0 on AF_INET6 must fail with EPROTOTYPE (41)");
    Require(Fails(socket_nid_postfix(2, 99, 6), ErrorProtocolNotSupported), "an unknown type with a protocol must fail with EPROTONOSUPPORT (43)");
    Require(Fails(socket_nid_postfix(2, 0, 0), ErrorProtocolNotSupported), "type 0 must fail with EPROTONOSUPPORT (43)");
    Require(Fails(socket_nid_postfix(2, Stream, 17), ErrorProtocolNotSupported), "a stream socket with the UDP protocol must fail with EPROTONOSUPPORT (43)");
    Require(Fails(socket_nid_postfix(2, Datagram, 6), ErrorProtocolNotSupported), "a datagram socket with the TCP protocol must fail with EPROTONOSUPPORT (43)");
    Require(Fails(socket_nid_postfix(99, Stream, 0), ErrorAddressFamilyNotSupported), "an unknown family must fail with EAFNOSUPPORT (47)");
    const int stream = socket_nid_postfix(2, Stream, 0);
    const int datagram = socket_nid_postfix(2, Datagram, 17);
    Require(stream >= 0 && datagram >= 0, "a valid socket must still be created");
    Require(close_nid_postfix(stream) == 0 && close_nid_postfix(datagram) == 0, "close failed");
    return 0;
}
