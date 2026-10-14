#include "prx/libSceAgc/Command/include/Control.hpp"

namespace Agc::Command {

std::uint32_t* WriteJump(CommandBuffer* buffer, std::uint8_t chain, std::uint8_t cachePolicy, const std::uint32_t* target, std::uint32_t sizeInDwords, const char* function) {
    const auto guestAddress = reinterpret_cast<std::uintptr_t>(target);
    CheckBits(chain, 1, function);
    CheckBits(cachePolicy, 3, function);
    CheckBits(sizeInDwords, 0xfffffu, function);
    Require((guestAddress & 3u) == 0, function, "misaligned jump target");
    return Emit(buffer, 0x3fu, {static_cast<std::uint32_t>(guestAddress) & ~3u, static_cast<std::uint32_t>(guestAddress >> 32u), 0x0f200000u | (static_cast<std::uint32_t>(cachePolicy) << 28u) | (static_cast<std::uint32_t>(chain) << 20u) | sizeInDwords}, function);
}

std::uint32_t* WriteRewind(CommandBuffer* buffer, std::uint32_t initialState, const char* function) {
    return Emit(buffer, 0x59u, {(initialState & 1u) << 31u}, function);
}

std::uint32_t* WritePredication(CommandBuffer* buffer, std::uint8_t condition, std::uint8_t operation, std::uint8_t waitOperation, const volatile void* address, const char* function) {
    CheckBits(condition, 1, function);
    CheckBits(operation, 7, function);
    CheckBits(waitOperation, 1, function);
    const auto guestAddress = reinterpret_cast<std::uintptr_t>(address);
    if (operation != 0) {
        CheckAddress(guestAddress, 8, function);
    }
    return Emit(buffer, 0x20u, {(static_cast<std::uint32_t>(condition) << 8u) | (static_cast<std::uint32_t>(waitOperation) << 12u) | (static_cast<std::uint32_t>(operation) << 16u), static_cast<std::uint32_t>(guestAddress) & ~0xfu, static_cast<std::uint32_t>(guestAddress >> 32u)}, function);
}

std::uint32_t* WriteMemSemaphore(CommandBuffer* buffer, const volatile void* address, std::uint8_t waitForMailbox, std::uint8_t signalType, std::uint8_t operation, const char* function) {
    CheckBits(waitForMailbox, 1, function);
    CheckBits(signalType, 1, function);
    Require(operation == 6 || operation == 7, function, "invalid memory semaphore operation");
    const auto guestAddress = reinterpret_cast<std::uintptr_t>(address);
    CheckAddress(guestAddress, 8, function);
    return Emit(buffer, 0x39u, {static_cast<std::uint32_t>(guestAddress), static_cast<std::uint32_t>(guestAddress >> 32u), (static_cast<std::uint32_t>(operation) << 29u) | (static_cast<std::uint32_t>(signalType) << 20u) | (static_cast<std::uint32_t>(waitForMailbox) << 16u)}, function);
}

std::uint32_t* WriteAtomicGds(CommandBuffer* buffer, std::uint32_t engine, std::uint32_t control, std::uint32_t mode, std::uint32_t gdsOp, std::uint32_t src, std::uint16_t offset, std::uint16_t index, std::uint32_t mask, std::uint32_t data, std::uint64_t compareData, std::uint64_t extraData, const char* function) {
    const auto operation = ((engine & 3u) << 30u) | (control & 0xffu) | ((control & 0x200u) << 7u) |
        ((control & 0x20u) << 13u) | ((control & 0x40u) << 14u) |
        ((control & 0x20u) == 0 && mode == 1 ? 0x20000u : 0u);
    return Emit(buffer, 0x1du, {
        operation, ((gdsOp & 1u) << 8u) | (src & 0x3fu), offset, index,
        mask & 0x00ff00ffu, data, static_cast<std::uint32_t>(compareData),
        static_cast<std::uint32_t>(compareData >> 32u), static_cast<std::uint32_t>(extraData),
        static_cast<std::uint32_t>(extraData >> 32u)}, function);
}

}
