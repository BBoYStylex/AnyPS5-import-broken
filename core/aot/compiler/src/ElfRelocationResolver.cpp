#include <compiler/ElfRelocationResolver.hpp>
#include <cstring>

namespace Aot::Compiler {

struct RawElf64Rela {
    uint64_t Offset;
    uint64_t Info;
    int64_t Addend;
};

struct RawElf64Sym {
    uint32_t NameOffset;
    uint8_t Info;
    uint8_t Other;
    uint16_t SectionIndex;
    uint64_t Value;
    uint64_t Size;
};

ElfRelocationResolver::ElfRelocationResolver() = default;
ElfRelocationResolver::~ElfRelocationResolver() = default;

bool ElfRelocationResolver::LoadSymbolsAndRelocations(const ParsedElfBinary& binary, std::span<const uint8_t> binaryData) {
    symbols.clear();
    relocations.clear();

    const ElfSection* symTab = binary.FindSection(".symtab");
    if (symTab == nullptr) {
        symTab = binary.FindSection(".dynsym");
    }

    const ElfSection* strTab = binary.FindSection(".strtab");
    if (strTab == nullptr) {
        strTab = binary.FindSection(".dynstr");
    }

    if (symTab != nullptr && !symTab->Data.empty()) {
        size_t count = symTab->Data.size() / sizeof(RawElf64Sym);
        const auto* rawSymbols = reinterpret_cast<const RawElf64Sym*>(symTab->Data.data());

        const char* strings = (strTab != nullptr && !strTab->Data.empty())
            ? reinterpret_cast<const char*>(strTab->Data.data())
            : nullptr;

        for (size_t i = 0; i < count; ++i) {
            const auto& rawSym = rawSymbols[i];
            std::string symName;
            if (strings != nullptr && rawSym.NameOffset < strTab->Data.size()) {
                symName = strings + rawSym.NameOffset;
            }

            symbols.push_back(ElfSymbolInfo{
                .Name = std::move(symName),
                .Value = rawSym.Value,
                .Size = rawSym.Size,
                .Binding = static_cast<uint8_t>(rawSym.Info >> 4),
                .Type = static_cast<uint8_t>(rawSym.Info & 0x0F),
                .SectionIndex = rawSym.SectionIndex
            });
        }
    }

    for (const auto& section : binary.Sections) {
        if (section.Type == 4 && !section.Data.empty()) {
            size_t count = section.Data.size() / sizeof(RawElf64Rela);
            const auto* rawRelas = reinterpret_cast<const RawElf64Rela*>(section.Data.data());

            for (size_t i = 0; i < count; ++i) {
                const auto& rawRela = rawRelas[i];
                uint32_t symIndex = static_cast<uint32_t>(rawRela.Info >> 32);
                uint32_t typeValue = static_cast<uint32_t>(rawRela.Info & 0xFFFFFFFF);

                relocations.push_back(ElfRelocationEntry{
                    .Offset = rawRela.Offset,
                    .SymbolIndex = symIndex,
                    .Type = static_cast<X86RelocationType>(typeValue),
                    .Addend = rawRela.Addend
                });
            }
        }
    }

    return !relocations.empty() || !symbols.empty();
}

size_t ElfRelocationResolver::ApplyRelocations(ParsedElfBinary& binary, uint64_t newBaseAddress) {
    size_t appliedCount = 0;

    for (const auto& rel : relocations) {
        for (auto& section : binary.Sections) {
            if (rel.Offset >= section.VirtualAddress &&
                rel.Offset + sizeof(uint64_t) <= section.VirtualAddress + section.Data.size()) {

                size_t offsetInSection = static_cast<size_t>(rel.Offset - section.VirtualAddress);
                uint8_t* targetBytes = section.Data.data() + offsetInSection;

                if (rel.Type == X86RelocationType::Relative) {
                    uint64_t resolvedValue = newBaseAddress + static_cast<uint64_t>(rel.Addend);
                    std::memcpy(targetBytes, &resolvedValue, sizeof(uint64_t));
                    ++appliedCount;
                } else if (rel.Type == X86RelocationType::Direct64 && rel.SymbolIndex < symbols.size()) {
                    uint64_t symVal = symbols[rel.SymbolIndex].Value;
                    uint64_t resolvedValue = (symVal != 0 ? symVal : newBaseAddress) + static_cast<uint64_t>(rel.Addend);
                    std::memcpy(targetBytes, &resolvedValue, sizeof(uint64_t));
                    ++appliedCount;
                } else if (rel.Type == X86RelocationType::Direct32 && rel.SymbolIndex < symbols.size()) {
                    uint64_t symVal = symbols[rel.SymbolIndex].Value;
                    uint32_t resolvedValue = static_cast<uint32_t>((symVal != 0 ? symVal : newBaseAddress) + rel.Addend);
                    std::memcpy(targetBytes, &resolvedValue, sizeof(uint32_t));
                    ++appliedCount;
                }
                break;
            }
        }
    }

    return appliedCount;
}

} // namespace Aot::Compiler
