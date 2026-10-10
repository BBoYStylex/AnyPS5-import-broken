#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>

extern "C" {
int APS5_VABI sceSysmoduleIsLoaded(std::uint16_t id);
int APS5_VABI sceSysmoduleLoadModule(std::uint16_t id);
int APS5_VABI sceSysmoduleUnloadModule(std::uint16_t id);
int APS5_VABI sceSysmoduleLoadModuleInternal(std::uint32_t id);
int APS5_VABI sceSysmoduleUnloadModuleInternal(std::uint32_t id);
int APS5_VABI sceSysmoduleGetModuleHandleInternal(std::uint32_t id, std::int32_t* handle);
int APS5_VABI sceKernelDlsym(std::int32_t handle, const char* symbol, void** address);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

constexpr std::uint16_t kFiberModuleId = 0x0006;
constexpr std::uint16_t kUltModuleId = 0x0007;
constexpr std::uint16_t kMouseModuleId = 0x00a9;
constexpr int kModuleUnloaded = static_cast<int>(0x805A1001);

}

int main() {
    Require(sceSysmoduleIsLoaded(kFiberModuleId) == kModuleUnloaded);
    Require(sceSysmoduleLoadModuleInternal(kFiberModuleId) == 0);
    Require(sceSysmoduleIsLoaded(kFiberModuleId) == 0);
    Require(sceSysmoduleLoadModuleInternal(kFiberModuleId) == 0);
    Require(sceSysmoduleUnloadModuleInternal(kFiberModuleId) == 0);
    Require(sceSysmoduleIsLoaded(kFiberModuleId) == 0);
    Require(sceSysmoduleUnloadModuleInternal(kFiberModuleId) == 0);
    Require(sceSysmoduleIsLoaded(kFiberModuleId) == kModuleUnloaded);
    Require(sceSysmoduleUnloadModuleInternal(kFiberModuleId) == kModuleUnloaded);

    Require(sceSysmoduleIsLoaded(kUltModuleId) == kModuleUnloaded);
    Require(sceSysmoduleUnloadModule(kUltModuleId) == kModuleUnloaded);
    Require(sceSysmoduleLoadModule(kUltModuleId) == 0);
    Require(sceSysmoduleIsLoaded(kUltModuleId) == 0);
    Require(sceSysmoduleUnloadModule(kUltModuleId) == 0);
    Require(sceSysmoduleIsLoaded(kUltModuleId) == kModuleUnloaded);

    std::int32_t handle = -1;
    Require(sceSysmoduleGetModuleHandleInternal(kMouseModuleId, &handle) == kModuleUnloaded && handle == -1);
    Require(sceSysmoduleLoadModule(kMouseModuleId) == 0);
    Require(sceSysmoduleGetModuleHandleInternal(kMouseModuleId, &handle) == 0 && handle >= 0);
    std::int32_t again = -1;
    Require(sceSysmoduleGetModuleHandleInternal(kMouseModuleId, &again) == 0 && again == handle);
    void* init = nullptr;
    Require(sceKernelDlsym(handle, "sceMouseInit", &init) == 0 && init != nullptr);
    Require(sceSysmoduleGetModuleHandleInternal(0x80000000u | 0x7fffu, &again) == kModuleUnloaded);
    Require(sceSysmoduleUnloadModule(kMouseModuleId) == 0);
    Require(sceSysmoduleGetModuleHandleInternal(kMouseModuleId, &again) == kModuleUnloaded);
}
