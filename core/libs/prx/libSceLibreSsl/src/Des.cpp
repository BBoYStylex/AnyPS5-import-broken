#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include "prx/libc/include/General.hpp"

namespace {
struct DesKeySchedule {
    std::uint32_t ks[16][2];
};

static_assert(sizeof(DesKeySchedule) == 128);

constexpr std::uint8_t InitialPermutation[64] {
    58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6, 64, 56, 48, 40, 32, 24, 16, 8,
    57, 49, 41, 33, 25, 17, 9, 1, 59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7,
};

constexpr std::uint8_t FinalPermutation[64] {
    40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31,
    38, 6, 46, 14, 54, 22, 62, 30, 37, 5, 45, 13, 53, 21, 61, 29,
    36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27,
    34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41, 9, 49, 17, 57, 25,
};

constexpr std::uint8_t Expansion[48] {
    32, 1, 2, 3, 4, 5, 4, 5, 6, 7, 8, 9, 8, 9, 10, 11, 12, 13, 12, 13, 14, 15, 16, 17,
    16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25, 24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32, 1,
};

constexpr std::uint8_t RoundPermutation[32] {
    16, 7, 20, 21, 29, 12, 28, 17, 1, 15, 23, 26, 5, 18, 31, 10,
    2, 8, 24, 14, 32, 27, 3, 9, 19, 13, 30, 6, 22, 11, 4, 25,
};

constexpr std::uint8_t KeyPermutation1[56] {
    57, 49, 41, 33, 25, 17, 9, 1, 58, 50, 42, 34, 26, 18, 10, 2, 59, 51, 43, 35, 27, 19, 11, 3, 60, 52, 44, 36,
    63, 55, 47, 39, 31, 23, 15, 7, 62, 54, 46, 38, 30, 22, 14, 6, 61, 53, 45, 37, 29, 21, 13, 5, 28, 20, 12, 4,
};

constexpr std::uint8_t KeyPermutation2[48] {
    14, 17, 11, 24, 1, 5, 3, 28, 15, 6, 21, 10, 23, 19, 12, 4, 26, 8, 16, 7, 27, 20, 13, 2,
    41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48, 44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32,
};

constexpr std::uint8_t KeyShifts[16] {1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};

constexpr std::uint8_t SBoxes[8][64] {
    {14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7, 0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8,
     4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0, 15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13},
    {15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10, 3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5,
     0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15, 13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9},
    {10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8, 13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1,
     13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7, 1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12},
    {7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15, 13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9,
     10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4, 3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14},
    {2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9, 14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6,
     4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14, 11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3},
    {12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11, 10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8,
     9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6, 4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13},
    {4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1, 13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6,
     1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2, 6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12},
    {13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7, 1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2,
     7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8, 2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11},
};

std::uint64_t Permute(std::uint64_t value, const std::uint8_t* table, std::size_t outputBits, std::size_t inputBits) {
    std::uint64_t result = 0;
    for (std::size_t i = 0; i < outputBits; ++i) result = result << 1 | ((value >> (inputBits - table[i])) & 1);
    return result;
}

std::uint64_t LoadBlock(const unsigned char* p) {
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i) value = value << 8 | p[i];
    return value;
}

void StoreBlock(unsigned char* p, std::uint64_t value) {
    for (int i = 7; i >= 0; --i, value >>= 8) p[i] = static_cast<unsigned char>(value);
}

void RequirePointer(const void* pointer, const char* function, const char* what) {
    if (!pointer) throw std::invalid_argument(std::string(function) + ": null " + what);
}

unsigned ReverseGroup(unsigned group) {
    unsigned result = 0;
    for (int i = 0; i < 6; ++i) result |= ((group >> i) & 1) << (5 - i);
    return result;
}

std::uint32_t RotateLeft(std::uint32_t value, int count) {
    return value << count | value >> (32 - count);
}

void PackRound(std::uint64_t subkey, std::uint32_t* words) {
    std::uint32_t even = 0, odd = 0;
    for (int i = 0; i < 4; ++i) {
        even |= ReverseGroup(static_cast<unsigned>(subkey >> (42 - 12 * i)) & 0x3f) << (2 + 8 * i);
        odd |= ReverseGroup(static_cast<unsigned>(subkey >> (36 - 12 * i)) & 0x3f) << (2 + 8 * i);
    }
    words[0] = even;
    words[1] = RotateLeft(odd, 4);
}

std::uint64_t UnpackRound(const std::uint32_t* words) {
    const std::uint32_t odd = RotateLeft(words[1], 28);
    std::uint64_t subkey = 0;
    for (int i = 0; i < 4; ++i) {
        subkey |= std::uint64_t(ReverseGroup((words[0] >> (2 + 8 * i)) & 0x3f)) << (42 - 12 * i);
        subkey |= std::uint64_t(ReverseGroup((odd >> (2 + 8 * i)) & 0x3f)) << (36 - 12 * i);
    }
    return subkey;
}

void ExpandKey(const unsigned char* key, DesKeySchedule& schedule) {
    const std::uint64_t permuted = Permute(LoadBlock(key), KeyPermutation1, 56, 64);
    std::uint32_t c = static_cast<std::uint32_t>(permuted >> 28) & 0xfffffff;
    std::uint32_t d = static_cast<std::uint32_t>(permuted) & 0xfffffff;
    for (int round = 0; round < 16; ++round) {
        for (int shift = 0; shift < KeyShifts[round]; ++shift) {
            c = ((c << 1) | (c >> 27)) & 0xfffffff;
            d = ((d << 1) | (d >> 27)) & 0xfffffff;
        }
        const std::uint64_t joined = std::uint64_t(c) << 28 | d;
        PackRound(Permute(joined, KeyPermutation2, 48, 56), schedule.ks[round]);
    }
}

std::uint32_t RoundFunction(std::uint32_t right, std::uint64_t subkey) {
    const std::uint64_t expanded = Permute(right, Expansion, 48, 32) ^ subkey;
    std::uint32_t substituted = 0;
    for (int box = 0; box < 8; ++box) {
        const unsigned group = static_cast<unsigned>(expanded >> (42 - 6 * box)) & 0x3f;
        const unsigned row = (group >> 4 & 2) | (group & 1);
        const unsigned column = (group >> 1) & 0xf;
        substituted = substituted << 4 | SBoxes[box][row * 16 + column];
    }
    return static_cast<std::uint32_t>(Permute(substituted, RoundPermutation, 32, 32));
}

std::uint64_t CryptBlock(std::uint64_t block, const DesKeySchedule& schedule, bool encrypt) {
    const std::uint64_t permuted = Permute(block, InitialPermutation, 64, 64);
    std::uint32_t left = static_cast<std::uint32_t>(permuted >> 32);
    std::uint32_t right = static_cast<std::uint32_t>(permuted);
    for (int round = 0; round < 16; ++round) {
        const auto subkey = UnpackRound(schedule.ks[encrypt ? round : 15 - round]);
        const std::uint32_t next = left ^ RoundFunction(right, subkey);
        left = right;
        right = next;
    }
    return Permute(std::uint64_t(right) << 32 | left, FinalPermutation, 64, 64);
}
}

extern "C" {

void APS5_VABI DES_set_odd_parity_nid_postfix(unsigned char* key) {
    RequirePointer(key, __func__, "key");
    for (int i = 0; i < 8; ++i) {
        unsigned bits = key[i] >> 1;
        bits ^= bits >> 4;
        bits ^= bits >> 2;
        bits ^= bits >> 1;
        key[i] = static_cast<unsigned char>((key[i] & 0xfe) | (~bits & 1));
    }
}

int APS5_VABI DES_set_key_nid_postfix(const unsigned char* key, DesKeySchedule* schedule) {
    RequirePointer(key, __func__, "key");
    RequirePointer(schedule, __func__, "key schedule");
    ExpandKey(key, *schedule);
    return 0;
}

void APS5_VABI DES_ecb_encrypt_nid_postfix(const unsigned char* input, unsigned char* output, const DesKeySchedule* schedule, int enc) {
    RequirePointer(input, __func__, "input");
    RequirePointer(output, __func__, "output");
    RequirePointer(schedule, __func__, "key schedule");
    StoreBlock(output, CryptBlock(LoadBlock(input), *schedule, enc != 0));
}

}
