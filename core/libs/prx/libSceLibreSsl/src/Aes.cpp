#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>
#include "prx/libc/include/General.hpp"

namespace {
struct AesKey {
    std::uint32_t rdKey[60];
    int rounds;
};

static_assert(sizeof(AesKey) == 244);

constexpr std::size_t BlockSize = 16;

std::uint8_t Xtime(std::uint8_t x) {
    return static_cast<std::uint8_t>((x << 1) ^ ((x & 0x80) ? 0x1b : 0));
}

std::uint8_t Multiply(std::uint8_t a, std::uint8_t b) {
    std::uint8_t result = 0;
    for (; b; b >>= 1) {
        if (b & 1) result ^= a;
        a = Xtime(a);
    }
    return result;
}

struct Tables {
    std::uint8_t sbox[256];
    std::uint8_t inverseSbox[256];

    Tables() {
        for (int x = 0; x < 256; ++x) {
            std::uint8_t inverse = 0;
            if (x) {
                for (int y = 1; y < 256; ++y) {
                    if (Multiply(static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y)) == 1) {
                        inverse = static_cast<std::uint8_t>(y);
                        break;
                    }
                }
            }
            std::uint8_t value = inverse;
            for (int shift = 1; shift <= 4; ++shift)
                value ^= static_cast<std::uint8_t>((inverse << shift) | (inverse >> (8 - shift)));
            value ^= 0x63;
            sbox[x] = value;
            inverseSbox[value] = static_cast<std::uint8_t>(x);
        }
    }
};

const Tables& GetTables() {
    static const Tables tables;
    return tables;
}

std::uint32_t LoadBig32(const unsigned char* p) {
    return std::uint32_t(p[0]) << 24 | std::uint32_t(p[1]) << 16 | std::uint32_t(p[2]) << 8 | std::uint32_t(p[3]);
}

std::uint32_t SubWord(std::uint32_t word) {
    const auto& t = GetTables();
    return std::uint32_t(t.sbox[word >> 24]) << 24 | std::uint32_t(t.sbox[(word >> 16) & 0xff]) << 16 |
           std::uint32_t(t.sbox[(word >> 8) & 0xff]) << 8 | std::uint32_t(t.sbox[word & 0xff]);
}

std::uint32_t InverseMixWord(std::uint32_t word) {
    const auto a0 = static_cast<std::uint8_t>(word >> 24);
    const auto a1 = static_cast<std::uint8_t>(word >> 16);
    const auto a2 = static_cast<std::uint8_t>(word >> 8);
    const auto a3 = static_cast<std::uint8_t>(word);
    const std::uint8_t b0 = Multiply(a0, 14) ^ Multiply(a1, 11) ^ Multiply(a2, 13) ^ Multiply(a3, 9);
    const std::uint8_t b1 = Multiply(a0, 9) ^ Multiply(a1, 14) ^ Multiply(a2, 11) ^ Multiply(a3, 13);
    const std::uint8_t b2 = Multiply(a0, 13) ^ Multiply(a1, 9) ^ Multiply(a2, 14) ^ Multiply(a3, 11);
    const std::uint8_t b3 = Multiply(a0, 11) ^ Multiply(a1, 13) ^ Multiply(a2, 9) ^ Multiply(a3, 14);
    return std::uint32_t(b0) << 24 | std::uint32_t(b1) << 16 | std::uint32_t(b2) << 8 | std::uint32_t(b3);
}

void ExpandEncryptKey(const unsigned char* userKey, int bits, AesKey& key) {
    const int words = bits / 32;
    key.rounds = words + 6;
    const int total = 4 * (key.rounds + 1);
    for (int i = 0; i < words; ++i) key.rdKey[i] = LoadBig32(userKey + 4 * i);
    std::uint8_t rcon = 1;
    for (int i = words; i < total; ++i) {
        std::uint32_t temp = key.rdKey[i - 1];
        if (i % words == 0) {
            temp = SubWord(temp << 8 | temp >> 24) ^ (std::uint32_t(rcon) << 24);
            rcon = Xtime(rcon);
        } else if (words > 6 && i % words == 4) {
            temp = SubWord(temp);
        }
        key.rdKey[i] = key.rdKey[i - words] ^ temp;
    }
}

const AesKey& RequireKey(const AesKey* key, const char* function) {
    if (!key) throw std::invalid_argument(std::string(function) + ": null key");
    if (key->rounds != 10 && key->rounds != 12 && key->rounds != 14)
        throw std::invalid_argument(std::string(function) + ": key schedule has an invalid round count");
    return *key;
}

void RequireBuffer(const void* data, std::size_t length, const char* function, const char* what) {
    if (!data && length) throw std::invalid_argument(std::string(function) + ": null " + what);
}

[[noreturn]] void ThrowUnsupported(const char* function, const char* what) {
    throw std::runtime_error(std::string(function) + ": " + what);
}

void RequireBlock(const void* data, const char* function, const char* what) {
    if (!data) throw std::invalid_argument(std::string(function) + ": null " + what);
}

void AddRoundKey(unsigned char* state, const std::uint32_t* roundKey) {
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            state[4 * column + row] ^= static_cast<unsigned char>(roundKey[column] >> (24 - 8 * row));
}

void SubBytes(unsigned char* state, const std::uint8_t* box) {
    for (int i = 0; i < 16; ++i) state[i] = box[state[i]];
}

void ShiftRows(unsigned char* state) {
    unsigned char copy[16];
    std::memcpy(copy, state, 16);
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row) state[4 * column + row] = copy[4 * ((column + row) % 4) + row];
}

void InverseShiftRows(unsigned char* state) {
    unsigned char copy[16];
    std::memcpy(copy, state, 16);
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row) state[4 * ((column + row) % 4) + row] = copy[4 * column + row];
}

void MixColumns(unsigned char* state) {
    for (int column = 0; column < 4; ++column) {
        unsigned char* a = state + 4 * column;
        const unsigned char a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3];
        a[0] = static_cast<unsigned char>(Xtime(a0) ^ (Xtime(a1) ^ a1) ^ a2 ^ a3);
        a[1] = static_cast<unsigned char>(a0 ^ Xtime(a1) ^ (Xtime(a2) ^ a2) ^ a3);
        a[2] = static_cast<unsigned char>(a0 ^ a1 ^ Xtime(a2) ^ (Xtime(a3) ^ a3));
        a[3] = static_cast<unsigned char>((Xtime(a0) ^ a0) ^ a1 ^ a2 ^ Xtime(a3));
    }
}

void InverseMixColumns(unsigned char* state) {
    for (int column = 0; column < 4; ++column) {
        unsigned char* a = state + 4 * column;
        const unsigned char a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3];
        a[0] = Multiply(a0, 14) ^ Multiply(a1, 11) ^ Multiply(a2, 13) ^ Multiply(a3, 9);
        a[1] = Multiply(a0, 9) ^ Multiply(a1, 14) ^ Multiply(a2, 11) ^ Multiply(a3, 13);
        a[2] = Multiply(a0, 13) ^ Multiply(a1, 9) ^ Multiply(a2, 14) ^ Multiply(a3, 11);
        a[3] = Multiply(a0, 11) ^ Multiply(a1, 13) ^ Multiply(a2, 9) ^ Multiply(a3, 14);
    }
}

void EncryptBlock(const AesKey& key, const unsigned char* in, unsigned char* out) {
    const auto& box = GetTables().sbox;
    unsigned char state[16];
    std::memcpy(state, in, 16);
    AddRoundKey(state, key.rdKey);
    for (int round = 1; round < key.rounds; ++round) {
        SubBytes(state, box);
        ShiftRows(state);
        MixColumns(state);
        AddRoundKey(state, key.rdKey + 4 * round);
    }
    SubBytes(state, box);
    ShiftRows(state);
    AddRoundKey(state, key.rdKey + 4 * key.rounds);
    std::memcpy(out, state, 16);
}

void DecryptBlock(const AesKey& key, const unsigned char* in, unsigned char* out) {
    const auto& box = GetTables().inverseSbox;
    unsigned char state[16];
    std::memcpy(state, in, 16);
    AddRoundKey(state, key.rdKey);
    for (int round = 1; round < key.rounds; ++round) {
        SubBytes(state, box);
        InverseShiftRows(state);
        InverseMixColumns(state);
        AddRoundKey(state, key.rdKey + 4 * round);
    }
    SubBytes(state, box);
    InverseShiftRows(state);
    AddRoundKey(state, key.rdKey + 4 * key.rounds);
    std::memcpy(out, state, 16);
}

void IncrementCounter(unsigned char* counter) {
    for (int i = 15; i >= 0; --i)
        if (++counter[i]) break;
}
}

extern "C" {

int APS5_VABI AES_set_encrypt_key_nid_postfix(const unsigned char* userKey, int bits, AesKey* key) {
    if (!userKey || !key) return -1;
    if (bits != 128 && bits != 192 && bits != 256) return -2;
    ExpandEncryptKey(userKey, bits, *key);
    return 0;
}

int APS5_VABI AES_set_decrypt_key_nid_postfix(const unsigned char* userKey, int bits, AesKey* key) {
    if (!userKey || !key) return -1;
    if (bits != 128 && bits != 192 && bits != 256) return -2;
    ExpandEncryptKey(userKey, bits, *key);
    for (int i = 0, j = 4 * key->rounds; i < j; i += 4, j -= 4)
        for (int k = 0; k < 4; ++k) std::swap(key->rdKey[i + k], key->rdKey[j + k]);
    for (int i = 4; i < 4 * key->rounds; ++i) key->rdKey[i] = InverseMixWord(key->rdKey[i]);
    return 0;
}

void APS5_VABI AES_encrypt_nid_postfix(const unsigned char* in, unsigned char* out, const AesKey* key) {
    const auto& k = RequireKey(key, __func__);
    RequireBlock(in, __func__, "input");
    RequireBlock(out, __func__, "output");
    EncryptBlock(k, in, out);
}

void APS5_VABI AES_decrypt_nid_postfix(const unsigned char* in, unsigned char* out, const AesKey* key) {
    const auto& k = RequireKey(key, __func__);
    RequireBlock(in, __func__, "input");
    RequireBlock(out, __func__, "output");
    DecryptBlock(k, in, out);
}

void APS5_VABI AES_ecb_encrypt_nid_postfix(const unsigned char* in, unsigned char* out, const AesKey* key, int enc) {
    const auto& k = RequireKey(key, __func__);
    RequireBlock(in, __func__, "input");
    RequireBlock(out, __func__, "output");
    if (enc != 0 && enc != 1) ThrowUnsupported(__func__, "enc other than AES_ENCRYPT or AES_DECRYPT");
    if (enc) EncryptBlock(k, in, out);
    else DecryptBlock(k, in, out);
}

void APS5_VABI AES_cbc_encrypt_nid_postfix(const unsigned char* in, unsigned char* out, std::size_t length, const AesKey* key, unsigned char* ivec, int enc) {
    const auto& k = RequireKey(key, __func__);
    RequireBlock(ivec, __func__, "iv");
    RequireBuffer(in, length, __func__, "input");
    RequireBuffer(out, length, __func__, "output");
    if (length % BlockSize) ThrowUnsupported(__func__, "length that is not a multiple of the block size");
    for (; length; length -= BlockSize, in += BlockSize, out += BlockSize) {
        unsigned char block[BlockSize];
        if (enc) {
            for (std::size_t i = 0; i < BlockSize; ++i) block[i] = in[i] ^ ivec[i];
            EncryptBlock(k, block, out);
            std::memcpy(ivec, out, BlockSize);
        } else {
            unsigned char cipher[BlockSize];
            std::memcpy(cipher, in, BlockSize);
            DecryptBlock(k, in, block);
            for (std::size_t i = 0; i < BlockSize; ++i) out[i] = block[i] ^ ivec[i];
            std::memcpy(ivec, cipher, BlockSize);
        }
    }
}

void APS5_VABI AES_cfb128_encrypt_nid_postfix(const unsigned char* in, unsigned char* out, std::size_t length, const AesKey* key, unsigned char* ivec, int* num, int enc) {
    const auto& k = RequireKey(key, __func__);
    RequireBlock(ivec, __func__, "iv");
    RequireBlock(num, __func__, "num");
    RequireBuffer(in, length, __func__, "input");
    RequireBuffer(out, length, __func__, "output");
    if (*num < 0 || *num >= static_cast<int>(BlockSize)) throw std::invalid_argument(std::string(__func__) + ": num is not below the block size");
    unsigned int n = static_cast<unsigned int>(*num);
    for (std::size_t i = 0; i < length; ++i) {
        if (n == 0) EncryptBlock(k, ivec, ivec);
        if (enc) {
            ivec[n] ^= in[i];
            out[i] = ivec[n];
        } else {
            const unsigned char cipher = in[i];
            out[i] = ivec[n] ^ cipher;
            ivec[n] = cipher;
        }
        n = (n + 1) % BlockSize;
    }
    *num = static_cast<int>(n);
}

void APS5_VABI AES_ctr128_encrypt_nid_postfix(const unsigned char* in, unsigned char* out, std::size_t length, const AesKey* key, unsigned char* ivec, unsigned char* ecountBuf, unsigned int* num) {
    const auto& k = RequireKey(key, __func__);
    RequireBlock(ivec, __func__, "counter");
    RequireBlock(ecountBuf, __func__, "keystream buffer");
    RequireBlock(num, __func__, "num");
    RequireBuffer(in, length, __func__, "input");
    RequireBuffer(out, length, __func__, "output");
    if (*num >= BlockSize) throw std::invalid_argument(std::string(__func__) + ": num is not below the block size");
    unsigned int n = *num;
    for (std::size_t i = 0; i < length; ++i) {
        if (n == 0) {
            EncryptBlock(k, ivec, ecountBuf);
            IncrementCounter(ivec);
        }
        out[i] = in[i] ^ ecountBuf[n];
        n = (n + 1) % BlockSize;
    }
    *num = n;
}

}
