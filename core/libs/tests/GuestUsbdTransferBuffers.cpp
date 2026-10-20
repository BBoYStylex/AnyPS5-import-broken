#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

extern "C" {
void* APS5_VABI sceUsbdAllocTransfer(std::int32_t);
void APS5_VABI sceUsbdFreeTransfer(void*);
void APS5_VABI sceUsbdFillInterruptTransfer(void*, void*, std::uint8_t, std::uint8_t*, std::int32_t, void*, void*, std::uint32_t);
void APS5_VABI sceUsbdFillControlTransfer(void*, void*, std::uint8_t*, void*, void*, std::uint32_t);
}

namespace {

alignas(64) std::array<std::uint8_t, 128> storage{};
bool custom = true;
bool live = false;
std::size_t activeSize = 0;
int freeCalls = 0;

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "USBD transfer buffer check failed at line %d\n", line);
        std::abort();
    }
}

#define Require(value) Check((value), __LINE__)

void* APS5_VABI Allocate(std::size_t size) {
    Require(!live && size <= storage.size());
    live = true;
    activeSize = size;
    return storage.data();
}

void APS5_VABI Free(void* pointer) {
    Require(live && pointer == storage.data());
    live = false;
    activeSize = 0;
    ++freeCalls;
}

void* APS5_VABI Calloc(std::size_t count, std::size_t size) {
    Require(size == 0 || count <= storage.size() / size);
    return std::memset(Allocate(count * size), 0, count * size);
}

void* APS5_VABI Reallocate(void* pointer, std::size_t size) {
    Require(pointer == storage.data() && live && size <= storage.size());
    activeSize = size;
    return pointer;
}

void* APS5_VABI Align(std::size_t alignment, std::size_t size) {
    Require(alignment != 0 && alignment <= 64 && (alignment & (alignment - 1)) == 0);
    return Allocate(size);
}

void* APS5_VABI Realign(void* pointer, std::size_t size, std::size_t alignment) {
    Require(alignment != 0 && alignment <= 64 && (alignment & (alignment - 1)) == 0);
    return Reallocate(pointer, size);
}

int APS5_VABI PosixAlign(void** pointer, std::size_t alignment, std::size_t size) {
    *pointer = Align(alignment, size);
    return 0;
}

std::uint64_t InUse() {
    if (custom) return activeSize;
    std::array<std::byte, 0x28> stats{};
    const std::uint16_t size = 0x28;
    const std::uint16_t version = 1;
    std::memcpy(stats.data(), &size, sizeof(size));
    std::memcpy(stats.data() + 2, &version, sizeof(version));
    Require(ApplicationHeapStatsFast_nid_no_patch(stats.data()) == 0);
    std::uint64_t bytes = 0;
    std::memcpy(&bytes, stats.data() + 0x20, sizeof(bytes));
    return bytes;
}

}

int main(int argc, char** argv) {
    custom = argc == 1 || std::strcmp(argv[1], "default") != 0;
    std::array<void*, 10> api{};
    if (custom) {
        api[0] = reinterpret_cast<void*>(Allocate);
        api[1] = reinterpret_cast<void*>(Free);
        api[2] = reinterpret_cast<void*>(Calloc);
        api[3] = reinterpret_cast<void*>(Reallocate);
        api[4] = reinterpret_cast<void*>(Align);
        api[5] = reinterpret_cast<void*>(Realign);
        api[6] = reinterpret_cast<void*>(PosixAlign);
    }
    ApplicationHeapRegister_nid_no_patch(api.data());
    auto* warmup = ApplicationHeapAllocate_nid_no_patch(1);
    ApplicationHeapFree_nid_no_patch(warmup);
    const auto baseline = InUse();
    const auto initialFreeCalls = freeCalls;
    for (const std::uint8_t flags : {std::uint8_t{2}, std::uint8_t{3}, std::uint8_t{10}, std::uint8_t{6}}) {
        auto* transfer = static_cast<std::uint8_t*>(sceUsbdAllocTransfer(0));
        Require(transfer != nullptr);
        auto* buffer = static_cast<std::uint8_t*>(ApplicationHeapAllocate_nid_no_patch(64));
        Require(InUse() >= baseline + 64);
        std::memset(buffer, 0x5A, 64);
        sceUsbdFillInterruptTransfer(transfer, nullptr, 0x81, buffer, 64, nullptr, nullptr, 0);
        transfer[8] = flags;
        sceUsbdFreeTransfer(transfer);
        Require(InUse() == baseline);
    }
    auto* borrowed = static_cast<std::uint8_t*>(ApplicationHeapAllocate_nid_no_patch(64));
    borrowed[0] = 0x5A;
    auto* transfer = static_cast<std::uint8_t*>(sceUsbdAllocTransfer(0));
    Require(transfer != nullptr);
    sceUsbdFillInterruptTransfer(transfer, nullptr, 0x81, borrowed, 64, nullptr, nullptr, 0);
    sceUsbdFreeTransfer(transfer);
    Require(InUse() >= baseline + 64 && borrowed[0] == 0x5A);
    ApplicationHeapFree_nid_no_patch(borrowed);
    Require(InUse() == baseline);

    auto* control = static_cast<std::uint8_t*>(ApplicationHeapAllocate_nid_no_patch(64));
    std::memset(control, 0, 64);
    control[6] = 16;
    transfer = static_cast<std::uint8_t*>(sceUsbdAllocTransfer(0));
    Require(transfer != nullptr);
    sceUsbdFillControlTransfer(transfer, nullptr, control, nullptr, nullptr, 0);
    transfer[8] = 2;
    sceUsbdFreeTransfer(transfer);
    Require(InUse() == baseline);
    transfer = static_cast<std::uint8_t*>(sceUsbdAllocTransfer(0));
    Require(transfer != nullptr);
    transfer[8] = 2;
    sceUsbdFreeTransfer(transfer);
    sceUsbdFreeTransfer(nullptr);
    Require(InUse() == baseline);
    if (custom) Require(freeCalls == initialFreeCalls + 6);
}
