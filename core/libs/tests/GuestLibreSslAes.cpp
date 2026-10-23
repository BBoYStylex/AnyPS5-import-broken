#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

struct AesKey {
    std::uint32_t roundKeys[60];
    std::int32_t rounds;
};

extern "C" {
int APS5_VABI AES_set_encrypt_key_nid_postfix(const unsigned char*, int, AesKey*);
int APS5_VABI AES_set_decrypt_key_nid_postfix(const unsigned char*, int, AesKey*);
void APS5_VABI AES_encrypt_nid_postfix(const unsigned char*, unsigned char*, const AesKey*);
void APS5_VABI AES_decrypt_nid_postfix(const unsigned char*, unsigned char*, const AesKey*);
void APS5_VABI AES_ecb_encrypt_nid_postfix(const unsigned char*, unsigned char*, const AesKey*, int);
void APS5_VABI AES_cbc_encrypt_nid_postfix(const unsigned char*, unsigned char*, std::size_t, const AesKey*, unsigned char*, int);
void APS5_VABI AES_cfb128_encrypt_nid_postfix(const unsigned char*, unsigned char*, std::size_t, const AesKey*, unsigned char*, int*, int);
void APS5_VABI AES_ctr128_encrypt_nid_postfix(const unsigned char*, unsigned char*, std::size_t, const AesKey*, unsigned char*, unsigned char*, unsigned*);
}

static void Require(bool value, const char* what) {
    if (value) return;
    std::fprintf(stderr, "failed: %s\n", what);
    std::abort();
}

template<typename TAction> static void RequireThrows(TAction action, const char* what) {
    try { action(); } catch (const std::exception&) { return; }
    Require(false, what);
}

static std::vector<unsigned char> Bytes(const char* hex) {
    std::vector<unsigned char> bytes;
    for (; hex[0] && hex[1]; hex += 2) bytes.push_back(static_cast<unsigned char>(std::stoi(std::string(hex, 2), nullptr, 16)));
    return bytes;
}

static std::string Hex(const unsigned char* data, std::size_t size) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string text;
    for (std::size_t i = 0; i < size; ++i) {
        text += digits[data[i] >> 4];
        text += digits[data[i] & 15];
    }
    return text;
}

int main() {
    const auto plain = Bytes("00112233445566778899aabbccddeeff");
    const char* keys[3] {
        "000102030405060708090a0b0c0d0e0f",
        "000102030405060708090a0b0c0d0e0f1011121314151617",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
    };
    const char* ciphers[3] {"69c4e0d86a7b0430d8cdb78070b4c55a", "dda97ca4864cdfe06eaf70a0ec0d7191", "8ea2b7ca516745bfeafc49904b496089"};
    for (int i = 0; i < 3; ++i) {
        const auto key = Bytes(keys[i]);
        const int bits = static_cast<int>(key.size() * 8);
        AesKey encrypt {}, decrypt {};
        unsigned char out[16] {}, back[16] {};
        Require(AES_set_encrypt_key_nid_postfix(key.data(), bits, &encrypt) == 0 && encrypt.rounds == 10 + 2 * i, "encrypt key");
        Require(AES_set_decrypt_key_nid_postfix(key.data(), bits, &decrypt) == 0 && decrypt.rounds == encrypt.rounds, "decrypt key");
        const int last = 4 * encrypt.rounds;
        Require(!std::memcmp(decrypt.roundKeys, encrypt.roundKeys + last, 16) && !std::memcmp(decrypt.roundKeys + last, encrypt.roundKeys, 16), "decrypt key reverses the round keys");
        AES_encrypt_nid_postfix(plain.data(), out, &encrypt);
        Require(Hex(out, 16) == ciphers[i], "FIPS-197 appendix C cipher");
        AES_decrypt_nid_postfix(out, back, &decrypt);
        Require(!std::memcmp(back, plain.data(), 16), "FIPS-197 appendix C inverse cipher");
        AES_ecb_encrypt_nid_postfix(plain.data(), out, &encrypt, 1);
        Require(Hex(out, 16) == ciphers[i], "ECB encrypt");
        AES_ecb_encrypt_nid_postfix(out, out, &decrypt, 0);
        Require(!std::memcmp(out, plain.data(), 16), "ECB decrypt in place");
    }

    AesKey fipsKey {};
    Require(AES_set_encrypt_key_nid_postfix(Bytes("2b7e151628aed2a6abf7158809cf4f3c").data(), 128, &fipsKey) == 0, "FIPS-197 appendix A key");
    Require(fipsKey.roundKeys[4] == 0xa0fafe17 && fipsKey.roundKeys[43] == 0xb6630ca6, "FIPS-197 appendix A.1 expansion");
    AesKey bad {};
    bad.rounds = 7;
    bad.roundKeys[0] = 1;
    const unsigned char zero[32] {};
    Require(AES_set_encrypt_key_nid_postfix(zero, 64, &bad) == -2 && bad.rounds == 0 && bad.roundKeys[0] == 0, "unsupported key length");
    Require(AES_set_decrypt_key_nid_postfix(zero, 129, &bad) == -2, "unsupported decrypt key length");
    Require(AES_set_encrypt_key_nid_postfix(nullptr, 128, &bad) == -1 && AES_set_encrypt_key_nid_postfix(zero, 128, nullptr) == -1, "null key arguments");
    unsigned char block[16] {};
    RequireThrows([&] { AES_encrypt_nid_postfix(zero, block, &bad); }, "key schedule without rounds");

    const auto key = Bytes("2b7e151628aed2a6abf7158809cf4f3c");
    const auto message = Bytes("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710");
    AesKey encrypt {}, decrypt {};
    AES_set_encrypt_key_nid_postfix(key.data(), 128, &encrypt);
    AES_set_decrypt_key_nid_postfix(key.data(), 128, &decrypt);
    std::vector<unsigned char> out(message.size() + 16), back(message.size() + 16);

    auto iv = Bytes("000102030405060708090a0b0c0d0e0f");
    AES_cbc_encrypt_nid_postfix(message.data(), out.data(), 64, &encrypt, iv.data(), 1);
    Require(Hex(out.data(), 64) == "7649abac8119b246cee98e9b12e9197d5086cb9b507219ee95db113a917678b273bed6b8e3c1743b7116e69e222295163ff1caa1681fac09120eca307586e1a7", "SP 800-38A F.2.1 CBC encrypt");
    Require(Hex(iv.data(), 16) == "3ff1caa1681fac09120eca307586e1a7", "CBC encrypt returns the last cipher block as iv");
    iv = Bytes("000102030405060708090a0b0c0d0e0f");
    std::memcpy(back.data(), out.data(), 64);
    AES_cbc_encrypt_nid_postfix(back.data(), back.data(), 32, &decrypt, iv.data(), 0);
    AES_cbc_encrypt_nid_postfix(back.data() + 32, back.data() + 32, 32, &decrypt, iv.data(), 0);
    Require(!std::memcmp(back.data(), message.data(), 64) && Hex(iv.data(), 16) == "3ff1caa1681fac09120eca307586e1a7", "SP 800-38A F.2.2 CBC decrypt in place across calls");
    iv = Bytes("000102030405060708090a0b0c0d0e0f");
    std::vector<unsigned char> partial(32, 0xee);
    AES_cbc_encrypt_nid_postfix(message.data(), partial.data(), 20, &encrypt, iv.data(), 1);
    Require(Hex(partial.data(), 16) == "7649abac8119b246cee98e9b12e9197d" && partial[31] != 0xee && Hex(iv.data(), 16) == Hex(partial.data() + 16, 16), "CBC encrypt pads a partial block with the iv and writes the whole block");
    iv = Bytes("000102030405060708090a0b0c0d0e0f");
    std::vector<unsigned char> plainPartial(32, 0xee);
    AES_cbc_encrypt_nid_postfix(partial.data(), plainPartial.data(), 20, &decrypt, iv.data(), 0);
    Require(!std::memcmp(plainPartial.data(), message.data(), 20) && plainPartial[20] == 0xee && Hex(iv.data(), 16) == Hex(partial.data() + 16, 16), "CBC decrypt writes only the requested length");

    iv = Bytes("000102030405060708090a0b0c0d0e0f");
    int num = 0;
    AES_cfb128_encrypt_nid_postfix(message.data(), out.data(), 5, &encrypt, iv.data(), &num, 1);
    AES_cfb128_encrypt_nid_postfix(message.data() + 5, out.data() + 5, 32, &encrypt, iv.data(), &num, 1);
    Require(num == 5, "CFB128 keeps the block position");
    AES_cfb128_encrypt_nid_postfix(message.data() + 37, out.data() + 37, 27, &encrypt, iv.data(), &num, 1);
    Require(Hex(out.data(), 64) == "3b3fd92eb72dad20333449f8e83cfb4ac8a64537a0b3a93fcde3cdad9f1ce58b26751f67a3cbb140b1808cf187a4f4dfc04b05357c5d1c0eeac4c66f9ff7f2e6" && num == 0, "SP 800-38A F.3.13 CFB128 encrypt in pieces");
    iv = Bytes("000102030405060708090a0b0c0d0e0f");
    num = 0;
    AES_cfb128_encrypt_nid_postfix(out.data(), back.data(), 64, &encrypt, iv.data(), &num, 0);
    Require(!std::memcmp(back.data(), message.data(), 64), "SP 800-38A F.3.14 CFB128 decrypt");
    num = 16;
    RequireThrows([&] { AES_cfb128_encrypt_nid_postfix(message.data(), out.data(), 1, &encrypt, iv.data(), &num, 1); }, "CFB128 position out of range");

    auto counter = Bytes("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
    unsigned char keystream[16] {};
    unsigned position = 0;
    AES_ctr128_encrypt_nid_postfix(message.data(), out.data(), 7, &encrypt, counter.data(), keystream, &position);
    AES_ctr128_encrypt_nid_postfix(message.data() + 7, out.data() + 7, 57, &encrypt, counter.data(), keystream, &position);
    Require(Hex(out.data(), 64) == "874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee", "SP 800-38A F.5.1 CTR encrypt in pieces");
    Require(position == 0 && Hex(counter.data(), 16) == "f0f1f2f3f4f5f6f7f8f9fafbfcfdff03", "CTR advances the counter per block");
    counter = Bytes("ffffffffffffffffffffffffffffffff");
    position = 0;
    AES_ctr128_encrypt_nid_postfix(message.data(), out.data(), 37, &encrypt, counter.data(), keystream, &position);
    Require(Hex(out.data(), 37) == "e13338e36cb71962e00d020b4cedbd86d3dae15b04bb352fa0f59febfcb4da3e67da610697", "CTR counter wraps across all 128 bits");
    Require(position == 5 && Hex(counter.data(), 16) == "00000000000000000000000000000002", "CTR position and counter after a partial block");
    position = 16;
    RequireThrows([&] { AES_ctr128_encrypt_nid_postfix(message.data(), out.data(), 1, &encrypt, counter.data(), keystream, &position); }, "CTR position out of range");

    std::puts("libSceLibreSsl AES tests passed");
}
