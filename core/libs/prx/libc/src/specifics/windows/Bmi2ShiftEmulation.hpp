#pragma once

#ifdef _WIN32
#include <windows.h>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {
namespace Bmi2Shift {

enum class Op { Shlx, Shrx, Sarx };

struct Instruction {
    Op Operation;
    bool Wide;
    unsigned Destination;
    unsigned Count;
    bool Memory;
    unsigned Source;
    int Base = -1;
    int Index = -1;
    unsigned Scale = 0;
    std::int64_t Displacement = 0;
    bool Relative = false;
    std::size_t Size = 0;
};

inline DWORD64& Register(CONTEXT& context, unsigned number) {
    switch (number) {
    case 0: return context.Rax;
    case 1: return context.Rcx;
    case 2: return context.Rdx;
    case 3: return context.Rbx;
    case 4: return context.Rsp;
    case 5: return context.Rbp;
    case 6: return context.Rsi;
    case 7: return context.Rdi;
    case 8: return context.R8;
    case 9: return context.R9;
    case 10: return context.R10;
    case 11: return context.R11;
    case 12: return context.R12;
    case 13: return context.R13;
    case 14: return context.R14;
    default: return context.R15;
    }
}

inline bool Decode(const std::uint8_t* bytes, std::size_t available, Instruction& out) {
    if (available < 5 || bytes[0] != 0xc4 || (bytes[1] & 0x1f) != 2 || bytes[3] != 0xf7) return false;
    const auto vex = bytes[2];
    if ((vex & 4) != 0 || (vex & 3) == 0) return false;
    const auto modrm = bytes[4];
    const unsigned mod = modrm >> 6;
    const unsigned rm = modrm & 7;
    const unsigned baseExtension = (bytes[1] & 0x20) ? 0 : 8;
    const unsigned indexExtension = (bytes[1] & 0x40) ? 0 : 8;
    Instruction result{};
    result.Operation = (vex & 3) == 1 ? Op::Shlx : (vex & 3) == 2 ? Op::Sarx : Op::Shrx;
    result.Wide = (vex & 0x80) != 0;
    result.Destination = ((modrm >> 3) & 7) | ((bytes[1] & 0x80) ? 0 : 8);
    result.Count = ((vex ^ 0xffu) >> 3) & 15;
    result.Memory = mod != 3;
    result.Source = rm | baseExtension;
    std::size_t cursor = 5;
    unsigned displacementSize = mod == 1 ? 1 : mod == 2 ? 4 : 0;
    if (result.Memory) {
        if (rm == 4) {
            if (cursor >= available) return false;
            const auto sib = bytes[cursor++];
            const unsigned base = sib & 7;
            const unsigned index = (sib >> 3) & 7;
            result.Scale = sib >> 6;
            if (index != 4 || indexExtension != 0) result.Index = index | indexExtension;
            if (mod == 0 && base == 5) displacementSize = 4;
            else result.Base = base | baseExtension;
        } else if (mod == 0 && rm == 5) {
            result.Relative = true;
            displacementSize = 4;
        } else {
            result.Base = result.Source;
        }
    }
    if (cursor + displacementSize > available) return false;
    if (displacementSize == 1) {
        std::int8_t displacement;
        std::memcpy(&displacement, bytes + cursor, sizeof(displacement));
        result.Displacement = displacement;
    } else if (displacementSize == 4) {
        std::int32_t displacement;
        std::memcpy(&displacement, bytes + cursor, sizeof(displacement));
        result.Displacement = displacement;
    }
    result.Size = cursor + displacementSize;
    out = result;
    return true;
}

inline bool Emulate(const std::uint8_t* bytes, std::size_t available, CONTEXT& context) {
    Instruction instruction;
    if (!Decode(bytes, available, instruction)) return false;
    std::uint64_t value;
    if (instruction.Memory) {
        std::uint64_t address = static_cast<std::uint64_t>(instruction.Displacement);
        if (instruction.Relative) address += context.Rip + instruction.Size;
        if (instruction.Base >= 0) address += Register(context, instruction.Base);
        if (instruction.Index >= 0) address += Register(context, instruction.Index) << instruction.Scale;
        value = 0;
        SIZE_T read = 0;
        const SIZE_T size = instruction.Wide ? 8 : 4;
        if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address), &value, size, &read) || read != size) return false;
    } else {
        value = Register(context, instruction.Source);
    }
    const unsigned width = instruction.Wide ? 64 : 32;
    const auto mask = instruction.Wide ? ~std::uint64_t{0} : std::uint64_t{0xffffffff};
    value &= mask;
    const unsigned count = Register(context, instruction.Count) & (width - 1);
    std::uint64_t result;
    if (instruction.Operation == Op::Shlx) {
        result = (value << count) & mask;
    } else {
        result = value >> count;
        if (instruction.Operation == Op::Sarx && count != 0 && (value & (std::uint64_t{1} << (width - 1))) != 0) {
            result |= (mask << (width - count)) & mask;
        }
    }
    Register(context, instruction.Destination) = result;
    context.Rip += instruction.Size;
    return true;
}

}
}
#endif
