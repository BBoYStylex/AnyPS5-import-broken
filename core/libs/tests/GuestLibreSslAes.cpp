#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

struct AesKey {
    std::uint32_t rdKey[60];
    std::int32_t rounds;
};

using Block = std::array<unsigned char, 16>;
using Bytes = std::vector<unsigned char>;

extern "C" {
int APS5_VABI AES_set_encrypt_key_nid_postfix(const unsigned char*, int, AesKey*);
int APS5_VABI AES_set_decrypt_key_nid_postfix(const unsigned char*, int, AesKey*);
void APS5_VABI AES_encrypt_nid_postfix(const unsigned char*, unsigned char*, const AesKey*);
void APS5_VABI AES_decrypt_nid_postfix(const unsigned char*, unsigned char*, const AesKey*);
void APS5_VABI AES_ecb_encrypt_nid_postfix(const unsigned char*, unsigned char*, const AesKey*, int);
void APS5_VABI AES_cbc_encrypt_nid_postfix(const unsigned char*, unsigned char*, std::size_t, const AesKey*, unsigned char*, int);
void APS5_VABI AES_cfb128_encrypt_nid_postfix(const unsigned char*, unsigned char*, std::size_t, const AesKey*, unsigned char*, int*, int);
void APS5_VABI AES_ctr128_encrypt_nid_postfix(const unsigned char*, unsigned char*, std::size_t, const AesKey*, unsigned char*, unsigned char*, unsigned int*);
}

static void Require(bool value) { if (!value) std::abort(); }

template<typename TException, typename TAction>
static void RequireThrows(TAction action) {
    try { action(); } catch (const TException&) { return; }
    std::abort();
}

static Bytes Hex(const std::string& text) {
    Bytes bytes;
    for (std::size_t index = 0; index + 1 < text.size(); index += 2) bytes.push_back(static_cast<unsigned char>(std::stoi(text.substr(index, 2), nullptr, 16)));
    return bytes;
}

static Block ToBlock(const Bytes& bytes) {
    Block block{};
    std::memcpy(block.data(), bytes.data(), block.size());
    return block;
}

static AesKey EncryptKey(const Bytes& key) {
    AesKey schedule{};
    Require(AES_set_encrypt_key_nid_postfix(key.data(), static_cast<int>(key.size() * 8), &schedule) == 0);
    return schedule;
}

static AesKey DecryptKey(const Bytes& key) {
    AesKey schedule{};
    Require(AES_set_decrypt_key_nid_postfix(key.data(), static_cast<int>(key.size() * 8), &schedule) == 0);
    return schedule;
}

static void CheckBlock(const std::string& key, const std::string& plain, const std::string& cipher) {
    const auto keyBytes = Hex(key);
    const auto encryptKey = EncryptKey(keyBytes);
    const auto decryptKey = DecryptKey(keyBytes);
    Require(encryptKey.rounds == static_cast<int>(keyBytes.size() / 4 + 6));
    Require(decryptKey.rounds == encryptKey.rounds);
    const auto plainBlock = ToBlock(Hex(plain));
    const auto cipherBlock = ToBlock(Hex(cipher));
    Block output{};
    AES_encrypt_nid_postfix(plainBlock.data(), output.data(), &encryptKey);
    Require(output == cipherBlock);
    AES_decrypt_nid_postfix(output.data(), output.data(), &decryptKey);
    Require(output == plainBlock);
    AES_ecb_encrypt_nid_postfix(plainBlock.data(), output.data(), &encryptKey, 1);
    Require(output == cipherBlock);
    AES_ecb_encrypt_nid_postfix(cipherBlock.data(), output.data(), &decryptKey, 0);
    Require(output == plainBlock);
    AES_ecb_encrypt_nid_postfix(cipherBlock.data(), output.data(), &decryptKey, 2);
    Require(output == plainBlock);
}

static void CheckSchedule() {
    const auto schedule = EncryptKey(Hex("2b7e151628aed2a6abf7158809cf4f3c"));
    Require(schedule.rdKey[0] == 0x2b7e1516 && schedule.rdKey[3] == 0x09cf4f3c);
    Require(schedule.rdKey[4] == 0xa0fafe17 && schedule.rdKey[43] == 0xb6630ca6);
    AesKey untouched{};
    std::memset(&untouched, 0x5a, sizeof(untouched));
    Require(AES_set_encrypt_key_nid_postfix(Hex("2b7e151628aed2a6abf7158809cf4f3c").data(), 128, &untouched) == 0);
    Require(untouched.rdKey[44] == 0x5a5a5a5a && untouched.rdKey[59] == 0x5a5a5a5a);
    const auto inverse = DecryptKey(Hex("2b7e151628aed2a6abf7158809cf4f3c"));
    Require(inverse.rdKey[0] == schedule.rdKey[40] && inverse.rdKey[3] == schedule.rdKey[43]);
    Require(inverse.rdKey[40] == schedule.rdKey[0] && inverse.rdKey[43] == schedule.rdKey[3]);
}

static const std::string NistKey = "2b7e151628aed2a6abf7158809cf4f3c";
static const std::string NistPlain =
    "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710";

static void CheckCbc() {
    const auto plain = Hex(NistPlain);
    const auto expected = Hex(
        "7649abac8119b246cee98e9b12e9197d5086cb9b507219ee95db113a917678b273bed6b8e3c1743b7116e69e222295163ff1caa1681fac09120eca307586e1a7");
    const auto encryptKey = EncryptKey(Hex(NistKey));
    const auto decryptKey = DecryptKey(Hex(NistKey));
    auto iv = Hex("000102030405060708090a0b0c0d0e0f");
    Bytes cipher(plain.size());
    AES_cbc_encrypt_nid_postfix(plain.data(), cipher.data(), plain.size(), &encryptKey, iv.data(), 1);
    Require(cipher == expected);
    Require(iv == Bytes(expected.end() - 16, expected.end()));
    iv = Hex("000102030405060708090a0b0c0d0e0f");
    Bytes split(plain.size());
    AES_cbc_encrypt_nid_postfix(plain.data(), split.data(), 16, &encryptKey, iv.data(), 7);
    AES_cbc_encrypt_nid_postfix(plain.data() + 16, split.data() + 16, 48, &encryptKey, iv.data(), 7);
    Require(split == expected);
    iv = Hex("000102030405060708090a0b0c0d0e0f");
    auto inPlace = expected;
    AES_cbc_encrypt_nid_postfix(inPlace.data(), inPlace.data(), inPlace.size(), &decryptKey, iv.data(), 0);
    Require(inPlace == plain);
    Require(iv == Bytes(expected.end() - 16, expected.end()));
    iv = Hex("000102030405060708090a0b0c0d0e0f");
    Bytes partial(32, 0xee);
    AES_cbc_encrypt_nid_postfix(plain.data(), partial.data(), 20, &encryptKey, iv.data(), 1);
    Require(Bytes(partial.begin(), partial.begin() + 16) == Bytes(expected.begin(), expected.begin() + 16));
    Bytes padded(plain.begin() + 16, plain.begin() + 20);
    Bytes secondInput(16);
    for (std::size_t index = 0; index < 16; ++index) secondInput[index] = (index < 4 ? padded[index] : 0) ^ expected[index];
    Block second{};
    AES_encrypt_nid_postfix(secondInput.data(), second.data(), &encryptKey);
    Require(Bytes(partial.begin() + 16, partial.end()) == Bytes(second.begin(), second.end()));
    Require(iv == Bytes(second.begin(), second.end()));
    iv = Hex("000102030405060708090a0b0c0d0e0f");
    Bytes decrypted(32, 0xee);
    AES_cbc_encrypt_nid_postfix(partial.data(), decrypted.data(), 20, &decryptKey, iv.data(), 0);
    Require(Bytes(decrypted.begin(), decrypted.begin() + 20) == Bytes(plain.begin(), plain.begin() + 20));
    Require(decrypted[20] == 0xee && decrypted[31] == 0xee);
    Require(iv == Bytes(second.begin(), second.end()));
    iv = Hex("000102030405060708090a0b0c0d0e0f");
    AES_cbc_encrypt_nid_postfix(nullptr, nullptr, 0, &encryptKey, iv.data(), 1);
    Require(iv == Hex("000102030405060708090a0b0c0d0e0f"));
}

static void CheckCfb() {
    const auto plain = Hex(NistPlain);
    const auto expected = Hex(
        "3b3fd92eb72dad20333449f8e83cfb4ac8a64537a0b3a93fcde3cdad9f1ce58b26751f67a3cbb140b1808cf187a4f4dfc04b05357c5d1c0eeac4c66f9ff7f2e6");
    const auto key = EncryptKey(Hex(NistKey));
    auto iv = Hex("000102030405060708090a0b0c0d0e0f");
    int num = 0;
    Bytes cipher(plain.size());
    AES_cfb128_encrypt_nid_postfix(plain.data(), cipher.data(), 5, &key, iv.data(), &num, 1);
    Require(num == 5);
    AES_cfb128_encrypt_nid_postfix(plain.data() + 5, cipher.data() + 5, 30, &key, iv.data(), &num, 1);
    Require(num == 3);
    AES_cfb128_encrypt_nid_postfix(plain.data() + 35, cipher.data() + 35, 29, &key, iv.data(), &num, 1);
    Require(num == 0);
    Require(cipher == expected);
    iv = Hex("000102030405060708090a0b0c0d0e0f");
    num = 0;
    auto inPlace = expected;
    AES_cfb128_encrypt_nid_postfix(inPlace.data(), inPlace.data(), 17, &key, iv.data(), &num, 0);
    AES_cfb128_encrypt_nid_postfix(inPlace.data() + 17, inPlace.data() + 17, inPlace.size() - 17, &key, iv.data(), &num, 0);
    Require(inPlace == plain);
    Require(num == 0);
    num = 16;
    RequireThrows<std::runtime_error>([&] { AES_cfb128_encrypt_nid_postfix(plain.data(), cipher.data(), 1, &key, iv.data(), &num, 1); });
    num = -1;
    RequireThrows<std::runtime_error>([&] { AES_cfb128_encrypt_nid_postfix(plain.data(), cipher.data(), 1, &key, iv.data(), &num, 1); });
}

static void CheckCtr() {
    const auto plain = Hex(NistPlain);
    const auto expected = Hex(
        "874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee");
    const auto key = EncryptKey(Hex(NistKey));
    auto counter = Hex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
    Block ecount{};
    unsigned int num = 0;
    Bytes cipher(plain.size());
    AES_ctr128_encrypt_nid_postfix(plain.data(), cipher.data(), 7, &key, counter.data(), ecount.data(), &num);
    Require(num == 7);
    AES_ctr128_encrypt_nid_postfix(plain.data() + 7, cipher.data() + 7, plain.size() - 7, &key, counter.data(), ecount.data(), &num);
    Require(num == 0);
    Require(cipher == expected);
    Require(counter == Hex("f0f1f2f3f4f5f6f7f8f9fafbfcfdff03"));
    auto wrap = Hex("ffffffffffffffffffffffffffffffff");
    num = 0;
    Bytes one(1);
    AES_ctr128_encrypt_nid_postfix(plain.data(), one.data(), 1, &key, wrap.data(), ecount.data(), &num);
    Require(wrap == Bytes(16, 0));
    num = 16;
    RequireThrows<std::runtime_error>([&] { AES_ctr128_encrypt_nid_postfix(plain.data(), one.data(), 1, &key, counter.data(), ecount.data(), &num); });
}

static void CheckErrors() {
    AesKey schedule{};
    const auto key = Hex(NistKey);
    Require(AES_set_encrypt_key_nid_postfix(nullptr, 128, &schedule) == -1);
    Require(AES_set_encrypt_key_nid_postfix(key.data(), 128, nullptr) == -1);
    Require(AES_set_encrypt_key_nid_postfix(key.data(), 64, &schedule) == -2);
    Require(AES_set_decrypt_key_nid_postfix(nullptr, 128, &schedule) == -1);
    Require(AES_set_decrypt_key_nid_postfix(key.data(), 512, &schedule) == -2);
    Block block{};
    AesKey invalid{};
    invalid.rounds = 11;
    RequireThrows<std::runtime_error>([&] { AES_encrypt_nid_postfix(block.data(), block.data(), &invalid); });
    RequireThrows<std::runtime_error>([&] { AES_decrypt_nid_postfix(block.data(), block.data(), &invalid); });
    const auto valid = EncryptKey(key);
    RequireThrows<std::invalid_argument>([&] { AES_encrypt_nid_postfix(nullptr, block.data(), &valid); });
    RequireThrows<std::invalid_argument>([&] { AES_decrypt_nid_postfix(block.data(), block.data(), nullptr); });
    RequireThrows<std::invalid_argument>([&] { AES_cbc_encrypt_nid_postfix(block.data(), block.data(), 16, &valid, nullptr, 1); });
    RequireThrows<std::invalid_argument>([&] { AES_cbc_encrypt_nid_postfix(nullptr, block.data(), 16, &valid, block.data(), 1); });
}

int main() {
    CheckBlock("000102030405060708090a0b0c0d0e0f", "00112233445566778899aabbccddeeff", "69c4e0d86a7b0430d8cdb78070b4c55a");
    CheckBlock("000102030405060708090a0b0c0d0e0f1011121314151617", "00112233445566778899aabbccddeeff", "dda97ca4864cdfe06eaf70a0ec0d7191");
    CheckBlock("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", "00112233445566778899aabbccddeeff", "8ea2b7ca516745bfeafc49904b496089");
    CheckBlock(NistKey, "3243f6a8885a308d313198a2e0370734", "3925841d02dc09fbdc118597196a0b32");
    CheckSchedule();
    CheckCbc();
    CheckCfb();
    CheckCtr();
    CheckErrors();
    return 0;
}
