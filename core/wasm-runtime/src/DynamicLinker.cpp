#include "Program.hpp"
#include "wasm-runtime/DynamicLinker.hpp"
#include "wasm-runtime/Host.hpp"
#include <cstring>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::uint32_t LoadSegment = 1;
constexpr std::uint64_t SegmentFileOffset = 0x4000;

struct LinkedImage {
    std::string name;
    std::uint64_t base = 0;
    std::uint64_t span = 0;
    std::vector<Elf64_Phdr> phdrs;
    link_map map{};
    std::size_t tlsModule = 0;
};

std::vector<LinkedImage>& Images() {
    static std::vector<LinkedImage> images;
    static std::once_flag built;
    std::call_once(built, [] {
        const auto& program = WasmRuntime::CurrentProgram();
        images.resize(program.metadata.images.size());
        for (std::size_t index = 0; index < images.size(); ++index) {
            const auto& meta = program.metadata.images[index];
            auto& image = images[index];
            image.name = index == 0 ? std::string() : meta.name;
            image.base = program.bases[index];
            image.span = meta.span;
            image.tlsModule = meta.tls ? index + 1 : 0;
            for (const auto& phdr : meta.phdrs) {
                Elf64_Phdr header{};
                header.p_type = phdr.kind;
                header.p_flags = phdr.flags;
                header.p_vaddr = phdr.vaddr;
                header.p_paddr = phdr.vaddr;
                header.p_offset = phdr.kind == LoadSegment ? phdr.vaddr + SegmentFileOffset : phdr.vaddr;
                header.p_memsz = phdr.memsz;
                header.p_filesz = phdr.memsz;
                header.p_align = 0x4000;
                image.phdrs.push_back(header);
            }
            image.map.l_addr = image.base;
            image.map.l_name = image.name.data();
        }
        for (std::size_t index = 0; index < images.size(); ++index) {
            images[index].map.l_prev = index > 0 ? &images[index - 1].map : nullptr;
            images[index].map.l_next = index + 1 < images.size() ? &images[index + 1].map : nullptr;
        }
    });
    return images;
}

LinkedImage* ImageAt(const void* address) {
    if (!WasmRuntime::ProgramLoaded()) return nullptr;
    const auto value = reinterpret_cast<std::uintptr_t>(address);
    for (auto& image : Images()) {
        if (value >= image.base && value - image.base < image.span) return &image;
    }
    return nullptr;
}

extern "C" void __dl_seterr(const char* format, ...);

void* Fail(const std::string& message) {
    __dl_seterr("%s", message.c_str());
    return nullptr;
}

LinkedImage* ImageForPath(std::string_view path) {
    auto& images = Images();
    for (std::size_t index = 1; index < images.size(); ++index) {
        const std::string_view name = images[index].name;
        if (path == name || (path.size() > name.size() && path.ends_with(name) && path[path.size() - name.size() - 1] == '/')) return &images[index];
    }
    return nullptr;
}

}  // namespace

const char* WasmRuntime::MainImageName() {
    return CurrentProgram().metadata.images.front().name.c_str();
}

extern "C" {

int dl_iterate_phdr(int (*callback)(dl_phdr_info*, std::size_t, void*), void* data) {
    if (!WasmRuntime::ProgramLoaded()) return 0;
    for (auto& image : Images()) {
        dl_phdr_info info{};
        info.dlpi_addr = image.base;
        info.dlpi_name = image.name.c_str();
        info.dlpi_phdr = image.phdrs.data();
        info.dlpi_phnum = static_cast<Elf64_Half>(image.phdrs.size());
        info.dlpi_tls_modid = image.tlsModule;
        if (const int result = callback(&info, sizeof(info), data)) return result;
    }
    return 0;
}

int dladdr(const void* address, Dl_info* info) {
    auto* image = ImageAt(address);
    if (!image) return 0;
    info->dli_fname = image->name.c_str();
    info->dli_fbase = reinterpret_cast<void*>(static_cast<std::uintptr_t>(image->base));
    info->dli_sname = nullptr;
    info->dli_saddr = nullptr;
    return 1;
}

int dlinfo(void* handle, int request, void* result) {
    static_cast<void>(handle);
    if (request != RTLD_DI_LINKMAP || !WasmRuntime::ProgramLoaded() || Images().empty()) return -1;
    *static_cast<link_map**>(result) = &Images().front().map;
    return 0;
}

int dladdr1(const void* address, Dl_info* info, void** extra, int flags) {
    if (!dladdr(address, info)) return 0;
    if (flags == RTLD_DL_LINKMAP && extra) *extra = &ImageAt(address)->map;
    return 1;
}

void* dlopen(const char* path, int flags) {
    static_cast<void>(flags);
    if (!WasmRuntime::ProgramLoaded() || Images().empty()) return Fail("dlopen: no program is loaded");
    if (path == nullptr) return &Images().front().map;
    if (auto* image = ImageForPath(path)) {
        WasmRuntime::OpenImage(WasmRuntime::CurrentProgram(), static_cast<std::size_t>(image - Images().data()));
        return &image->map;
    }
    return Fail(std::string("dlopen: ") + path + " is not part of game.wasm");
}

void* dlsym(void* handle, const char* name) {
    if (!WasmRuntime::ProgramLoaded() || name == nullptr) return Fail("dlsym: no program is loaded");
    const auto& program = WasmRuntime::CurrentProgram();
    auto& images = Images();
    for (std::size_t index = 0; index < images.size(); ++index) {
        if (handle != RTLD_DEFAULT && handle != &images[index].map) continue;
        if (handle == RTLD_DEFAULT && !WasmRuntime::ImageOpened(WasmRuntime::CurrentProgram(), index)) continue;
        for (const auto& exported : program.metadata.images[index].exports) {
            if (exported.nid == name) return reinterpret_cast<void*>(static_cast<std::uintptr_t>(program.bases[index] + exported.vaddr));
        }
    }
    return Fail(std::string("dlsym: ") + name + " is not exported");
}

int dlclose(void* handle) {
    static_cast<void>(handle);
    return 0;
}

}
