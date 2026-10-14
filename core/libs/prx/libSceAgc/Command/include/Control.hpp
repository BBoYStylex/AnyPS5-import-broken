#ifndef CORE_LIBS_PRX_LIBSCEAGC_COMMAND_INCLUDE_CONTROL_HPP
#define CORE_LIBS_PRX_LIBSCEAGC_COMMAND_INCLUDE_CONTROL_HPP

#include "prx/libSceAgc/Command/include/Packet.hpp"

namespace Agc::Command {

std::uint32_t* WriteJump(CommandBuffer* buffer, std::uint8_t chain, std::uint8_t cachePolicy, const std::uint32_t* target, std::uint32_t sizeInDwords, const char* function);
std::uint32_t* WriteRewind(CommandBuffer* buffer, std::uint32_t initialState, const char* function);
std::uint32_t* WritePredication(CommandBuffer* buffer, std::uint8_t condition, std::uint8_t operation, std::uint8_t waitOperation, const volatile void* address, const char* function);
std::uint32_t* WriteMemSemaphore(CommandBuffer* buffer, const volatile void* address, std::uint8_t waitForMailbox, std::uint8_t signalType, std::uint8_t operation, const char* function);
std::uint32_t* WriteAtomicGds(CommandBuffer* buffer, std::uint32_t engine, std::uint32_t control, std::uint32_t mode, std::uint32_t gdsOp, std::uint32_t src, std::uint16_t offset, std::uint16_t index, std::uint32_t mask, std::uint32_t data, std::uint64_t compareData, std::uint64_t extraData, const char* function);

}

#endif
