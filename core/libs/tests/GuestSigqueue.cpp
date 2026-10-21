#include "prx/libc/include/general/VabiMacros.hpp"
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
using Handler = void (APS5_VABI *)(int);
extern "C" {
int APS5_VABI sigqueue_nid_postfix(int, int, void*);
Handler APS5_VABI signal_nid_postfix(int, Handler);
int APS5_VABI getpid_nid_postfix(void);
int* APS5_VABI __error_nid_postfix();
}
volatile std::sig_atomic_t received = 0;
void APS5_VABI Callback(int value) { received = value; }
static void Require(bool value) { if (!value) std::abort(); }
static void Fails(int pid, int guest, int error) {
    *__error_nid_postfix() = 0;
    Require(sigqueue_nid_postfix(pid, guest, nullptr) == -1 && *__error_nid_postfix() == error);
}
static void Succeeds(int pid, int guest, void* value) {
    *__error_nid_postfix() = 0;
    Require(sigqueue_nid_postfix(pid, guest, value) == 0 && *__error_nid_postfix() == 0);
}
int main() {
    const int self = getpid_nid_postfix();
    const auto invalid = reinterpret_cast<Handler>(static_cast<std::uintptr_t>(-1));
    const auto ignore = reinterpret_cast<Handler>(std::uintptr_t{1});
    int payload = 7;
    Succeeds(self, 0, &payload);
    Succeeds(self, 0, nullptr);
    for (int guest : {-1, 129}) {
        Fails(self, guest, 22);
        Fails(self + 1, guest, 22);
        Fails(0, guest, 22);
    }
    for (int target : {0, -1, -self}) {
        Fails(target, 0, 22);
        Fails(target, 15, 22);
    }
    for (int target : {self + 1, self + 1000}) {
        Fails(target, 0, 3);
        Fails(target, 15, 3);
    }
    Require(signal_nid_postfix(30, ignore) != invalid);
    received = 0;
    Succeeds(self, 30, &payload);
    Require(received == 0);
    Require(signal_nid_postfix(15, Callback) != invalid);
    received = 0;
    Succeeds(self, 15, &payload);
    Require(received == 15);
    received = 0;
    Fails(self + 1, 15, 3);
    Require(received == 0);
    return 0;
}
