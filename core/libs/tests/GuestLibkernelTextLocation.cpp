#include "SceTypes.hpp"
#include "prx/libkernel/KernelErrors.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string>
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <mach-o/loader.h>
#else
#include <fstream>
#endif
extern "C" {
int APS5_VABI sceKernelGetLibkernelTextLocation(std::uint64_t*, std::uint64_t*);
}
static void Require(bool value) { if (!value) std::abort(); }
// Re-derives libkernel's own executable range with host APIs so the test
// checks the reported range against the loaded image, not against itself.
static void HostTextRange(std::uint64_t& base, std::uint64_t& size) {
#ifdef _WIN32
    const auto module = reinterpret_cast<std::uint64_t>(GetModuleHandleA("libkernel.prx"));
    Require(module != 0);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
    Require(dos->e_magic == IMAGE_DOS_SIGNATURE);
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(module + dos->e_lfanew);
    Require(nt->Signature == IMAGE_NT_SIGNATURE);
    const auto* sections = IMAGE_FIRST_SECTION(nt);
    base = 0;
    std::uint64_t end = 0;
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if ((sections[i].Characteristics & IMAGE_SCN_CNT_CODE) == 0) continue;
        const std::uint64_t start = module + sections[i].VirtualAddress;
        const std::uint64_t sectionSize = sections[i].Misc.VirtualSize != 0 ? sections[i].Misc.VirtualSize : sections[i].SizeOfRawData;
        if (base == 0 || start < base) base = start;
        if (start + sectionSize > end) end = start + sectionSize;
    }
    Require(base != 0 && end > base);
    size = end - base;
#elif defined(__APPLE__)
    base = 0;
    size = 0;
    for (std::uint32_t i = 0; i < _dyld_image_count(); ++i) {
        const char* name = _dyld_get_image_name(i);
        if (name == nullptr || std::strstr(name, "libkernel.prx") == nullptr) continue;
        const auto* header = _dyld_get_image_header(i);
        Require(header != nullptr && header->magic == MH_MAGIC_64);
        unsigned long textSize = 0;
        const auto* text = getsectiondata(reinterpret_cast<const mach_header_64*>(header), "__TEXT", "__text", &textSize);
        Require(text != nullptr && textSize != 0);
        base = reinterpret_cast<std::uint64_t>(text);
        size = textSize;
        return;
    }
    Require(false);
#else
    std::ifstream maps("/proc/self/maps");
    Require(static_cast<bool>(maps));
    std::string line;
    base = 0;
    size = 0;
    while (std::getline(maps, line)) {
        std::uint64_t start = 0;
        std::uint64_t end = 0;
        char perms[8] = {};
        char path[4096] = {};
        const int parsed = std::sscanf(line.c_str(), "%llx-%llx %7s %*llx %*x:%*x %*llu %4095s",
            (unsigned long long*)&start, (unsigned long long*)&end, perms, path);
        if (parsed < 4 || std::strstr(perms, "x") == nullptr || std::strstr(path, "libkernel.prx") == nullptr) continue;
        if (base == 0 || start < base) base = start;
        if (end > base + size) size = end - base;
    }
    Require(base != 0 && size != 0);
#endif
}
int main() {
    std::uint64_t address = 0;
    std::uint64_t size = 0;
    Require(sceKernelGetLibkernelTextLocation(nullptr, &size) == SCE_KERNEL_ERROR_EFAULT);
    Require(sceKernelGetLibkernelTextLocation(&address, nullptr) == SCE_KERNEL_ERROR_EFAULT);
    Require(sceKernelGetLibkernelTextLocation(nullptr, nullptr) == SCE_KERNEL_ERROR_EFAULT);
    Require(address == 0);
    Require(sceKernelGetLibkernelTextLocation(&address, &size) == 0);
    Require(address != 0 && size != 0);
    std::uint64_t hostBase = 0;
    std::uint64_t hostSize = 0;
    HostTextRange(hostBase, hostSize);
    Require(address == hostBase && size == hostSize);
    std::uint64_t again = 0;
    std::uint64_t againSize = 0;
    Require(sceKernelGetLibkernelTextLocation(&again, &againSize) == 0);
    Require(again == address && againSize == size);
    return 0;
}
