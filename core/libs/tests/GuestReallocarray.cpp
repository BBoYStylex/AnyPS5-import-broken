#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>

extern "C" {
void* APS5_VABI reallocarray_nid_postfix(void*, std::size_t, std::size_t);
void APS5_VABI free_nid_postfix(void*);
int* APS5_VABI __error_nid_postfix();
}

namespace {

constexpr int NoMemory = 12;

void Require(bool value) {
    if (!value) throw std::runtime_error("reallocarray check failed");
}

}

int main() {
    void* api[10]{};
    ApplicationHeapRegister_nid_no_patch(api);

    *__error_nid_postfix() = 77;
    auto* block = static_cast<unsigned char*>(reallocarray_nid_postfix(nullptr, 4, 16));
    Require(block != nullptr);
    Require(*__error_nid_postfix() == 77);
    std::memset(block, 0x5a, 64);

    *__error_nid_postfix() = 77;
    auto* grown = static_cast<unsigned char*>(reallocarray_nid_postfix(block, 8, 16));
    Require(grown != nullptr);
    Require(*__error_nid_postfix() == 77);
    for (int index = 0; index < 64; ++index) Require(grown[index] == 0x5a);

    *__error_nid_postfix() = 0;
    Require(reallocarray_nid_postfix(grown, std::numeric_limits<std::size_t>::max(), 2) == nullptr);
    Require(*__error_nid_postfix() == NoMemory);
    for (int index = 0; index < 64; ++index) Require(grown[index] == 0x5a);

    *__error_nid_postfix() = 0;
    Require(reallocarray_nid_postfix(nullptr, std::numeric_limits<std::size_t>::max(), 2) == nullptr);
    Require(*__error_nid_postfix() == NoMemory);

    free_nid_postfix(grown);
}
