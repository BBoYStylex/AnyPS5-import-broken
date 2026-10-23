#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include <nid/NidCompute.hpp>
#include <array>
#include <thread>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <source_location>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winioctl.h>
#else
#include <dlfcn.h>
#endif

static void Require(bool value, std::source_location location = std::source_location::current()) {
    if (!value) {
        std::fprintf(stderr, "Kernel sys API check failed at line %u\n", location.line());
        std::abort();
    }
}

static void* LoadLibraryFile(const std::filesystem::path& path) {
#ifdef _WIN32
    auto* module = reinterpret_cast<void*>(LoadLibraryExW(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH));
#else
    auto* module = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!module) std::fprintf(stderr, "%s\n", dlerror());
#endif
    Require(module != nullptr);
#ifdef _WIN32
    std::wstring loaded(32768, L'\0');
    const auto length = GetModuleFileNameW(static_cast<HMODULE>(module), loaded.data(), static_cast<DWORD>(loaded.size()));
    Require(length != 0 && length < loaded.size());
    loaded.resize(length);
    Require(std::filesystem::equivalent(path, std::filesystem::path(loaded)));
#endif
    return module;
}

template <typename Function>
static Function LoadFunction(void* module, const char* name, bool patched, const char* unpatched = nullptr) {
    const auto symbol = patched ? Nid::ComputeNid(name, "") : unpatched ? std::string(unpatched) : std::string(name) + "_nid_postfix";
#ifdef _WIN32
    const auto function = reinterpret_cast<Function>(reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(module), symbol.c_str())));
#else
    const auto function = reinterpret_cast<Function>(dlsym(module, symbol.c_str()));
#endif
    if (!function) std::fprintf(stderr, "Missing symbol %s\n", symbol.c_str());
    Require(function != nullptr);
    return function;
}


int main(int argc, char** argv) {
    Require(argc == 4);
    const auto directory = std::filesystem::absolute(argv[1]);
    const bool patched = std::strcmp(argv[2], "patched") == 0;
    const std::string test = argv[3];
#ifdef __linux__
    if (test == "main_late_load") {
        int (APS5_VABI* isMain)() = nullptr;
        int other = -1;
        std::thread loader([&] {
            auto* module = LoadLibraryFile(directory / "libkernel_sys.prx");
            isMain = LoadFunction<int (APS5_VABI*)()>(module, "pthread_main_np", patched);
            other = isMain();
        });
        loader.join();
        Require(other == 0 && isMain() == 1);
        return 0;
    }
#endif
    auto* sys = LoadLibraryFile(directory / "libkernel_sys.prx");
    auto* kernel = LoadLibraryFile(directory / "libkernel.prx");
    auto* libc = LoadLibraryFile(directory / "libc.prx");
    const auto error = LoadFunction<int* (APS5_VABI*)()>(libc, "__error", patched);
    using Attr = void*;
    const auto init = LoadFunction<int (APS5_VABI*)(Attr*)>(kernel, "pthread_attr_init", patched);
    const auto destroy = LoadFunction<int (APS5_VABI*)(Attr*)>(kernel, "pthread_attr_destroy", patched);
    const auto getAffinity = LoadFunction<int (APS5_VABI*)(const Attr*, std::size_t, void*)>(sys, "pthread_attr_getaffinity_np", patched);
    const auto setAffinity = LoadFunction<int (APS5_VABI*)(Attr*, std::size_t, const void*)>(sys, "pthread_attr_setaffinity_np", patched);
    Attr attr = nullptr;
    Require(init(&attr) == 0);
    *error() = 71;
    if (test == "main") {
        const auto isMain = LoadFunction<int (APS5_VABI*)()>(sys, "pthread_main_np", patched);
        int other = -1;
        std::thread worker([&] { other = isMain(); });
        worker.join();
        Require(other == 0 && isMain() == 1 && *error() == 71);
    } else if (test == "stack") {
        const auto setStack = LoadFunction<int (APS5_VABI*)(Attr*, void*)>(sys, "pthread_attr_setstackaddr", patched);
        const auto getStack = LoadFunction<int (APS5_VABI*)(const Attr*, void**, std::size_t*)>(kernel, "pthread_attr_getstack", patched);
        alignas(16) std::array<unsigned char, 16384> stack{};
        void* address = nullptr;
        std::size_t size = 0;
        Require(getStack(&attr, &address, &size) == 0);
        const auto originalSize = size;
        Require(setStack(&attr, stack.data()) == 0 && *error() == 71);
        Require(getStack(&attr, &address, &size) == 0 && address == stack.data() && size == originalSize);
        Require(setStack(&attr, nullptr) == 22 && setStack(nullptr, stack.data()) == 22);
        Require(getStack(&attr, &address, &size) == 0 && address == stack.data());
    } else if (test == "solo") {
        const auto getSolo = LoadFunction<int (APS5_VABI*)(const Attr*, int*)>(sys, "pthread_attr_getsolosched_np", patched);
        const auto setSolo = LoadFunction<int (APS5_VABI*)(Attr*, int)>(kernel, "pthread_attr_setsolosched_np", patched);
        int value = -1;
        Require(getSolo(&attr, &value) == 0 && value == 0);
        Require(setSolo(&attr, 16) == 0 && getSolo(&attr, &value) == 0 && value == 16);
        Require(getSolo(nullptr, &value) == 22 && value == 16);
        Require(getSolo(&attr, nullptr) == 22 && *error() == 71);
    } else if (test == "get_affinity") {
        std::array<unsigned char, 66> output;
        output.fill(0xa5);
        Require(getAffinity(&attr, 64, output.data() + 1) == 0 && *error() == 71);
        Require(output.front() == 0xa5 && output.back() == 0xa5);
        for (std::size_t i = 1; i <= 64; ++i) Require(output[i] == (i <= 32 ? 0xff : 0));
        const auto sceSet = LoadFunction<int (APS5_VABI*)(Attr*, std::uint64_t)>(kernel, "scePthreadAttrSetaffinity", patched, "scePthreadAttrSetaffinity");
        Require(sceSet(&attr, 5) == 0 && getAffinity(&attr, 64, output.data() + 1) == 0);
        Require(output[1] == 5);
        for (std::size_t i = 2; i <= 64; ++i) Require(output[i] == 0);
        output.fill(0xa5);
        Require(getAffinity(&attr, 31, output.data()) == 34);
        for (auto byte : output) Require(byte == 0xa5);
        Require(getAffinity(&attr, 32, nullptr) == 14 && getAffinity(nullptr, 32, output.data()) == 22);
    } else if (test == "set_affinity") {
        std::array<unsigned char, 64> mask{};
        mask[0] = 5;
        mask[31] = 0x80;
        Require(setAffinity(&attr, mask.size(), mask.data()) == 0 && *error() == 71);
        std::array<unsigned char, 64> output{};
        Require(getAffinity(&attr, output.size(), output.data()) == 0 && output == mask);
        mask[63] = 1;
        Require(setAffinity(&attr, mask.size(), mask.data()) == 22);
        Require(getAffinity(&attr, output.size(), output.data()) == 0 && output[31] == 0x80 && output[63] == 0);
        Require(setAffinity(&attr, 31, mask.data()) == 34);
        Require(setAffinity(&attr, 0, mask.data()) == 0);
        Require(getAffinity(&attr, output.size(), output.data()) == 0);
        for (std::size_t i = 0; i < output.size(); ++i) Require(output[i] == (i < 32 ? 0xff : 0));
        mask.fill(0);
        Require(setAffinity(&attr, 32, mask.data()) == 0 && getAffinity(&attr, 64, output.data()) == 0 && output == mask);
        Require(setAffinity(&attr, 1, nullptr) == 0 && setAffinity(nullptr, 0, nullptr) == 22);
    } else {
        Require(false);
    }
    Require(*error() == 71);
    Require(destroy(&attr) == 0);
    std::array<unsigned char, 32> output{};
    Require(setAffinity(&attr, 32, output.data()) == 22 && getAffinity(&attr, 32, output.data()) == 22);
    return 0;
}
