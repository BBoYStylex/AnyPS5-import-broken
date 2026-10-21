#include "prx/libSceAmpr/include/AmprDataObjects.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <link.h>
#endif

static_assert(AmprDataObjects::FacetIdBytes == 8, "the facet id holds one size_t");
static_assert(AmprDataObjects::NumGetVtableSlots == 15, "the num_get vtable slot count");
static_assert(AmprDataObjects::NumGetVtableBytes == 120, "the num_get vtable size");

namespace {

constexpr std::size_t PathCapacity = 512;

void Require(bool value, const char* what) {
    if (value) return;
    std::fprintf(stderr, "ampr data objects: %s\n", what);
    std::abort();
}

void* Load(const char* directory, const char* name) {
    char path[PathCapacity];
    std::size_t length = std::strlen(directory);
    const std::size_t nameLength = std::strlen(name);
    if (length + nameLength + 2u >= PathCapacity) return nullptr;
    std::memcpy(path, directory, length);
    if (length == 0 || (directory[length - 1] != '/' && directory[length - 1] != '\\')) path[length++] = '/';
    std::memcpy(path + length, name, nameLength + 1u);
#ifdef _WIN32
    wchar_t wide[PathCapacity];
    const int count = MultiByteToWideChar(CP_UTF8, 0, path, -1, wide, PathCapacity);
    if (count <= 0) return nullptr;
    return reinterpret_cast<void*>(LoadLibraryExW(wide, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH));
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

void* Resolve(void* library, const char* name) {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(library), name));
#else
    return dlsym(library, name);
#endif
}

#ifdef _WIN32
bool IsExecutable(const void* address) {
    MEMORY_BASIC_INFORMATION information{};
    if (VirtualQuery(address, &information, sizeof(information)) != sizeof(information)) return false;
    const auto protection = information.Protect & 0xffu;
    return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
           protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
}
#else
struct SegmentQuery {
    std::uintptr_t address;
    bool executable;
};

bool IsExecutable(const void* address) {
    SegmentQuery query{reinterpret_cast<std::uintptr_t>(address), false};
    dl_iterate_phdr([](dl_phdr_info* image, std::size_t, void* data) {
        auto* wanted = static_cast<SegmentQuery*>(data);
        for (std::uint16_t index = 0; index < image->dlpi_phnum; ++index) {
            const auto& header = image->dlpi_phdr[index];
            if (header.p_type != PT_LOAD || (header.p_flags & PF_X) == 0) continue;
            const auto start = image->dlpi_addr + header.p_vaddr;
            if (wanted->address >= start && wanted->address < start + header.p_memsz) {
                wanted->executable = true;
                return 1;
            }
        }
        return 0;
    }, &query);
    return query.executable;
}
#endif

void CheckDataObject(void* library, const char* nid, std::size_t size, const char* what) {
    const auto address = Resolve(library, nid);
    Require(address != nullptr, what);
    Require(!IsExecutable(address), what);
    const auto* bytes = static_cast<const unsigned char*>(address);
    for (std::size_t index = 0; index < size; ++index) {
        if (bytes[index] != 0) {
            Require(false, what);
            return;
        }
    }
}

}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s <patched libraries directory>\n", argv[0]);
        return 2;
    }
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS);
#endif
    Load(argv[1], "libc.prx");
    Load(argv[1], "libkernel.prx");
    auto* library = Load(argv[1], "libSceAmpr.prx");
    if (library == nullptr) {
#ifdef _WIN32
        std::fprintf(stderr, "ampr data objects: the patched libSceAmpr.prx did not load, error %lu\n", GetLastError());
#else
        std::fprintf(stderr, "ampr data objects: the patched libSceAmpr.prx did not load: %s\n", dlerror());
#endif
        return 1;
    }
    CheckDataObject(library, "-mLzBSk-VGs", AmprDataObjects::FacetIdBytes, "the num_get facet id is not a data object");
    CheckDataObject(library, "a54t8+k7KpY", AmprDataObjects::FacetIdBytes, "the time_get facet id is not a data object");
    CheckDataObject(library, "KfcTPbeaOqg", AmprDataObjects::NumGetVtableBytes, "the num_get vtable is not a data object");
    return 0;
}
