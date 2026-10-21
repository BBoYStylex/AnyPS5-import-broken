#ifndef CORE_AOT_COMPILER_ELFBINARYPARSER_HPP
#define CORE_AOT_COMPILER_ELFBINARYPARSER_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <span>
#include <optional>

namespace Aot::Compiler {

struct ElfSection {
    std::string Name;
    uint32_t Type;
    uint64_t Flags;
    uint64_t VirtualAddress;
    uint64_t FileOffset;
    uint64_t Size;
    std::vector<uint8_t> Data;
};

struct ElfSegment {
    uint32_t Type;
    uint32_t Flags;
    uint64_t FileOffset;
    uint64_t VirtualAddress;
    uint64_t PhysicalAddress;
    uint64_t FileSize;
    uint64_t MemorySize;
    uint64_t Alignment;
};

struct ParsedElfBinary {
    uint64_t EntryPoint;
    uint64_t BaseVirtualAddress;
    uint64_t TotalMemorySize;
    std::vector<ElfSection> Sections;
    std::vector<ElfSegment> Segments;

    [[nodiscard]] const ElfSection* FindSection(std::string_view sectionName) const;
};

class ElfBinaryParser {
public:
    static constexpr size_t ConsolePageAlignment = 16ULL * 1024ULL;
    static constexpr size_t ConsoleLargePageAlignment = 2ULL * 1024ULL * 1024ULL;

    [[nodiscard]] std::optional<ParsedElfBinary> ParseFromFile(const std::filesystem::path& executablePath);
    [[nodiscard]] std::optional<ParsedElfBinary> ParseFromMemory(std::span<const uint8_t> binaryData);

private:
    bool validateHeader(const uint8_t* data, size_t size);
};

} // namespace Aot::Compiler
#endif
