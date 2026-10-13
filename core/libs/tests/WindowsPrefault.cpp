#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/DirectMemory/DirectMemory.hpp"
#include <windows.h>
#include <psapi.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <source_location>
#include <thread>
#include <vector>

extern "C" {
int APS5_VABI sceKernelAllocateDirectMemory(std::int64_t, std::int64_t, std::size_t, std::size_t, int, std::int64_t*);
int APS5_VABI sceKernelMapDirectMemory(void**, std::size_t, int, int, std::int64_t, std::size_t);
int APS5_VABI sceKernelEnableDmemAliasing();
int APS5_VABI sceKernelMunmap(void*, std::size_t);
int APS5_VABI sceKernelReleaseDirectMemory(std::int64_t, std::size_t);
}

static void Require(bool value, std::source_location location = std::source_location::current()) {
    if (!value) {
        std::fprintf(stderr, "Prefault check failed at %s:%u\n", location.file_name(), location.line());
        std::abort();
    }
}

static bool Resident(void* address, std::size_t length) {
    std::vector<PSAPI_WORKING_SET_EX_INFORMATION> pages(length / 4096);
    for (std::size_t i = 0; i < pages.size(); ++i) pages[i].VirtualAddress = static_cast<char*>(address) + i * 4096;
    Require(QueryWorkingSetEx(GetCurrentProcess(), pages.data(), static_cast<DWORD>(pages.size() * sizeof(pages[0]))));
    for (const auto& page : pages) if (!page.VirtualAttributes.Valid) return false;
    return true;
}

static void AwaitResident(void* address, std::size_t length) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!Resident(address, length) && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    Require(Resident(address, length));
}

int main(int argc, char**) {
    const bool enabled = argc > 1;
    Require(_putenv_s("APS5_PREFAULT_MEMORY", enabled ? "1" : "") == 0);
    constexpr std::size_t chunk = 2u << 20;
    constexpr std::size_t length = chunk + 0x4000;
    std::int64_t physical = 0;
    Require(sceKernelAllocateDirectMemory(0, 0x7fffffffffll, length * 3, 0, 0, &physical) == 0);
    void* small = nullptr;
    void* inaccessible = nullptr;
    void* memory = nullptr;
    Require(sceKernelMapDirectMemory(&small, 0x4000, 3, 0, physical, 0) == 0);
    Require(sceKernelMapDirectMemory(&inaccessible, length, 0, 0, physical + length, 0) == 0);
    Require(sceKernelMapDirectMemory(&memory, length, 3, 0, physical + length * 2, 0) == 0);
    if (enabled) AwaitResident(memory, length);
    else {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        Require(!Resident(memory, length));
    }
    Require(!Resident(small, 0x4000));
    Require(!Resident(inaccessible, length));
    MEMORY_BASIC_INFORMATION info{};
    Require(VirtualQuery(inaccessible, &info, sizeof(info)) == sizeof(info));
    Require(info.Protect == PAGE_NOACCESS);
    auto* bytes = static_cast<unsigned char*>(memory);
    for (std::size_t i = 0; i < length; ++i) Require(bytes[i] == 0);
    bytes[0] = 17;
    bytes[chunk - 1] = 33;
    bytes[length - 1] = 65;
    Require(sceKernelEnableDmemAliasing() == 0);
    void* alias = nullptr;
    Require(sceKernelMapDirectMemory(&alias, length, 1, 0, physical + length * 2, 0) == 0);
    if (enabled) AwaitResident(alias, length);
    Require(VirtualQuery(alias, &info, sizeof(info)) == sizeof(info));
    Require(info.Protect == PAGE_READONLY);
    const auto* other = static_cast<const unsigned char*>(alias);
    for (std::size_t i = 0; i < length; ++i) Require(other[i] == bytes[i]);
    Require(sceKernelMunmap(alias, length) == 0);
    for (int i = 0; i < 16; ++i) {
        void* transient = nullptr;
        Require(sceKernelMapDirectMemory(&transient, length, 3, 0, physical + length * 2, 0) == 0);
        Require(sceKernelMunmap(transient, length) == 0);
    }
    void* finalAlias = nullptr;
    Require(sceKernelMapDirectMemory(&finalAlias, length, 1, 0, physical + length * 2, 0) == 0);
    if (enabled) AwaitResident(finalAlias, length);
    Require(static_cast<const unsigned char*>(finalAlias)[length - 1] == 65);
    Require(sceKernelMunmap(finalAlias, length) == 0);
    Require(sceKernelMunmap(memory, length) == 0);
    Require(sceKernelMunmap(inaccessible, length) == 0);
    Require(sceKernelMunmap(small, 0x4000) == 0);
    Require(sceKernelReleaseDirectMemory(physical, length * 3) == 0);
    std::puts(enabled ? "Prefault enabled checks passed" : "Prefault disabled checks passed");
}