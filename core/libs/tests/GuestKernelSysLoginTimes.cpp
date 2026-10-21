#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <source_location>
#include <string>
#ifndef _WIN32
#include <sys/stat.h>
#endif

extern "C" {
int* APS5_VABI __error_nid_postfix();
int APS5_VABI getlogin_r_nid_postfix(char* output, int size);
char* APS5_VABI getlogin_nid_postfix(void);
int APS5_VABI futimesat_nid_postfix(int fd, const char* path, const KernelTimeval* times);
int APS5_VABI sceKernelOpen(const char* path, int flags, std::uint16_t mode);
int APS5_VABI sceKernelClose(int fd);
}

namespace {

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (condition) return;
    std::fprintf(stderr, "check failed at line %u\n", static_cast<unsigned>(location.line()));
    std::exit(1);
}

int Error() {
    return *__error_nid_postfix();
}

std::int64_t ModifiedSeconds(const std::filesystem::path& path) {
    const auto written = std::chrono::clock_cast<std::chrono::system_clock>(std::filesystem::last_write_time(path));
    return std::chrono::duration_cast<std::chrono::seconds>(written.time_since_epoch()).count();
}

void Getlogin() {
    char expected[33]{};
    const int error = getlogin_r_nid_postfix(expected, static_cast<int>(sizeof(expected)));
    char* name = getlogin_nid_postfix();
    if (error != 0) {
        Require(name == nullptr && Error() == error);
        return;
    }
    if (expected[0] == '\0') {
        Require(name == nullptr);
        return;
    }
    Require(name != nullptr && std::strcmp(name, expected) == 0);
    Require(getlogin_nid_postfix() == name);
}

void Futimesat() {
    const auto root = std::filesystem::path("anyps5-futimesat-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Require(std::filesystem::create_directory(root));
    const auto file = root / "file.bin";
    { std::ofstream stream(file); stream << "x"; }
    const KernelTimeval first[2]{{1000000000, 0}, {1000000000, 0}};
    const KernelTimeval second[2]{{1500000000, 0}, {1500000000, 0}};
    Require(futimesat_nid_postfix(-100, file.string().c_str(), first) == 0);
    Require(ModifiedSeconds(file) == 1000000000);
    const int base = sceKernelOpen(root.string().c_str(), SCE_KERNEL_O_RDONLY | SCE_KERNEL_O_DIRECTORY, 0);
    Require(base >= 0);
    Require(futimesat_nid_postfix(base, "file.bin", second) == 0);
    Require(ModifiedSeconds(file) == 1500000000);
    Require(futimesat_nid_postfix(base, "file.bin", nullptr) == 0);
    Require(ModifiedSeconds(file) > 1500000000);
    Require(futimesat_nid_postfix(base, "missing.bin", first) == -1 && Error() == 2);
#ifndef _WIN32
    const auto fifo = root / "fifo";
    Require(::mkfifo(fifo.string().c_str(), 0600) == 0);
    Require(futimesat_nid_postfix(base, "fifo", second) == 0);
    Require(ModifiedSeconds(fifo) == 1500000000);
#endif
    const int plain = sceKernelOpen(file.string().c_str(), SCE_KERNEL_O_RDONLY, 0);
    Require(plain >= 0);
    Require(futimesat_nid_postfix(plain, "file.bin", first) == -1 && Error() == 20);
    Require(futimesat_nid_postfix(-100, nullptr, first) == -1 && Error() == 14);
    Require(futimesat_nid_postfix(-100, "", first) == -1 && Error() == 2);
    Require(sceKernelClose(plain) == 0 && sceKernelClose(base) == 0);
    std::filesystem::remove_all(root);
}

}

int main() {
    Getlogin();
    Futimesat();
    std::puts("getlogin and futimesat passed");
    return 0;
}
