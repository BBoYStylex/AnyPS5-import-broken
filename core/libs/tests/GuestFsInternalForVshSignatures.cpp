#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceFsInternalForVsh/include/FsInternalForVsh.hpp"
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <type_traits>

static void Require(bool value) { if (!value) std::abort(); }

static_assert(std::is_same_v<decltype(&sceFsInitMountSaveDataOpt), int (APS5_VABI *)(void*)>);
static_assert(std::is_same_v<decltype(&sceFsInitUmountSaveDataOpt), int (APS5_VABI *)(void*)>);
static_assert(std::is_same_v<decltype(&sceFsMountSaveData), int (APS5_VABI *)(void*, const char*, const char*, std::uint8_t*)>);
static_assert(std::is_same_v<decltype(&sceFsUmountSaveData), int (APS5_VABI *)(void*, const char*, int, int)>);

template <typename TCall>
static bool ReportsNotImplemented(const char* name, TCall call) {
    try {
        call();
    } catch (const std::runtime_error& error) {
        return std::string(error.what()) == std::string(name) + " not implemented";
    }
    return false;
}

int main() {
    const char* path = "/data";
    std::uint8_t key[16]{};
    Require(ReportsNotImplemented("sceFsInitMountSaveDataOpt", [] { sceFsInitMountSaveDataOpt(nullptr); }));
    Require(ReportsNotImplemented("sceFsInitUmountSaveDataOpt", [] { sceFsInitUmountSaveDataOpt(nullptr); }));
    Require(ReportsNotImplemented("sceFsMountSaveData", [&] { sceFsMountSaveData(nullptr, path, path, key); }));
    Require(ReportsNotImplemented("sceFsUmountSaveData", [&] { sceFsUmountSaveData(nullptr, path, -1, 1); }));
    return 0;
}
