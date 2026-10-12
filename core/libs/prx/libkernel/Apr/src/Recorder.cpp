#include <cstdint>
#include <cstddef>
#include <cstring>
#include "SceTypes.hpp"
#include "prx/libkernel/KernelErrors.hpp"
#include "prx/libkernel/Apr/include/AprCommandBuffer.hpp"

namespace {

int AppendAmm(Apr::CommandBufferObject* buffer, const void* command, std::uint32_t bytes) {
    if (bytes > buffer->size - buffer->offset) return SCE_KERNEL_ERROR_EBUSY;
    std::memcpy(buffer->base + buffer->offset, command, bytes);
    buffer->offset += bytes;
    ++buffer->numCommands;
    return 0;
}

bool ValidMapRange(std::uint64_t address, std::uint64_t size) {
    constexpr std::uint64_t pageMask = 0x3FFFu;
    return address != 0u && size != 0u && ((address | size) & pageMask) == 0u && address + size > address;
}

bool ValidAmmProtection(std::int32_t protection) {
    return (static_cast<std::uint32_t>(protection) & 0xFFFFFC0Cu) == 0u;
}

bool ValidRemap(std::uint64_t address, std::uint64_t source, std::uint64_t size, std::int32_t protection) {
    return ValidMapRange(address, size) && ValidMapRange(source, size) && ValidAmmProtection(protection);
}

bool ValidProtect(std::uint64_t address, std::uint64_t size, std::int32_t protection, std::int32_t mask) {
    return ValidMapRange(address, size) && ValidAmmProtection(protection) && ValidAmmProtection(mask);
}

}

extern "C" {

int APS5_VABI sceKernelWriteModifyMtypeProtectCommand(Apr::CommandBufferObject* buffer, std::uint64_t address, std::uint64_t size, std::int32_t type, std::int32_t protection, std::int32_t mask) {
    if (!buffer) return SCE_KERNEL_ERROR_EINVAL;
    if (!buffer->base) return SCE_KERNEL_ERROR_EPERM;
    if (!ValidProtect(address, size, protection, mask)) return SCE_KERNEL_ERROR_EINVAL;
    const Apr::AmmProtectCommand command{{Apr::Opcode::AmmModifyMtypeProtect, sizeof(command)}, address, size, type, protection, mask, 0};
    return AppendAmm(buffer, &command, sizeof(command));
}

int APS5_VABI sceKernelWriteRemapIntoPrtCommand(Apr::CommandBufferObject* buffer, std::uint64_t address, std::uint64_t source, std::uint64_t size, std::int32_t protection, std::uint32_t opcode) {
    if (!buffer) return SCE_KERNEL_ERROR_EINVAL;
    if (!buffer->base) return SCE_KERNEL_ERROR_EPERM;
    if (!ValidRemap(address, source, size, protection)) return SCE_KERNEL_ERROR_EINVAL;
    const Apr::AmmRemapCommand command{{Apr::Opcode::AmmRemapIntoPrt, sizeof(command)}, address, source, size, protection, static_cast<std::int32_t>(opcode ? opcode : 1011u)};
    return AppendAmm(buffer, &command, sizeof(command));
}

}
