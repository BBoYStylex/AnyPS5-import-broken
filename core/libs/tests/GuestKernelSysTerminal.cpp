#include "prx/libc/include/general/VabiMacros.hpp"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

extern "C" {
int APS5_VABI tcgetsid_nid_postfix(int);
int APS5_VABI open_nid_postfix(const char*, int, int);
int APS5_VABI pipe_nid_postfix(int*);
int APS5_VABI socket_nid_postfix(int, int, int);
int APS5_VABI socketpair_nid_postfix(int, int, int, int*);
int APS5_VABI mkdir_nid_postfix(const char*, unsigned short);
int APS5_VABI close_nid_postfix(int);
int* APS5_VABI __error_nid_postfix();
}

namespace {

constexpr int BadDescriptor = 9;
constexpr int NotTerminal = 25;
constexpr int OperationNotSupported = 45;
constexpr int AddressNotAvailable = 49;

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "tcgetsid check failed at line %d, errno %d\n", line, *__error_nid_postfix());
        std::abort();
    }
}
#define Require(value) Check((value), __LINE__)

bool Fails(int descriptor, int error) {
    *__error_nid_postfix() = 0;
    return tcgetsid_nid_postfix(descriptor) == -1 && *__error_nid_postfix() == error;
}

}

int main() {
    const auto name = "anyps5-tcgetsid-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const int file = open_nid_postfix(name.c_str(), 0x202, 0644);
    Require(file >= 0);
    Require(Fails(file, NotTerminal));
    Require(close_nid_postfix(file) == 0);
    Require(Fails(file, BadDescriptor));
    std::filesystem::remove(name);

    Require(Fails(-1, BadDescriptor));

    int ends[2] = {-1, -1};
    Require(pipe_nid_postfix(ends) == 0);
    for (const int end : ends) Require(Fails(end, NotTerminal));
    Require(close_nid_postfix(ends[0]) == 0 && close_nid_postfix(ends[1]) == 0);

    const int internet = socket_nid_postfix(2, 2, 0);
    Require(internet >= 0);
    Require(Fails(internet, AddressNotAvailable));
    Require(close_nid_postfix(internet) == 0);

    int pair[2] = {-1, -1};
    Require(socketpair_nid_postfix(1, 1, 0, pair) == 0);
    for (const int end : pair) Require(Fails(end, OperationNotSupported));
    Require(close_nid_postfix(pair[0]) == 0 && close_nid_postfix(pair[1]) == 0);

    const auto directoryName = name + "-directory";
    Require(mkdir_nid_postfix(directoryName.c_str(), 0755) == 0);
    const int directory = open_nid_postfix(directoryName.c_str(), 0x00020000, 0);
    Require(directory >= 0);
    Require(Fails(directory, NotTerminal));
    Require(close_nid_postfix(directory) == 0);
    Require(Fails(directory, BadDescriptor));
    std::filesystem::remove(directoryName);
    return 0;
}
