#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" {
int APS5_VABI open_nid_postfix(const char*, int, int);
int APS5_VABI close_nid_postfix(int);
int APS5_VABI unlink_nid_postfix(const char*);
std::int64_t APS5_VABI write_nid_postfix(int, const char*, std::int64_t);
int APS5_VABI fstat_nid_disambig1_nid_postfix(int, void*);
int APS5_VABI flock_nid_postfix(int, int);
int* APS5_VABI __error_nid_postfix();
}

namespace {

constexpr int ReadWrite = 2;
constexpr int Create = 0x200;
constexpr int ExclusiveLock = 2;
constexpr int ErrorBadDescriptor = 9;
constexpr int ErrorFault = 14;
constexpr int ErrorInvalid = 22;
constexpr char FileName[] = "anyps5_bad_arguments_file";
int openDescriptor = -1;

void Require(bool condition, const char* what) {
    if (condition) return;
    const int error = *__error_nid_postfix();
    if (openDescriptor >= 0) close_nid_postfix(openDescriptor);
    unlink_nid_postfix(FileName);
    std::fprintf(stderr, "%s (errno %d)\n", what, error);
    std::abort();
}

bool Fails(long result, int error) { return result == -1 && *__error_nid_postfix() == error; }

}

int main() {
    unsigned char status[256];
    unlink_nid_postfix(FileName);
    const int descriptor = open_nid_postfix(FileName, Create | ReadWrite, 0644);
    openDescriptor = descriptor;
    Require(descriptor >= 0, "open failed");
    Require(fstat_nid_disambig1_nid_postfix(descriptor, status) == 0, "fstat of an open file failed");
    Require(flock_nid_postfix(descriptor, ExclusiveLock) == 0, "flock of an open file failed");
    Require(Fails(fstat_nid_disambig1_nid_postfix(descriptor, nullptr), ErrorFault), "fstat with a null buffer must fail with EFAULT (14)");
    Require(Fails(write_nid_postfix(descriptor, "x", -1), ErrorInvalid), "write of a negative length must fail with EINVAL (22)");
    openDescriptor = -1;
    Require(close_nid_postfix(descriptor) == 0, "close failed");
    Require(Fails(fstat_nid_disambig1_nid_postfix(descriptor, status), ErrorBadDescriptor), "fstat on a closed descriptor must fail with EBADF (9)");
    Require(Fails(fstat_nid_disambig1_nid_postfix(-1, status), ErrorBadDescriptor), "fstat on -1 must fail with EBADF (9)");
    Require(Fails(flock_nid_postfix(descriptor, ExclusiveLock), ErrorBadDescriptor), "flock on a closed descriptor must fail with EBADF (9)");
    Require(Fails(flock_nid_postfix(-1, ExclusiveLock), ErrorBadDescriptor), "flock on -1 must fail with EBADF (9)");
    unlink_nid_postfix(FileName);
    return 0;
}
