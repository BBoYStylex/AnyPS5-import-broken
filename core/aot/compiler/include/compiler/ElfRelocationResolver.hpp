#ifndef CORE_AOT_COMPILER_ELFRELOCATIONRESOLVER_HPP
#define CORE_AOT_COMPILER_ELFRELOCATIONRESOLVER_HPP

#include <compiler/ElfBinaryParser.hpp>
#include <cstdint>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <span>

namespace Aot::Compiler {

enum class X86RelocationType : uint32_t {
    None = 0,
    Direct64 = 1,
    PcRelative32 = 2,
    Got32 = 3,
    Plt32 = 4,
    Copy = 5,
    GlobDat = 6,
    JumpSlot = 7,
    Relative = 8,
    GotPcRel = 9,
    Direct32 = 10,
    Direct32Signed = 11,
    TpOff64 = 18,
    GotTpOff = 22,
    TpOff32 = 23,
    Pc64 = 24
};

struct ElfSymbolInfo {
    std::string Name;
    uint64_t Value;
    uint64_t Size;
    uint8_t Binding;
    uint8_t Type;
    uint16_t SectionIndex;
};

struct ElfRelocationEntry {
    uint64_t Offset;
    uint32_t SymbolIndex;
    X86RelocationType Type;
    int64_t Addend;
};

class ElfRelocationResolver {
public:
    ElfRelocationResolver();
    ~ElfRelocationResolver();

    bool LoadSymbolsAndRelocations(const ParsedElfBinary& binary, std::span<const uint8_t> binaryData);
    size_t ApplyRelocations(ParsedElfBinary& binary, uint64_t newBaseAddress);

    void SetImportedSymbol(std::string_view name, uint64_t address);
    void SetImportedSymbols(const std::unordered_map<std::string, uint64_t>& symbols);
    [[nodiscard]] std::optional<uint64_t> FindImportedSymbol(std::string_view name) const;
    [[nodiscard]] const std::unordered_map<std::string, uint64_t>& GetImportedSymbols() const { return importedSymbols; }

    void SetTlsBaseAddress(uint64_t address);
    [[nodiscard]] uint64_t GetTlsBaseAddress() const;

    void AddRelocation(const ElfRelocationEntry& entry);
    void AddSymbol(const ElfSymbolInfo& symbol);
    void Clear();

    [[nodiscard]] const std::vector<ElfSymbolInfo>& GetSymbols() const { return symbols; }
    [[nodiscard]] const std::vector<ElfRelocationEntry>& GetRelocations() const { return relocations; }

private:
    std::vector<ElfSymbolInfo> symbols;
    std::vector<ElfRelocationEntry> relocations;
    std::unordered_map<std::string, uint64_t> importedSymbols;
    uint64_t tlsBaseAddress = 0;
};

} // namespace Aot::Compiler
#endif
