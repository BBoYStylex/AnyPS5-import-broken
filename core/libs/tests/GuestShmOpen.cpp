#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
extern "C" {
int APS5_VABI shm_open_nid_postfix(const char*, int, int);
int APS5_VABI ftruncate_nid_postfix(int, std::int64_t);
std::int64_t APS5_VABI pread_nid_postfix(int, void*, std::size_t, std::int64_t);
std::int64_t APS5_VABI pwrite_nid_disambig1_nid_postfix(int, const void*, std::size_t, std::int64_t);
std::int64_t APS5_VABI write_nid_postfix(int, const char*, std::int64_t);
std::int64_t APS5_VABI lseek_nid_postfix(int, std::int64_t, int);
void* APS5_VABI mmap_nid_postfix(void*, std::size_t, int, int, int, std::int64_t) noexcept;
int APS5_VABI dup_nid_postfix(int);
int APS5_VABI close_nid_postfix(int);
int* APS5_VABI __error_nid_postfix();
}
static void Require(bool value) { if (!value) std::abort(); }
static const char* const anonymous = reinterpret_cast<const char*>(1);
static void RequireInvalid(const char* path, int flags) {
    *__error_nid_postfix() = 0;
    Require(shm_open_nid_postfix(path, flags, 0600) == -1);
    Require(*__error_nid_postfix() == 22);
}
static void RequireInvalidSeek(int descriptor, std::int64_t offset, int whence) {
    *__error_nid_postfix() = 0;
    Require(lseek_nid_postfix(descriptor, offset, whence) == -1);
    Require(*__error_nid_postfix() == 22);
}
int main() {
    *__error_nid_postfix() = 13;
    const int first = shm_open_nid_postfix(anonymous, 0x100202, 0600);
    const int second = shm_open_nid_postfix(anonymous, 0x2, 0);
    const int third = shm_open_nid_postfix(anonymous, 0x100e02, 0400);
    Require(first >= 0 && second >= 0 && third >= 0);
    Require(first != second && second != third && first != third);
    Require(*__error_nid_postfix() == 13);
    Require(ftruncate_nid_postfix(first, 0x4000) == 0);
    const char text[] = "anonymous";
    Require(pwrite_nid_disambig1_nid_postfix(first, text, sizeof(text), 0x1000) == static_cast<std::int64_t>(sizeof(text)));
    char back[sizeof(text)] = {};
    Require(pread_nid_postfix(first, back, sizeof(back), 0x1000) == static_cast<std::int64_t>(sizeof(back)));
    Require(std::memcmp(back, text, sizeof(text)) == 0);
    char last = 1;
    Require(pread_nid_postfix(first, &last, 1, 0x3fff) == 1 && last == 0);
    Require(pread_nid_postfix(first, &last, 1, 0x4000) == 0);
    Require(pread_nid_postfix(second, back, sizeof(back), 0) == 0);
    Require(pwrite_nid_disambig1_nid_postfix(third, text, sizeof(text), 0) == 0);
    Require(write_nid_postfix(third, text, sizeof(text)) == 0);
    Require(lseek_nid_postfix(third, 0, 2) == 0);
    RequireInvalidSeek(third, 1, 0);
    Require(ftruncate_nid_postfix(third, 0x4000) == 0);
    Require(pwrite_nid_disambig1_nid_postfix(third, text, sizeof(text), 0) == static_cast<std::int64_t>(sizeof(text)));
    Require(pwrite_nid_disambig1_nid_postfix(third, text, sizeof(text), 0x3ffc) == 4);
    Require(lseek_nid_postfix(third, 0x3ffc, 0) == 0x3ffc);
    Require(write_nid_postfix(third, text, sizeof(text)) == 4);
    Require(write_nid_postfix(third, text, sizeof(text)) == 0);
    Require(lseek_nid_postfix(third, 0, 1) == 0x4000);
    Require(lseek_nid_postfix(third, 0, 2) == 0x4000);
    RequireInvalidSeek(third, 1, 2);
    RequireInvalidSeek(third, 0x4001, 0);
    Require(lseek_nid_postfix(third, 0, 1) == 0x4000);
    const int copy = dup_nid_postfix(third);
    Require(copy >= 0);
    Require(write_nid_postfix(copy, text, sizeof(text)) == 0);
    Require(pwrite_nid_disambig1_nid_postfix(copy, text, sizeof(text), 0x3fff) == 1);
    Require(close_nid_postfix(copy) == 0);
    Require(pread_nid_postfix(third, &last, 1, 0x4000) == 0);
#ifdef _WIN32
    *__error_nid_postfix() = 0;
    Require(mmap_nid_postfix(nullptr, 0x4000, 3, 0x1, third, 0) == reinterpret_cast<void*>(-1));
    Require(*__error_nid_postfix() == 45);
#endif
    Require(close_nid_postfix(first) == 0 && close_nid_postfix(second) == 0 && close_nid_postfix(third) == 0);
    RequireInvalid(anonymous, 0x0);
    RequireInvalid(anonymous, 0x200);
    RequireInvalid(anonymous, 0x1);
    RequireInvalid(anonymous, 0x3);
    RequireInvalid(anonymous, 0x2 | 0x8);
    RequireInvalid(anonymous, 0x2 | 0x4);
    RequireInvalid("/anyps5-shm", 0x1);
    RequireInvalid("/anyps5-shm", 0x202 | 0x8);
#ifdef _WIN32
    bool threw = false;
    try {
        shm_open_nid_postfix("/anyps5-shm", 0x202, 0600);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);
#else
    RequireInvalid("", 0x202);
#endif
}
