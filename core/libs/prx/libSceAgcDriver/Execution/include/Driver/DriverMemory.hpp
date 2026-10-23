#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_DRIVER_DRIVERMEMORY_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_DRIVER_DRIVERMEMORY_HPP

#ifdef __APPLE__

#include <cstdint>

namespace AgcDriver::DriverDetail {

// The AGC driver's own guest block (the FS table, its routine table and routine; Lifecycle.cpp) is at
// 0xfe0000000 on the console. macOS itself owns 0xfc0000000-0x1000000000 with maximum protection none, so on macOS
// the block sits lower in the same system-reserved guest range, and guest shaders that build an address inside the
// console block are retargeted to it (ShaderRegistry.cpp).
constexpr std::uint64_t Ps5DriverMemoryBase = 0xfe0000000ull;
constexpr std::uint64_t MacOsDriverMemoryBase = 0xee0000000ull;
constexpr std::uint64_t DriverMemorySpan = 2u << 20u;

constexpr std::uint64_t RetargetPs5DriverMemory(std::uint64_t address) {
    return address >= Ps5DriverMemoryBase && address - Ps5DriverMemoryBase < DriverMemorySpan ? address - Ps5DriverMemoryBase + MacOsDriverMemoryBase : address;
}

}

#endif

#endif
