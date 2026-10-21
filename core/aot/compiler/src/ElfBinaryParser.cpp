#include <compiler/ElfBinaryParser.hpp>
#include <fstream>
#include <cstring>
#include <algorithm>

namespace Aot::Compiler {

struct RawElf64Header {
    uint8_t Ident[16];
    uint16_t Type;
    uint16_t Machine;
    uint32_t Version;
    uint64_t EntryPoint;
    uint64_t ProgramHeaderOffset;
    uint64_t SectionHeaderOffset;
    uint32_t Flags;
    uint16_t HeaderSize;
    uint16_t ProgramHeaderEntrySize;
    uint16_t ProgramHeaderCount;
    uint16_t SectionHeaderEntrySize;
    uint16_t SectionHeaderCount;
    uint16_t SectionHeaderStringIndex;
};

struct RawElf64ProgramHeader {
    uint32_t Type;
    uint32_t Flags;
    uint64_t Offset;
    uint64_t VirtualAddress;
    uint64_t PhysicalAddress;
    uint64_t FileSize;
    uint64_t MemorySize;
    uint64_t Alignment;
};

struct RawElf64SectionHeader {
    uint32_t NameOffset;
    uint32_t Type;
    uint64_t Flags;
    uint64_t VirtualAddress;
    uint64_t Offset;
    uint64_t Size;
    uint32_t Link;
    uint32_t Info;
    uint64_t AddressAlignment;
    uint64_t EntrySize;
};

const ElfSection* ParsedElfBinary::FindSection(std::string_view sectionName) const {
    for (const auto& section : Sections) {
        if (section.Name == sectionName) {
            return &section;
        }
    }
    return nullptr;
}

std::optional<ParsedElfBinary> ElfBinaryParser::ParseFromFile(const std::filesystem::path& executablePath) {
    std::ifstream file(executablePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::streamsize fileSize = file.tellg();
    if (fileSize < static_cast<std::streamsize>(sizeof(RawElf64Header))) {
        return std::nullopt;
    }

    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        return std::nullopt;
    }

    return ParseFromMemory(buffer);
}

std::optional<ParsedElfBinary> ElfBinaryParser::ParseFromMemory(std::span<const uint8_t> binaryData) {
    if (binaryData.size() < sizeof(RawElf64Header)) {
        return std::nullopt;
    }

    const auto* header = reinterpret_cast<const RawElf64Header*>(binaryData.data());

    if (header->Ident[0] != 0x7F || header->Ident[1] != 'E' ||
        header->Ident[2] != 'L' || header->Ident[3] != 'F' ||
        header->Ident[4] != 2) {
        return std::nullopt;
    }

    ParsedElfBinary result{};
    result.EntryPoint = header->EntryPoint;

    uint64_t minVirtualAddress = UINT64_MAX;
    uint64_t maxVirtualAddress = 0;

    if (header->ProgramHeaderOffset + (header->ProgramHeaderCount * sizeof(RawElf64ProgramHeader)) <= binaryData.size()) {
        const auto* programHeaders = reinterpret_cast<const RawElf64ProgramHeader*>(
            binaryData.data() + header->ProgramHeaderOffset
        );

        for (uint16_t i = 0; i < header->ProgramHeaderCount; ++i) {
            const auto& ph = programHeaders[i];
            ElfSegment segment{
                .Type = ph.Type,
                .Flags = ph.Flags,
                .FileOffset = ph.Offset,
                .VirtualAddress = ph.VirtualAddress,
                .PhysicalAddress = ph.PhysicalAddress,
                .FileSize = ph.FileSize,
                .MemorySize = ph.MemorySize,
                .Alignment = ph.Alignment
            };
            result.Segments.push_back(segment);

            if (ph.Type == 1) {
                minVirtualAddress = std::min(minVirtualAddress, ph.VirtualAddress);
                maxVirtualAddress = std::max(maxVirtualAddress, ph.VirtualAddress + ph.MemorySize);
            }
        }
    }

    if (minVirtualAddress != UINT64_MAX) {
        result.BaseVirtualAddress = minVirtualAddress;
        uint64_t span = maxVirtualAddress - minVirtualAddress;
        result.TotalMemorySize = (span + ConsolePageAlignment - 1) & ~(ConsolePageAlignment - 1);
    }

    if (header->SectionHeaderOffset != 0 && header->SectionHeaderCount > 0 &&
        header->SectionHeaderOffset + (header->SectionHeaderCount * sizeof(RawElf64SectionHeader)) <= binaryData.size()) {
        const auto* sectionHeaders = reinterpret_cast<const RawElf64SectionHeader*>(
            binaryData.data() + header->SectionHeaderOffset
        );

        const char* stringTable = nullptr;
        if (header->SectionHeaderStringIndex < header->SectionHeaderCount) {
            const auto& strSection = sectionHeaders[header->SectionHeaderStringIndex];
            if (strSection.Offset + strSection.Size <= binaryData.size()) {
                stringTable = reinterpret_cast<const char*>(binaryData.data() + strSection.Offset);
            }
        }

        for (uint16_t i = 0; i < header->SectionHeaderCount; ++i) {
            const auto& sh = sectionHeaders[i];
            std::string sectionName;
            if (stringTable != nullptr && sh.NameOffset < binaryData.size()) {
                sectionName = stringTable + sh.NameOffset;
            }

            ElfSection section{
                .Name = std::move(sectionName),
                .Type = sh.Type,
                .Flags = sh.Flags,
                .VirtualAddress = sh.VirtualAddress,
                .FileOffset = sh.Offset,
                .Size = sh.Size
            };

            if (sh.Type != 8 && sh.Offset + sh.Size <= binaryData.size()) {
                section.Data.assign(
                    binaryData.data() + sh.Offset,
                    binaryData.data() + sh.Offset + sh.Size
                );
            }

            result.Sections.push_back(std::move(section));
        }
    }

    return result;
}

} // namespace Aot::Compiler
