#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" {
std::int32_t APS5_VABI sceDiscMapIsRequestOnHDD(const char* path, std::int64_t offset, std::int64_t size, std::int32_t* result);
std::int32_t APS5_VABI sceDiscMapUnknown_ioKMruft1ek(const char* path, std::int64_t offset, std::int64_t size, void* flags, void* out1, void* out2);
std::int32_t APS5_VABI sceDiscMapGetPackageSize(std::int64_t fflags, void* out1, void* out2);
}

namespace {

constexpr std::int32_t noBitmapInfo = static_cast<std::int32_t>(0x81100004);

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "DiscMap: %s\n", message);
        std::abort();
    }
}

}

int main() {
    std::int32_t result = 0x5a5a5a5a;
    Require(sceDiscMapIsRequestOnHDD("/app0/data.bin", 0, 0x10000, &result) == noBitmapInfo, "sceDiscMapIsRequestOnHDD did not report the missing bitmap");
    Require(result == 0x5a5a5a5a, "sceDiscMapIsRequestOnHDD wrote its result");
    std::uint64_t flags = ~0ull;
    std::uint64_t out1 = ~0ull;
    std::uint64_t out2 = ~0ull;
    Require(sceDiscMapUnknown_ioKMruft1ek("/app0/data.bin", 0x8000, 0x10000, &flags, &out1, &out2) == noBitmapInfo, "ioKMruft1ek did not report the missing bitmap");
    Require(flags == ~0ull && out1 == ~0ull && out2 == ~0ull, "ioKMruft1ek wrote an output");
    const std::int64_t packageFlags[] = {0, 0x1, 0x100};
    for (std::int64_t fflags : packageFlags) {
        Require(sceDiscMapGetPackageSize(fflags, &out1, &out2) == noBitmapInfo, "sceDiscMapGetPackageSize did not report the missing bitmap");
        Require(out1 == ~0ull && out2 == ~0ull, "sceDiscMapGetPackageSize wrote an output");
    }
    Require(sceDiscMapGetPackageSize(0, nullptr, nullptr) == noBitmapInfo, "sceDiscMapGetPackageSize did not report the missing bitmap for null outputs");
    std::puts("DiscMap tests passed");
    return 0;
}
