#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

extern "C" {
void APS5_VABI DES_set_odd_parity_nid_postfix(unsigned char*);
int APS5_VABI DES_set_key_nid_postfix(const unsigned char*, void*);
void APS5_VABI DES_ecb_encrypt_nid_postfix(const unsigned char*, unsigned char*, const void*, int);
}

namespace {
constexpr std::size_t ScheduleSize = 128;

struct Vector {
    const char* key;
    const char* plain;
    const char* cipher;
};

constexpr Vector Vectors[] {
    {"133457799bbcdff1", "0123456789abcdef", "85e813540f0ab405"},
    {"0101010101010101", "8000000000000000", "95f8a5e5dd31d900"},
    {"fedcba9876543210", "1122334455667788", "db9a39c3a6ca1ca8"},
};

constexpr const char* FirstSchedule =
    "60d0fc80c4400fce78dc6ce48a85c943a84c087c890f458338ec6c284b85c68cf80c5c700107ce4a18282834cfc5074"
    "edc48bc4403c14ec8bc140cf4cdc185cc1cf4dc78084b8d0734b074988f0f4e0210fcec70868a4c0bb8e0a4f889ce0a4"
    "6a4e87c94c883480de8703c38c5c24ec7f460f03cc5098b0c4c6cc0f8ca0c4dc8";

void Require(bool value, const char* what) {
    if (value) return;
    std::fprintf(stderr, "failed: %s\n", what);
    std::abort();
}

template<typename TAction> void RequireThrows(TAction action, const char* what) {
    try { action(); } catch (const std::exception&) { return; }
    Require(false, what);
}

std::vector<unsigned char> FromHex(const char* text) {
    std::vector<unsigned char> bytes;
    for (; text[0] && text[1]; text += 2) {
        const char pair[3] {text[0], text[1], 0};
        bytes.push_back(static_cast<unsigned char>(std::strtoul(pair, nullptr, 16)));
    }
    return bytes;
}
}

int main() {
    for (const auto& vector : Vectors) {
        const auto key = FromHex(vector.key);
        const auto plain = FromHex(vector.plain);
        const auto cipher = FromHex(vector.cipher);
        unsigned char schedule[ScheduleSize];
        Require(DES_set_key_nid_postfix(key.data(), schedule) == 0, "DES_set_key");

        unsigned char out[8];
        DES_ecb_encrypt_nid_postfix(plain.data(), out, schedule, 1);
        Require(std::memcmp(out, cipher.data(), 8) == 0, "DES_ecb_encrypt known answer");
        DES_ecb_encrypt_nid_postfix(plain.data(), out, schedule, 7);
        Require(std::memcmp(out, cipher.data(), 8) == 0, "DES_ecb_encrypt treats any nonzero enc as encrypt");
        DES_ecb_encrypt_nid_postfix(cipher.data(), out, schedule, 0);
        Require(std::memcmp(out, plain.data(), 8) == 0, "DES_ecb_encrypt decrypt");

        unsigned char inPlace[8];
        std::memcpy(inPlace, plain.data(), 8);
        DES_ecb_encrypt_nid_postfix(inPlace, inPlace, schedule, 1);
        Require(std::memcmp(inPlace, cipher.data(), 8) == 0, "DES_ecb_encrypt in place");

        RequireThrows([&] { DES_set_key_nid_postfix(nullptr, schedule); }, "null key");
        RequireThrows([&] { DES_set_key_nid_postfix(key.data(), nullptr); }, "null key schedule");
        RequireThrows([&] { DES_ecb_encrypt_nid_postfix(nullptr, out, schedule, 1); }, "null input");
        RequireThrows([&] { DES_ecb_encrypt_nid_postfix(plain.data(), nullptr, schedule, 1); }, "null output");
        RequireThrows([&] { DES_ecb_encrypt_nid_postfix(plain.data(), out, nullptr, 1); }, "null schedule");
    }

    const auto first = FromHex(Vectors[0].key);
    unsigned char schedule[ScheduleSize];
    DES_set_key_nid_postfix(first.data(), schedule);
    Require(std::memcmp(schedule, FromHex(FirstSchedule).data(), ScheduleSize) == 0, "key schedule layout");

    unsigned char parity[8] {0x00, 0x01, 0x02, 0x03, 0xfe, 0xff, 0x7f, 0x80};
    DES_set_odd_parity_nid_postfix(parity);
    Require(std::memcmp(parity, FromHex("01010202fefe7f80").data(), 8) == 0, "DES_set_odd_parity");
    RequireThrows([&] { DES_set_odd_parity_nid_postfix(nullptr); }, "null parity key");
    return 0;
}
