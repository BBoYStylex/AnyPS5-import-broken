#include "prx/libc/include/general/VabiMacros.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
int APS5_VABI AES_set_encrypt_key_nid_postfix(const unsigned char*, int, void*);
int APS5_VABI AES_set_decrypt_key_nid_postfix(const unsigned char*, int, void*);
void APS5_VABI AES_encrypt_nid_postfix(const unsigned char*, unsigned char*, const void*);
void APS5_VABI AES_decrypt_nid_postfix(const unsigned char*, unsigned char*, const void*);
void APS5_VABI AES_ecb_encrypt_nid_postfix(const unsigned char*, unsigned char*, const void*, int);
void APS5_VABI AES_cbc_encrypt_nid_postfix(const unsigned char*, unsigned char*, std::size_t, const void*, unsigned char*, int);
void APS5_VABI AES_cfb128_encrypt_nid_postfix(const unsigned char*, unsigned char*, std::size_t, const void*, unsigned char*, int*, int);
void APS5_VABI AES_ctr128_encrypt_nid_postfix(const unsigned char*, unsigned char*, std::size_t, const void*, unsigned char*, unsigned char*, unsigned int*);
}

namespace {
constexpr std::size_t KeySize = 244;

struct Vector {
    int bits;
    const char* ecb;
    const char* cbc;
    const char* cfb;
    const char* ctr;
};

constexpr Vector Vectors[] {
    {128, "69c4e0d86a7b0430d8cdb78070b4c55a", "43b9c056cc9c709a426c2b4267f87ffd2c7125f02eae1dcf959cf1f87dca8f3dd9fdcfc6c21e1bc000d5238fd5349cc6", "02ecf058d893500efd7469852f3a295bef378c5993824bec6a66be1f848b92f04640256e131f5266ec37c0095e9ec42356fcb8e91a8b961d0de83ae4f35fe89c3317bf664d69", "33d9f78b20899b6c1cc80803061306cdec4500f6034e71988298544735c11f8ab6aa25a63d4df4ae7db98d8971f0c2b6fa16a6d8e53988e544a8e531a40ef405d106195865e9"},
    {192, "dda97ca4864cdfe06eaf70a0ec0d7191", "076d3f4cdd2735aa063ac37144a56f5c5fda1177323172dcb02b225f2e5f131208a130a108e161791aeb0218207a5341", "96bc2453a684bb75f07f213d544cf4e5aa1b34a35e817f918e6fef2775f70bf8b68d48f15fe6bbb71c03df5cb1c65377e56da80332953b4db445265376147e3ee69c03165cb3", "7ba55e3271c5ea29894081e7e849222f477dde63b20fd6cdd016404fc8ba3725f886c88771dafb3840764de49cb1b7ce61bd97405702340541699fee4583a067defe18cafa2f"},
    {256, "8ea2b7ca516745bfeafc49904b496089", "8f01886432ea03dc8854560b9e30de00cd5da46299b12f132ca7e363235e5273a40d70f47f17de6324bc3ccd73829700", "ecd1f0a68b7200869d0e1b4297491346e7ffd1576ce1a49b2322179f118a91a00485ef750310b4a9aa5ecdce4674b6f19950af36e238f6409b67b8faf0b7098bf111227c4dc4", "e8af8907e0c1ef9d9b6751a809832971731934a0f4f65e2b65b7614506c27447f4af6a23438ed87da985b6fc2d0121ab3547b3733b59eeb4e2897c8633c535bd74593b11481b"},
};

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

std::vector<unsigned char> Sequence(std::size_t count, unsigned multiplier, unsigned offset) {
    std::vector<unsigned char> bytes(count);
    for (std::size_t i = 0; i < count; ++i) bytes[i] = static_cast<unsigned char>((i * multiplier + offset) % 256);
    return bytes;
}
}

int main() {
    const auto block = FromHex("00112233445566778899aabbccddeeff");
    const auto data48 = Sequence(48, 7, 3);
    const auto data70 = Sequence(70, 13, 5);
    std::vector<unsigned char> iv(16), cfbIv(16), ctrIv(16, 0xff);
    for (int i = 0; i < 16; ++i) {
        iv[i] = static_cast<unsigned char>(0xa0 + i);
        cfbIv[i] = static_cast<unsigned char>(0x10 + i);
    }
    std::fill(ctrIv.begin(), ctrIv.begin() + 8, 0);
    ctrIv[15] = 0xfe;

    for (const auto& vector : Vectors) {
        const auto key = Sequence(vector.bits / 8, 1, 0);
        unsigned char encKey[KeySize], decKey[KeySize];
        Require(AES_set_encrypt_key_nid_postfix(key.data(), vector.bits, encKey) == 0, "encrypt key");
        Require(AES_set_decrypt_key_nid_postfix(key.data(), vector.bits, decKey) == 0, "decrypt key");

        unsigned char out[16], back[16];
        AES_encrypt_nid_postfix(block.data(), out, encKey);
        Require(std::memcmp(out, FromHex(vector.ecb).data(), 16) == 0, "AES_encrypt known answer");
        AES_decrypt_nid_postfix(out, back, decKey);
        Require(std::memcmp(back, block.data(), 16) == 0, "AES_decrypt round trip");
        AES_ecb_encrypt_nid_postfix(block.data(), out, encKey, 1);
        Require(std::memcmp(out, FromHex(vector.ecb).data(), 16) == 0, "AES_ecb_encrypt encrypt");
        AES_ecb_encrypt_nid_postfix(out, back, decKey, 0);
        Require(std::memcmp(back, block.data(), 16) == 0, "AES_ecb_encrypt decrypt");

        std::vector<unsigned char> cipher(48), plain(48), chain = iv;
        AES_cbc_encrypt_nid_postfix(data48.data(), cipher.data(), 48, encKey, chain.data(), 1);
        Require(cipher == FromHex(vector.cbc), "AES_cbc_encrypt known answer");
        Require(std::memcmp(chain.data(), cipher.data() + 32, 16) == 0, "AES_cbc_encrypt updates the iv");
        chain = iv;
        AES_cbc_encrypt_nid_postfix(cipher.data(), plain.data(), 48, decKey, chain.data(), 0);
        Require(plain == data48, "AES_cbc_encrypt decrypt");
        chain = iv;
        std::vector<unsigned char> inPlace = data48;
        AES_cbc_encrypt_nid_postfix(inPlace.data(), inPlace.data(), 48, encKey, chain.data(), 1);
        Require(inPlace == cipher, "AES_cbc_encrypt in place");

        const std::size_t chunks[] {3, 29, 1, 37};
        std::vector<unsigned char> stream(70), recovered(70);
        std::vector<unsigned char> state = cfbIv;
        int num = 0;
        std::size_t offset = 0;
        for (const auto chunk : chunks) {
            AES_cfb128_encrypt_nid_postfix(data70.data() + offset, stream.data() + offset, chunk, encKey, state.data(), &num, 1);
            offset += chunk;
        }
        Require(stream == FromHex(vector.cfb), "AES_cfb128_encrypt known answer");
        Require(num == 70 % 16, "AES_cfb128_encrypt num");
        state = cfbIv;
        num = 0;
        offset = 0;
        for (const auto chunk : chunks) {
            AES_cfb128_encrypt_nid_postfix(stream.data() + offset, recovered.data() + offset, chunk, encKey, state.data(), &num, 0);
            offset += chunk;
        }
        Require(recovered == data70, "AES_cfb128_encrypt decrypt");

        std::vector<unsigned char> counter = ctrIv;
        unsigned char keystream[16] {};
        unsigned int ctrNum = 0;
        offset = 0;
        for (const auto chunk : chunks) {
            AES_ctr128_encrypt_nid_postfix(data70.data() + offset, stream.data() + offset, chunk, encKey, counter.data(), keystream, &ctrNum);
            offset += chunk;
        }
        Require(stream == FromHex(vector.ctr), "AES_ctr128_encrypt known answer");
        Require(ctrNum == 70 % 16, "AES_ctr128_encrypt num");

        RequireThrows([&] { AES_encrypt_nid_postfix(nullptr, out, encKey); }, "null input");
        RequireThrows([&] { AES_encrypt_nid_postfix(block.data(), nullptr, encKey); }, "null output");
        RequireThrows([&] { AES_encrypt_nid_postfix(block.data(), out, nullptr); }, "null key");
        RequireThrows([&] { AES_ecb_encrypt_nid_postfix(block.data(), out, encKey, 2); }, "unknown enc value");
        RequireThrows([&] { AES_cbc_encrypt_nid_postfix(data48.data(), cipher.data(), 40, encKey, chain.data(), 1); }, "partial block");
        RequireThrows([&] { AES_cbc_encrypt_nid_postfix(data48.data(), cipher.data(), 48, encKey, nullptr, 1); }, "null iv");
        int badNum = 16;
        RequireThrows([&] { AES_cfb128_encrypt_nid_postfix(data70.data(), stream.data(), 1, encKey, state.data(), &badNum, 1); }, "cfb num out of range");
        unsigned int badCtrNum = 16;
        RequireThrows([&] { AES_ctr128_encrypt_nid_postfix(data70.data(), stream.data(), 1, encKey, counter.data(), keystream, &badCtrNum); }, "ctr num out of range");
    }

    unsigned char zeroKey[KeySize] {};
    unsigned char out[16];
    RequireThrows([&] { AES_encrypt_nid_postfix(block.data(), out, zeroKey); }, "key schedule that was never set");
    unsigned char material[32] {};
    Require(AES_set_encrypt_key_nid_postfix(nullptr, 128, zeroKey) == -1, "null user key");
    Require(AES_set_encrypt_key_nid_postfix(material, 128, nullptr) == -1, "null key schedule");
    Require(AES_set_decrypt_key_nid_postfix(nullptr, 128, zeroKey) == -1, "null user key for decrypt");
    Require(AES_set_encrypt_key_nid_postfix(material, 100, zeroKey) == -2, "unsupported key size");
    Require(AES_set_decrypt_key_nid_postfix(material, 512, zeroKey) == -2, "unsupported key size for decrypt");
    return 0;
}
