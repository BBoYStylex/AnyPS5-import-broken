#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#endif

extern "C" {
int APS5_VABI daemon_nid_postfix(int, int);
char* APS5_VABI getcwd_nid_postfix(char*, std::size_t);
}

static void Require(bool value) { if (!value) std::abort(); }

int main() {
    Require(daemon_nid_postfix(1, 1) == 0);
#ifndef _WIN32
    const int savedOut = ::dup(1);
    const int savedErr = ::dup(2);
    Require(savedOut >= 0 && savedErr >= 0);
    Require(daemon_nid_postfix(1, 0) == 0);
    struct stat null{};
    Require(::stat("/dev/null", &null) == 0);
    for (int descriptor = 0; descriptor < 3; ++descriptor) {
        struct stat redirected{};
        Require(::fstat(descriptor, &redirected) == 0 && S_ISCHR(redirected.st_mode) && redirected.st_rdev == null.st_rdev);
    }
    Require(::dup2(savedOut, 1) == 1 && ::dup2(savedErr, 2) == 2);
#else
    Require(daemon_nid_postfix(1, 0) == 0);
#endif
    Require(daemon_nid_postfix(0, 1) == 0);
    char directory[64];
    Require(getcwd_nid_postfix(directory, sizeof(directory)) == directory && std::strcmp(directory, "/") == 0);
    return 0;
}
