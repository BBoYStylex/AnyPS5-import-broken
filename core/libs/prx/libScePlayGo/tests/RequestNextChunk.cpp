#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <source_location>

extern "C" int APS5_VABI scePlayGoRequestNextChunk(int, const void*);

static void Require(bool value, const std::source_location location = std::source_location::current()) {
    if (value) return;
    std::fprintf(stderr, "requirement failed at line %u\n", location.line());
    std::abort();
}

int main() {
    constexpr int badHandle = static_cast<int>(0x80B20009);
    constexpr int invalidArgument = static_cast<int>(0x80B20004);
    int reserved = 0;
    Require(scePlayGoRequestNextChunk(2, nullptr) == badHandle);
    Require(scePlayGoRequestNextChunk(1, &reserved) == invalidArgument);
    Require(scePlayGoRequestNextChunk(1, nullptr) == 0);
    return 0;
}
