#include "prx/libc/include/general/VabiMacros.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

extern "C" {
void APS5_VABI RAND_add_nid_postfix(const void*, int, double);
int APS5_VABI RAND_bytes_nid_postfix(unsigned char*, int);
const char* APS5_VABI RAND_file_name_nid_postfix(char*, std::size_t);
int APS5_VABI RAND_load_file_nid_postfix(const char*, long);
int APS5_VABI RAND_status_nid_postfix(void);
}

static void Require(bool value) { if (!value) std::abort(); }

template<typename TAction>
static void RequireThrows(TAction action) {
    try { action(); } catch (const std::invalid_argument&) { return; }
    std::abort();
}

static void CheckBytes() {
    std::array<unsigned char, 600> first{};
    std::array<unsigned char, 600> second{};
    Require(RAND_bytes_nid_postfix(first.data(), static_cast<int>(first.size())) == 1);
    Require(RAND_bytes_nid_postfix(second.data(), static_cast<int>(second.size())) == 1);
    Require(first != second);
    Require(std::any_of(first.begin(), first.end(), [](unsigned char byte) { return byte != 0; }));
    std::array<unsigned char, 4> untouched{0xaa, 0xaa, 0xaa, 0xaa};
    Require(RAND_bytes_nid_postfix(untouched.data(), 0) == 1 && RAND_bytes_nid_postfix(untouched.data(), -5) == 1);
    Require(std::all_of(untouched.begin(), untouched.end(), [](unsigned char byte) { return byte == 0xaa; }));
    Require(RAND_bytes_nid_postfix(nullptr, 0) == 1);
    RequireThrows([] { RAND_bytes_nid_postfix(nullptr, 1); });
}

static void CheckFileName() {
    std::array<char, 16> buffer{};
    buffer.fill('x');
    Require(RAND_file_name_nid_postfix(buffer.data(), 13) == buffer.data());
    Require(std::strcmp(buffer.data(), "/dev/urandom") == 0 && buffer[13] == 'x');
    buffer.fill('x');
    Require(RAND_file_name_nid_postfix(buffer.data(), 12) == nullptr);
    Require(std::strcmp(buffer.data(), "/dev/urando") == 0 && buffer[12] == 'x');
    buffer.fill('x');
    Require(RAND_file_name_nid_postfix(buffer.data(), 1) == nullptr && buffer[0] == '\0' && buffer[1] == 'x');
    Require(RAND_file_name_nid_postfix(buffer.data(), 0) == nullptr && buffer[1] == 'x');
    Require(RAND_file_name_nid_postfix(nullptr, 0) == nullptr);
}

int main() {
    CheckBytes();
    CheckFileName();
    Require(RAND_status_nid_postfix() == 1);
    Require(RAND_load_file_nid_postfix("/any", -1) == 123456);
    Require(RAND_load_file_nid_postfix(nullptr, 1024) == 1024 && RAND_load_file_nid_postfix(nullptr, 0) == 0);
    RAND_add_nid_postfix(nullptr, 0, 0.0);
    std::array<unsigned char, 8> seed{};
    RAND_add_nid_postfix(seed.data(), static_cast<int>(seed.size()), 8.0);
    return 0;
}
