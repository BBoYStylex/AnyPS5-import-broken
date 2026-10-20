#include <relinker/analysis/RelativeJumpTables.hpp>
#include <codegen/x86/X64InstructionDecoder.hpp>
#include <codegen/CodegenException.hpp>
#include <algorithm>
#include <cstring>
#include <optional>
#include <set>

namespace Relinker {

namespace {

constexpr VirtualAddress MaxTableGapAfterFunction = 0x10000;

struct TableLoad {
    std::size_t At;
    std::size_t Target;
};

bool IsNop(std::span<const std::uint8_t> code, std::size_t at, std::size_t length) {
    auto position = at;
    const auto end = at + length;
    while (position < end && (code[position] == 0x66 || code[position] == 0x2e || code[position] == 0x3e)) ++position;
    if (position >= end) return false;
    if (code[position] == 0x90) return position + 1 == end;
    return code[position] == 0x0f && position + 1 < end && code[position + 1] == 0x1f;
}

bool OnlyPadding(std::span<const std::uint8_t> code, std::size_t from, const Codegen::X64InstructionDecoder& decoder) {
    for (auto position = from; position < code.size();) {
        if (code[position] == 0x00 || code[position] == 0xcc || code[position] == 0x90) {
            ++position;
            continue;
        }
        try {
            const auto length = decoder.DecodeInstruction(code.data() + position, code.size() - position).Length;
            if (length == 0 || length > code.size() - position || !IsNop(code, position, length)) return false;
            position += length;
        } catch (const Codegen::CodegenException&) {
            return false;
        }
    }
    return true;
}

std::size_t TableEntries(std::span<const std::uint8_t> code, std::size_t table, std::size_t end, std::size_t firstTable, const std::vector<bool>& instructionStarts) {
    std::size_t entries = 0;
    while (table + (entries + 1) * 4 <= end) {
        std::int32_t relative = 0;
        std::memcpy(&relative, code.data() + table + entries * 4, sizeof(relative));
        const auto destination = static_cast<std::int64_t>(table) + relative;
        if (destination < 0 || destination >= static_cast<std::int64_t>(firstTable) || !instructionStarts[static_cast<std::size_t>(destination)]) break;
        ++entries;
    }
    return entries;
}

std::size_t TablesEnd(std::span<const std::uint8_t> code, std::size_t firstTable, const std::vector<TableLoad>& loads, const std::vector<bool>& instructionStarts) {
    auto table = firstTable;
    while (table < code.size()) {
        auto end = code.size();
        for (const auto& load : loads)
            if (load.Target > table && load.Target < end) end = load.Target;
        const auto entries = TableEntries(code, table, end, firstTable, instructionStarts);
        if (entries == 0 || (table == firstTable && entries < 2)) return table == firstTable ? 0 : table;
        table += entries * 4;
        if (std::none_of(loads.begin(), loads.end(), [&](const auto& load) { return load.Target == table; })) return table;
    }
    return table;
}

bool IsInvalid64BitSingleByteOpcode(std::uint8_t opcode) {
    switch (opcode) {
    case 0x06:
    case 0x07:
    case 0x0e:
    case 0x16:
    case 0x17:
    case 0x1e:
    case 0x1f:
    case 0x27:
    case 0x2f:
    case 0x37:
    case 0x3f:
    case 0x60:
    case 0x61:
    case 0x82:
    case 0x9a:
    case 0xce:
    case 0xd4:
    case 0xd5:
    case 0xd6:
    case 0xea:
        return true;
    default:
        return false;
    }
}

bool IsForbiddenSyscallInstruction(std::span<const std::uint8_t> code, std::size_t pos, const Codegen::DecodedInstructionInfo& info) {
    const auto opPos = pos + info.OpcodeOffset;
    if (opPos + 1 >= pos + info.Length || opPos + 1 >= code.size()) return false;
    const auto b0 = code[opPos];
    const auto b1 = code[opPos + 1];
    return (b0 == 0x0f && (b1 == 0x05 || b1 == 0x34 || b1 == 0x07)) || (b0 == 0xcd && b1 == 0x80);
}

bool IsValid64BitInstruction(std::span<const std::uint8_t> code, std::size_t pos, const Codegen::DecodedInstructionInfo& info, VirtualAddress baseVAddr, VirtualAddress segmentVAddr, VirtualAddress segmentEnd) {
    if (info.Length == 0 || info.Length > code.size() - pos) return false;
    if (!info.IsTwoByteOpcode && IsInvalid64BitSingleByteOpcode(info.Opcode)) return false;
    if (info.HasBranchTarget && !info.HasRipRelativeDisp) {
        const auto target = static_cast<std::int64_t>(baseVAddr + pos + info.Length) + info.BranchDisp;
        if (target < static_cast<std::int64_t>(segmentVAddr) || target >= static_cast<std::int64_t>(segmentEnd)) return false;
    }
    return true;
}

bool DecodesAsTrailingCodeBlock(std::span<const std::uint8_t> code, std::size_t from, VirtualAddress baseVAddr, VirtualAddress segmentVAddr, VirtualAddress segmentEnd, const Codegen::X64InstructionDecoder& decoder, bool* outHasSyscall = nullptr) {
    if (outHasSyscall != nullptr) *outHasSyscall = false;
    if (from >= code.size() || code[from] == 0x00 || code[from] == 0xcc || code[from] == 0x90) return false;
    if (OnlyPadding(code, from, decoder)) return false;
    bool sawNonPadding = false;
    bool lastWasTerminator = false;
    for (auto pos = from; pos < code.size();) {
        if (OnlyPadding(code, pos, decoder)) return sawNonPadding && lastWasTerminator;
        Codegen::DecodedInstructionInfo info;
        try {
            info = decoder.DecodeInstruction(code.data() + pos, code.size() - pos);
        } catch (const Codegen::CodegenException&) {
            return false;
        }
        if (!IsValid64BitInstruction(code, pos, info, baseVAddr, segmentVAddr, segmentEnd)) return false;
        const bool isSyscall = IsForbiddenSyscallInstruction(code, pos, info);
        if (isSyscall && outHasSyscall != nullptr) *outHasSyscall = true;
        if (!IsNop(code, pos, info.Length) && code[pos] != 0xcc && code[pos] != 0x00) {
            sawNonPadding = true;
            lastWasTerminator = info.FlowKind == Codegen::ControlFlowKind::Return
                || info.FlowKind == Codegen::ControlFlowKind::UnconditionalJump
                || info.FlowKind == Codegen::ControlFlowKind::IndirectJump
                || info.FlowKind == Codegen::ControlFlowKind::Trap
                || isSyscall;
        }
        pos += info.Length;
    }
    return sawNonPadding && lastWasTerminator;
}

std::size_t FindEmbeddedDataTableEnd(std::span<const std::uint8_t> code, VirtualAddress tableVAddr, VirtualAddress segmentVAddr, VirtualAddress segmentEnd, const Codegen::X64InstructionDecoder& decoder) {
    std::optional<std::size_t> firstInvalidPos;
    for (std::size_t pos = 0; pos < code.size();) {
        if (OnlyPadding(code, pos, decoder)) break;
        Codegen::DecodedInstructionInfo info;
        try {
            info = decoder.DecodeInstruction(code.data() + pos, code.size() - pos);
        } catch (const Codegen::CodegenException&) {
            firstInvalidPos = pos;
            break;
        }
        if (!IsValid64BitInstruction(code, pos, info, tableVAddr, segmentVAddr, segmentEnd)) {
            firstInvalidPos = pos;
            break;
        }
        if (IsForbiddenSyscallInstruction(code, pos, info) || info.FlowKind != Codegen::ControlFlowKind::Sequential) return 0;
        pos += info.Length;
    }
    if (!firstInvalidPos) return 0;
    const bool duplicatedEightByteRow = code.size() >= 16 && std::memcmp(code.data(), code.data() + 8, 8) == 0;
    if (*firstInvalidPos != 0 && (!duplicatedEightByteRow || *firstInvalidPos > 4)) return 0;

    std::optional<std::size_t> firstDecodeFailure;
    for (std::size_t pos = 0; pos < code.size();) {
        if (OnlyPadding(code, pos, decoder)) break;
        try {
            const auto info = decoder.DecodeInstruction(code.data() + pos, code.size() - pos);
            if (info.Length == 0 || info.Length > code.size() - pos) {
                firstDecodeFailure = pos;
                break;
            }
            if (IsForbiddenSyscallInstruction(code, pos, info)) return 0;
            pos += info.Length;
        } catch (const Codegen::CodegenException&) {
            firstDecodeFailure = pos;
            break;
        }
    }
    if (!firstDecodeFailure) return 0;

    for (std::size_t pos = *firstDecodeFailure + 1; pos < code.size();) {
        if (code[pos] != 0xcc && code[pos] != 0x90 && code[pos] != 0x00) {
            ++pos;
            continue;
        }
        const auto padStart = pos;
        bool hasZeroPad = false;
        while (pos < code.size() && (code[pos] == 0xcc || code[pos] == 0x90 || code[pos] == 0x00)) {
            if (code[pos] == 0x00) hasZeroPad = true;
            ++pos;
        }
        if (pos < code.size() && DecodesAsTrailingCodeBlock(code, pos, tableVAddr, segmentVAddr, segmentEnd, decoder))
            return hasZeroPad ? pos : padStart;
    }
    for (std::size_t pos = *firstDecodeFailure + 1; pos < code.size(); ++pos) {
        bool hasSyscall = false;
        if (DecodesAsTrailingCodeBlock(code, pos, tableVAddr, segmentVAddr, segmentEnd, decoder, &hasSyscall) && hasSyscall)
            return pos;
    }
    return code.size();
}

}

std::size_t FindTrailingRelativeJumpTable(std::span<const std::uint8_t> code) {
    const Codegen::X64InstructionDecoder decoder;
    std::vector<TableLoad> loads;
    std::vector<bool> instructionStarts(code.size() + 1, false);
    bool flowEnded = false;
    bool sawIndirectJump = false;
    std::size_t lastIndirectJump = 0;
    for (std::size_t position = 0; position < code.size();) {
        instructionStarts[position] = true;
        if (flowEnded && sawIndirectJump) {
            for (const auto& load : loads) {
                if (load.Target != position || load.At >= lastIndirectJump) continue;
                const auto end = TablesEnd(code, position, loads, instructionStarts);
                if (end != 0 && OnlyPadding(code, end, decoder)) return position;
            }
        }
        Codegen::DecodedInstructionInfo info;
        try {
            info = decoder.DecodeInstruction(code.data() + position, code.size() - position);
        } catch (const Codegen::CodegenException&) {
            break;
        }
        if (info.Length == 0 || info.Length > code.size() - position) break;
        const auto next = position + info.Length;
        if (info.HasRipRelativeDisp && !info.IsTwoByteOpcode && info.OpcodeOffset < info.Length && info.Length >= 4 && code[position + info.OpcodeOffset] == 0x8d) {
            std::int32_t displacement = 0;
            std::memcpy(&displacement, code.data() + next - 4, sizeof(displacement));
            const auto target = static_cast<std::int64_t>(next) + displacement;
            if (target > static_cast<std::int64_t>(next) && target < static_cast<std::int64_t>(code.size())) loads.push_back({position, static_cast<std::size_t>(target)});
        }
        if (info.FlowKind == Codegen::ControlFlowKind::IndirectJump) {
            sawIndirectJump = true;
            lastIndirectJump = position;
        }
        if (!IsNop(code, position, info.Length)) {
            const bool directJumpOrCall = (info.Length == 5 && (code[position] == 0xe8 || code[position] == 0xe9)) || (info.Length == 2 && code[position] == 0xeb);
            flowEnded = directJumpOrCall || info.FlowKind == Codegen::ControlFlowKind::Return || info.FlowKind == Codegen::ControlFlowKind::IndirectJump || info.FlowKind == Codegen::ControlFlowKind::Trap;
        }
        position = next;
    }
    return code.size();
}

std::vector<RelativeJumpTable> FindRelativeJumpTables(std::span<const std::uint8_t> segment, VirtualAddress segmentVAddr, const std::vector<std::pair<VirtualAddress, VirtualAddress>>& functions) {
    std::vector<RelativeJumpTable> tables;
    const auto segmentEnd = segmentVAddr + segment.size();
    const Codegen::X64InstructionDecoder decoder;
    std::set<VirtualAddress> branchTargets;
    for (const auto& [funcBegin, funcEnd] : functions) {
        if (funcBegin < segmentVAddr || funcBegin >= segmentEnd || funcEnd <= funcBegin) continue;
        const auto inspectFuncEnd = std::min(funcEnd, segmentEnd);
        const auto funcCode = segment.subspan(funcBegin - segmentVAddr, inspectFuncEnd - funcBegin);
        for (std::size_t pos = 0; pos < funcCode.size();) {
            Codegen::DecodedInstructionInfo info;
            try {
                info = decoder.DecodeInstruction(funcCode.data() + pos, funcCode.size() - pos);
            } catch (const Codegen::CodegenException&) {
                break;
            }
            if (info.Length == 0 || info.Length > funcCode.size() - pos) break;
            if (info.HasBranchTarget && !info.HasRipRelativeDisp) {
                const auto target = static_cast<std::int64_t>(funcBegin + pos + info.Length) + info.BranchDisp;
                if (target >= static_cast<std::int64_t>(segmentVAddr) && target < static_cast<std::int64_t>(segmentEnd))
                    branchTargets.insert(static_cast<VirtualAddress>(target));
            }
            pos += info.Length;
        }
    }
    std::vector<VirtualAddress> pendingTargets(branchTargets.begin(), branchTargets.end());
    std::set<VirtualAddress> exploredGaps;
    while (!pendingTargets.empty()) {
        const auto current = pendingTargets.back();
        pendingTargets.pop_back();
        if (current < segmentVAddr || current >= segmentEnd) continue;
        bool inKnownFunction = false;
        for (const auto& [funcBegin, funcEnd] : functions) {
            if (current >= funcBegin && current < funcEnd) {
                inKnownFunction = true;
                break;
            }
        }
        if (inKnownFunction || !exploredGaps.insert(current).second) continue;
        auto pos = static_cast<std::size_t>(current - segmentVAddr);
        while (pos < segment.size()) {
            Codegen::DecodedInstructionInfo info;
            try {
                info = decoder.DecodeInstruction(segment.data() + pos, segment.size() - pos);
            } catch (const Codegen::CodegenException&) {
                break;
            }
            if (info.Length == 0 || info.Length > segment.size() - pos) break;
            if (info.HasBranchTarget && !info.HasRipRelativeDisp) {
                const auto target = static_cast<std::int64_t>(segmentVAddr + pos + info.Length) + info.BranchDisp;
                if (target >= static_cast<std::int64_t>(segmentVAddr) && target < static_cast<std::int64_t>(segmentEnd)) {
                    const auto targetVAddr = static_cast<VirtualAddress>(target);
                    if (branchTargets.insert(targetVAddr).second) pendingTargets.push_back(targetVAddr);
                }
            }
            if (info.FlowKind == Codegen::ControlFlowKind::Return || info.FlowKind == Codegen::ControlFlowKind::IndirectJump || info.FlowKind == Codegen::ControlFlowKind::Trap) break;
            pos += info.Length;
        }
    }
    for (std::size_t index = 0; index < functions.size(); ++index) {
        const auto [begin, end] = functions[index];
        if (begin < segmentVAddr || begin >= segmentEnd || end <= begin) continue;
        auto inspectEnd = std::min(end, segmentEnd);
        if (index + 1 < functions.size() && functions[index + 1].first > inspectEnd && functions[index + 1].first - inspectEnd <= MaxTableGapAfterFunction)
            inspectEnd = std::min(functions[index + 1].first, segmentEnd);
        const auto code = segment.subspan(begin - segmentVAddr, inspectEnd - begin);
        if (const auto tail = FindTrailingRelativeJumpTable(code); tail != 0 && tail < code.size()) {
            tables.push_back({begin, begin + tail, inspectEnd});
            continue;
        }
        if (inspectEnd <= end) continue;
        const auto funcCode = segment.subspan(begin - segmentVAddr, end - begin);
        std::optional<VirtualAddress> candidateTable;
        bool sawIndirectJump = false;
        for (std::size_t pos = 0; pos < funcCode.size();) {
            Codegen::DecodedInstructionInfo info;
            try {
                info = decoder.DecodeInstruction(funcCode.data() + pos, funcCode.size() - pos);
            } catch (const Codegen::CodegenException&) {
                break;
            }
            if (info.Length == 0 || info.Length > funcCode.size() - pos) break;
            if (info.FlowKind == Codegen::ControlFlowKind::IndirectJump) sawIndirectJump = true;
            const auto next = pos + info.Length;
            if (info.HasRipRelativeDisp && !info.IsTwoByteOpcode && info.OpcodeOffset < info.Length && info.Length >= 4 && funcCode[pos + info.OpcodeOffset] == 0x8d) {
                std::int32_t displacement = 0;
                std::memcpy(&displacement, funcCode.data() + next - 4, sizeof(displacement));
                const auto target = static_cast<std::int64_t>(begin + next) + displacement;
                if (target >= static_cast<std::int64_t>(end) && target < static_cast<std::int64_t>(inspectEnd)) {
                    const auto targetVAddr = static_cast<VirtualAddress>(target);
                    candidateTable = candidateTable ? std::min(*candidateTable, targetVAddr) : targetVAddr;
                }
            }
            pos = next;
        }
        if (sawIndirectJump || !candidateTable) continue;
        const auto tableVAddr = *candidateTable;
        const auto paddingCode = segment.subspan(end - segmentVAddr, tableVAddr - end);
        if (!OnlyPadding(paddingCode, 0, decoder)) continue;
        const auto it = branchTargets.lower_bound(tableVAddr);
        if (it != branchTargets.end() && *it < inspectEnd) continue;
        const auto tableCode = segment.subspan(tableVAddr - segmentVAddr, inspectEnd - tableVAddr);
        const auto tableSize = FindEmbeddedDataTableEnd(tableCode, tableVAddr, segmentVAddr, segmentEnd, decoder);
        if (tableSize == 0) continue;
        tables.push_back({begin, tableVAddr, tableVAddr + tableSize});
    }
    return tables;
}

}
