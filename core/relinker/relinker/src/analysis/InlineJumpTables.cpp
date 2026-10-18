#include <relinker/analysis/InlineJumpTables.hpp>
#include <codegen/x86/X64InstructionDecoder.hpp>
#include <codegen/CodegenException.hpp>
#include <cstring>
#include <iterator>
#include <map>
#include <set>

namespace Relinker {

namespace {

std::int32_t readI32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    std::int32_t value;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

bool holdsOnlyTables(std::span<const std::uint8_t> function, VirtualAddress begin, VirtualAddress first, const std::set<VirtualAddress>& tables) {
    const auto end = begin + function.size();
    if ((end - first) % 4 != 0) return false;
    for (auto slot = first; slot < end; slot += 4) {
        const auto base = *std::prev(tables.upper_bound(slot));
        const auto target = base + static_cast<VirtualAddress>(static_cast<std::int64_t>(readI32(function, slot - begin)));
        if (target < begin || target >= first) return false;
    }
    return true;
}

class TableLoads {
public:
    std::optional<VirtualAddress> Step(std::span<const std::uint8_t> code, VirtualAddress begin, VirtualAddress address, const Codegen::DecodedInstructionInfo& info) {
        const auto* instruction = code.data() + (address - begin);
        if (!info.IsTwoByteOpcode && info.HasModRm && info.Opcode == 0x8d && info.HasRipRelativeDisp && (info.RexPrefix & 8) != 0) {
            const auto displacement = readI32(code, address - begin + info.RipRelativeDispOffset);
            ripAddresses[static_cast<std::uint8_t>(info.ModRmRegField | ((info.RexPrefix & 4) << 1))] =
                address + info.Length + static_cast<VirtualAddress>(static_cast<std::int64_t>(displacement));
        } else if (!info.IsTwoByteOpcode && info.HasModRm && (info.Opcode == 0x89 || info.Opcode == 0x8b) && (info.ModRmByte >> 6) == 3 && (info.RexPrefix & 8) != 0) {
            const auto reg = static_cast<std::uint8_t>(info.ModRmRegField | ((info.RexPrefix & 4) << 1));
            const auto rm = static_cast<std::uint8_t>((info.ModRmByte & 7) | ((info.RexPrefix & 1) << 3));
            const auto destination = info.Opcode == 0x89 ? rm : reg;
            const auto source = ripAddresses.find(info.Opcode == 0x89 ? reg : rm);
            if (source != ripAddresses.end()) ripAddresses[destination] = source->second;
            else ripAddresses.erase(destination);
        } else if (!info.IsTwoByteOpcode && info.HasModRm && info.Opcode == 0x63 && (info.ModRmByte & 7) == 4 && (info.ModRmByte >> 6) <= 1 && info.OpcodeOffset + 2 < info.Length) {
            const auto sib = instruction[info.OpcodeOffset + 2];
            const auto table = ripAddresses.find(static_cast<std::uint8_t>((sib & 7) | ((info.RexPrefix & 1) << 3)));
            if ((sib >> 6) == 2 && table != ripAddresses.end()) return table->second;
        }
        return std::nullopt;
    }

private:
    std::map<std::uint8_t, VirtualAddress> ripAddresses;
};

}

std::optional<VirtualAddress> FindInlineJumpTables(std::span<const std::uint8_t> function, VirtualAddress begin) {
    const Codegen::X64InstructionDecoder decoder;
    const auto end = begin + function.size();
    TableLoads loads;
    std::set<VirtualAddress> tables;
    for (auto address = begin; address < end;) {
        while (!tables.empty() && *tables.begin() < address) tables.erase(tables.begin());
        if (!tables.empty() && address == *tables.begin() && holdsOnlyTables(function, begin, address, tables)) return address;
        Codegen::DecodedInstructionInfo info;
        try {
            info = decoder.DecodeInstruction(function.data() + (address - begin), end - address);
        } catch (const Codegen::CodegenException&) {
            return std::nullopt;
        }
        if (info.Length == 0 || info.Length > end - address) return std::nullopt;
        if (const auto table = loads.Step(function, begin, address, info); table && *table > address && *table < end) tables.insert(*table);
        address += info.Length;
    }
    return std::nullopt;
}

std::vector<std::pair<VirtualAddress, VirtualAddress>> FindTrailingJumpTables(std::span<const std::uint8_t> code, VirtualAddress begin, VirtualAddress functionEnd) {
    const Codegen::X64InstructionDecoder decoder;
    const auto limit = begin + code.size();
    TableLoads loads;
    std::set<VirtualAddress> bases;
    for (auto address = begin; address < functionEnd;) {
        Codegen::DecodedInstructionInfo info;
        try {
            info = decoder.DecodeInstruction(code.data() + (address - begin), functionEnd - address);
        } catch (const Codegen::CodegenException&) {
            break;
        }
        if (info.Length == 0 || info.Length > functionEnd - address) break;
        if (const auto table = loads.Step(code, begin, address, info); table && *table >= functionEnd && *table < limit) bases.insert(*table);
        address += info.Length;
    }
    std::vector<std::pair<VirtualAddress, VirtualAddress>> tables;
    for (auto base = bases.begin(); base != bases.end(); ++base) {
        const auto stop = std::next(base) == bases.end() ? limit : *std::next(base);
        auto slot = *base;
        while (stop - slot >= 4) {
            const auto target = *base + static_cast<VirtualAddress>(static_cast<std::int64_t>(readI32(code, slot - begin)));
            if (target < begin || target >= functionEnd) break;
            slot += 4;
        }
        if (slot != *base) tables.emplace_back(*base, slot);
    }
    return tables;
}

}
