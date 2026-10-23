#include <array>
#include <utility>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include "prx/libc/include/General.hpp"

namespace {

constexpr int AesBlockSize = 16;
constexpr int AesMaxRounds = 14;

struct AesKey {
    std::uint32_t rdKey[4 * (AesMaxRounds + 1)];
    std::int32_t rounds;
};

static_assert(sizeof(AesKey) == 244);

using AesState = std::array<unsigned char, AesBlockSize>;

constexpr unsigned char Multiply(unsigned char a, unsigned char b) {
    unsigned char result = 0;
    while (b != 0) {
        if (b & 1) result ^= a;
        a = static_cast<unsigned char>((a << 1) ^ ((a & 0x80) ? 0x1b : 0));
        b >>= 1;
    }
    return result;
}

constexpr unsigned char RotateLeft(unsigned char value, int shift) {
    return static_cast<unsigned char>((value << shift) | (value >> (8 - shift)));
}

constexpr std::array<unsigned char, 256> MakeSbox() {
    std::array<unsigned char, 256> box{};
    unsigned char power = 1;
    unsigned char inverse = 1;
    do {
        power = static_cast<unsigned char>(power ^ (power << 1) ^ ((power & 0x80) ? 0x1b : 0));
        inverse ^= static_cast<unsigned char>(inverse << 1);
        inverse ^= static_cast<unsigned char>(inverse << 2);
        inverse ^= static_cast<unsigned char>(inverse << 4);
        if (inverse & 0x80) inverse ^= 0x09;
        box[power] = static_cast<unsigned char>(inverse ^ RotateLeft(inverse, 1) ^ RotateLeft(inverse, 2) ^
            RotateLeft(inverse, 3) ^ RotateLeft(inverse, 4) ^ 0x63);
    } while (power != 1);
    box[0] = 0x63;
    return box;
}

constexpr std::array<unsigned char, 256> MakeInverseSbox(const std::array<unsigned char, 256>& box) {
    std::array<unsigned char, 256> inverse{};
    for (int value = 0; value < 256; ++value) inverse[box[value]] = static_cast<unsigned char>(value);
    return inverse;
}

constexpr auto Sbox = MakeSbox();
constexpr auto InverseSbox = MakeInverseSbox(Sbox);

static_assert(Sbox[0x00] == 0x63 && Sbox[0x01] == 0x7c && Sbox[0x53] == 0xed && Sbox[0xff] == 0x16);

std::uint32_t LoadBig32(const unsigned char* p) {
    return std::uint32_t(p[0]) << 24 | std::uint32_t(p[1]) << 16 | std::uint32_t(p[2]) << 8 | std::uint32_t(p[3]);
}

std::uint32_t SubWord(std::uint32_t word) {
    return std::uint32_t(Sbox[word >> 24]) << 24 | std::uint32_t(Sbox[(word >> 16) & 0xff]) << 16 |
        std::uint32_t(Sbox[(word >> 8) & 0xff]) << 8 | Sbox[word & 0xff];
}

void AddRoundKey(AesState& state, const std::uint32_t* roundKey) {
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            state[4 * column + row] ^= static_cast<unsigned char>(roundKey[column] >> (24 - 8 * row));
        }
    }
}

void SubBytes(AesState& state, const std::array<unsigned char, 256>& box) {
    for (auto& byte : state) byte = box[byte];
}

void ShiftRows(AesState& state, bool inverse) {
    const AesState source = state;
    for (int row = 1; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            const int from = inverse ? (column + 4 - row) % 4 : (column + row) % 4;
            state[4 * column + row] = source[4 * from + row];
        }
    }
}

void MixColumn(unsigned char* column, const unsigned char (&matrix)[4]) {
    unsigned char source[4];
    std::memcpy(source, column, 4);
    for (int row = 0; row < 4; ++row) {
        column[row] = Multiply(source[0], matrix[(4 - row) % 4]) ^ Multiply(source[1], matrix[(5 - row) % 4]) ^
            Multiply(source[2], matrix[(6 - row) % 4]) ^ Multiply(source[3], matrix[(7 - row) % 4]);
    }
}

constexpr unsigned char MixMatrix[4] {2, 3, 1, 1};
constexpr unsigned char InverseMixMatrix[4] {14, 11, 13, 9};

void MixColumns(AesState& state, const unsigned char (&matrix)[4]) {
    for (int column = 0; column < 4; ++column) MixColumn(state.data() + 4 * column, matrix);
}

std::uint32_t InverseMixWord(std::uint32_t word) {
    unsigned char column[4];
    for (int row = 0; row < 4; ++row) column[row] = static_cast<unsigned char>(word >> (24 - 8 * row));
    MixColumn(column, InverseMixMatrix);
    return LoadBig32(column);
}

int SetEncryptKey(const unsigned char* userKey, int bits, AesKey* key) {
    if (userKey == nullptr || key == nullptr) return -1;
    if (bits != 128 && bits != 192 && bits != 256) return -2;
    const int keyWords = bits / 32;
    key->rounds = keyWords + 6;
    const int totalWords = 4 * (key->rounds + 1);
    std::uint32_t* words = key->rdKey;
    for (int index = 0; index < keyWords; ++index) words[index] = LoadBig32(userKey + 4 * index);
    std::uint32_t rcon = 0x01;
    for (int index = keyWords; index < totalWords; ++index) {
        std::uint32_t temp = words[index - 1];
        if (index % keyWords == 0) {
            temp = SubWord((temp << 8) | (temp >> 24)) ^ (rcon << 24);
            rcon = Multiply(static_cast<unsigned char>(rcon), 2);
        } else if (keyWords == 8 && index % keyWords == 4) {
            temp = SubWord(temp);
        }
        words[index] = words[index - keyWords] ^ temp;
    }
    return 0;
}

const AesKey& RequireKey(const void* key, const char* function) {
    if (key == nullptr) throw std::invalid_argument(std::string(function) + ": invalid argument");
    const auto& aesKey = *static_cast<const AesKey*>(key);
    if (aesKey.rounds != 10 && aesKey.rounds != 12 && aesKey.rounds != 14) {
        throw std::runtime_error(std::string(function) + ": AES_KEY rounds " + std::to_string(aesKey.rounds) + " is not 10, 12 or 14");
    }
    return aesKey;
}

void EncryptBlock(const unsigned char* in, unsigned char* out, const AesKey& key) {
    AesState state;
    std::memcpy(state.data(), in, AesBlockSize);
    AddRoundKey(state, key.rdKey);
    for (int round = 1; round < key.rounds; ++round) {
        SubBytes(state, Sbox);
        ShiftRows(state, false);
        MixColumns(state, MixMatrix);
        AddRoundKey(state, key.rdKey + 4 * round);
    }
    SubBytes(state, Sbox);
    ShiftRows(state, false);
    AddRoundKey(state, key.rdKey + 4 * key.rounds);
    std::memcpy(out, state.data(), AesBlockSize);
}

void DecryptBlock(const unsigned char* in, unsigned char* out, const AesKey& key) {
    AesState state;
    std::memcpy(state.data(), in, AesBlockSize);
    AddRoundKey(state, key.rdKey);
    for (int round = 1; round < key.rounds; ++round) {
        SubBytes(state, InverseSbox);
        ShiftRows(state, true);
        MixColumns(state, InverseMixMatrix);
        AddRoundKey(state, key.rdKey + 4 * round);
    }
    SubBytes(state, InverseSbox);
    ShiftRows(state, true);
    AddRoundKey(state, key.rdKey + 4 * key.rounds);
    std::memcpy(out, state.data(), AesBlockSize);
}

void RequireBuffers(const unsigned char* in, unsigned char* out, std::size_t length, const char* function) {
    if (length != 0 && (in == nullptr || out == nullptr)) throw std::invalid_argument(std::string(function) + ": invalid argument");
}

}

extern "C" {

int APS5_VABI AES_set_encrypt_key_nid_postfix(const unsigned char* userKey, int bits, AesKey* key) {
    return SetEncryptKey(userKey, bits, key);
}

int APS5_VABI AES_set_decrypt_key_nid_postfix(const unsigned char* userKey, int bits, AesKey* key) {
    const int status = SetEncryptKey(userKey, bits, key);
    if (status < 0) return status;
    std::uint32_t* words = key->rdKey;
    for (int low = 0, high = 4 * key->rounds; low < high; low += 4, high -= 4) {
        for (int column = 0; column < 4; ++column) std::swap(words[low + column], words[high + column]);
    }
    for (int index = 4; index < 4 * key->rounds; ++index) words[index] = InverseMixWord(words[index]);
    return 0;
}

void APS5_VABI AES_encrypt_nid_postfix(const unsigned char* in, unsigned char* out, const AesKey* key) {
    const auto& aesKey = RequireKey(key, __func__);
    if (in == nullptr || out == nullptr) APS5_INVALID_ARG_EX;
    EncryptBlock(in, out, aesKey);
}

void APS5_VABI AES_decrypt_nid_postfix(const unsigned char* in, unsigned char* out, const AesKey* key) {
    const auto& aesKey = RequireKey(key, __func__);
    if (in == nullptr || out == nullptr) APS5_INVALID_ARG_EX;
    DecryptBlock(in, out, aesKey);
}

void APS5_VABI AES_ecb_encrypt_nid_postfix(const unsigned char* in, unsigned char* out, const AesKey* key, int enc) {
    const auto& aesKey = RequireKey(key, __func__);
    if (in == nullptr || out == nullptr) APS5_INVALID_ARG_EX;
    if (enc == 1) EncryptBlock(in, out, aesKey);
    else DecryptBlock(in, out, aesKey);
}

void APS5_VABI AES_cbc_encrypt_nid_postfix(const unsigned char* in, unsigned char* out, std::size_t length, const AesKey* key,
    unsigned char* ivec, int enc) {
    const auto& aesKey = RequireKey(key, __func__);
    if (ivec == nullptr) APS5_INVALID_ARG_EX;
    RequireBuffers(in, out, length, __func__);
    if (enc != 0) {
        const unsigned char* iv = ivec;
        while (length != 0) {
            std::size_t index = 0;
            for (; index < AesBlockSize && index < length; ++index) out[index] = in[index] ^ iv[index];
            for (; index < AesBlockSize; ++index) out[index] = iv[index];
            EncryptBlock(out, out, aesKey);
            iv = out;
            if (length <= AesBlockSize) break;
            length -= AesBlockSize;
            in += AesBlockSize;
            out += AesBlockSize;
        }
        std::memmove(ivec, iv, AesBlockSize);
        return;
    }
    while (length != 0) {
        unsigned char decrypted[AesBlockSize];
        DecryptBlock(in, decrypted, aesKey);
        std::size_t index = 0;
        for (; index < AesBlockSize && index < length; ++index) {
            const unsigned char cipher = in[index];
            out[index] = decrypted[index] ^ ivec[index];
            ivec[index] = cipher;
        }
        if (length <= AesBlockSize) {
            for (; index < AesBlockSize; ++index) ivec[index] = in[index];
            break;
        }
        length -= AesBlockSize;
        in += AesBlockSize;
        out += AesBlockSize;
    }
}

void APS5_VABI AES_cfb128_encrypt_nid_postfix(const unsigned char* in, unsigned char* out, std::size_t length, const AesKey* key,
    unsigned char* ivec, int* num, int enc) {
    const auto& aesKey = RequireKey(key, __func__);
    if (ivec == nullptr || num == nullptr) APS5_INVALID_ARG_EX;
    RequireBuffers(in, out, length, __func__);
    auto position = static_cast<unsigned int>(*num);
    if (position >= AesBlockSize) throw std::runtime_error(std::string(__func__) + ": *num " + std::to_string(*num) + " is not below 16");
    for (std::size_t index = 0; index < length; ++index) {
        if (position == 0) EncryptBlock(ivec, ivec, aesKey);
        const unsigned char input = in[index];
        out[index] = ivec[position] ^ input;
        ivec[position] = enc != 0 ? out[index] : input;
        position = (position + 1) % AesBlockSize;
    }
    *num = static_cast<int>(position);
}

void APS5_VABI AES_ctr128_encrypt_nid_postfix(const unsigned char* in, unsigned char* out, std::size_t length, const AesKey* key,
    unsigned char* ivec, unsigned char* ecountBuf, unsigned int* num) {
    const auto& aesKey = RequireKey(key, __func__);
    if (ivec == nullptr || ecountBuf == nullptr || num == nullptr) APS5_INVALID_ARG_EX;
    RequireBuffers(in, out, length, __func__);
    unsigned int position = *num;
    if (position >= AesBlockSize) throw std::runtime_error(std::string(__func__) + ": *num " + std::to_string(position) + " is not below 16");
    for (std::size_t index = 0; index < length; ++index) {
        if (position == 0) {
            EncryptBlock(ivec, ecountBuf, aesKey);
            for (int byte = AesBlockSize - 1; byte >= 0 && ++ivec[byte] == 0; --byte) {}
        }
        out[index] = in[index] ^ ecountBuf[position];
        position = (position + 1) % AesBlockSize;
    }
    *num = position;
}

}
