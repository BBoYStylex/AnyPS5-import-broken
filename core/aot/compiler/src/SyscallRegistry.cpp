#include <compiler/SyscallRegistry.hpp>

namespace Aot::Compiler {

SyscallRegistry& SyscallRegistry::Instance() {
    static SyscallRegistry instance;
    return instance;
}

SyscallRegistry::SyscallRegistry() {
    Register(1, "aps5_sys_exit", 1, true);
    Register(3, "aps5_sys_read", 3, false);
    Register(4, "aps5_sys_write", 3, false);
    Register(5, "aps5_sys_open", 3, false);
    Register(6, "aps5_sys_close", 1, false);
    Register(54, "aps5_sys_ioctl", 3, false);
    Register(73, "aps5_sys_munmap", 2, false);
    Register(74, "aps5_sys_mprotect", 3, false);
    Register(477, "aps5_sys_mmap", 6, false);
    Register(161, "aps5_sys_thr_create", 3, false);
    Register(162, "aps5_sys_thr_exit", 1, true);
    Register(163, "aps5_sys_thr_self", 1, false);
    Register(481, "aps5_sys_thr_kill", 2, false);
    Register(482, "aps5_sys_thr_wake", 1, false);
    Register(532, "aps5_sys_regmgr_call", 5, false);
    Register(533, "aps5_sys_jit_allocate", 3, false);
    Register(534, "aps5_sys_dmem_allocate", 4, false);
    Register(535, "aps5_sys_dmem_release", 2, false);
    Register(20, "aps5_sys_getpid", 0, false);
    Register(232, "aps5_sys_clock_gettime", 2, false);
    Register(240, "aps5_sys_nanosleep", 2, false);
    Register(327, "aps5_sys_sched_yield", 0, false);
    Register(585, "aps5_sys_named_event_create", 3, false);
}

void SyscallRegistry::Register(uint32_t syscallId, std::string_view nativeSymbol, uint32_t parameterCount, bool returnsVoid) {
    registryMap[syscallId] = SyscallSignature{
        .NativeSymbol = std::string(nativeSymbol),
        .ParameterCount = parameterCount,
        .ReturnsVoid = returnsVoid
    };
}

std::optional<SyscallSignature> SyscallRegistry::Find(uint32_t syscallId) const {
    auto iterator = registryMap.find(syscallId);
    if (iterator == registryMap.end()) {
        return std::nullopt;
    }
    return iterator->second;
}

bool SyscallRegistry::Has(uint32_t syscallId) const {
    return registryMap.contains(syscallId);
}

} // namespace Aot::Compiler
