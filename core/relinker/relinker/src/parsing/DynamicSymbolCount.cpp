#include <relinker/parsing/DynamicSymbolCount.hpp>
#include <io/BufferUtils.hpp>
#include <algorithm>
#include <limits>

namespace Relinker {

std::uint64_t ReadDynamicSymbolCount(
    const std::vector<std::uint8_t>& bytes,
    const std::vector<Domain::ProgramHeader>& headers,
    std::optional<std::uint64_t> sceByteSize,
    std::optional<std::uint64_t> sysvHashAddress,
    std::optional<std::uint64_t> gnuHashAddress,
    std::optional<std::uint64_t> standardSymbolAddress) {
    const auto fileOffset = [&](std::uint64_t address, std::uint64_t size) {
        if (size > std::numeric_limits<std::uint64_t>::max() - address)
            throw Domain::RelinkerException("Dynamic symbols: unmapped address", address);
        for (const auto& header : headers) {
            if (header.Type != 1 || address < header.MappedAddress) continue;
            const auto delta = address - header.MappedAddress;
            if (delta > header.FileSize || size > header.FileSize - delta) continue;
            if (header.Offset > bytes.size() || delta > bytes.size() - header.Offset || size > bytes.size() - header.Offset - delta)
                throw Domain::RelinkerException("Dynamic symbols: file range exceeds image", address);
            return header.Offset + delta;
        }
        throw Domain::RelinkerException("Dynamic symbols: unmapped address", address);
    };
    std::uint64_t symbolCount = 0;
    if (sceByteSize) {
        if (*sceByteSize % 24 != 0)
            throw Domain::RelinkerException("Dynamic symbols: invalid dynamic symbol table size");
        symbolCount = *sceByteSize / 24;
    } else if (sysvHashAddress) {
        const auto offset = fileOffset(*sysvHashAddress, 8);
        const auto bucketCount = Io::ReadU32(bytes, offset);
        symbolCount = Io::ReadU32(bytes, offset + 4);
        fileOffset(*sysvHashAddress, 8 + (static_cast<std::uint64_t>(bucketCount) + symbolCount) * 4);
    } else if (gnuHashAddress) {
        const auto address = *gnuHashAddress;
        const auto header = fileOffset(address, 16);
        const auto bucketCount = Io::ReadU32(bytes, header);
        const auto firstSymbol = Io::ReadU32(bytes, header + 4);
        const auto bloomCount = Io::ReadU32(bytes, header + 8);
        if (bucketCount == 0 || bloomCount == 0 || firstSymbol == 0)
            throw Domain::RelinkerException("Dynamic symbols: invalid GNU hash header", address);
        const std::uint64_t bucketOffset = 16 + static_cast<std::uint64_t>(bloomCount) * 8;
        const auto chainOffset = bucketOffset + static_cast<std::uint64_t>(bucketCount) * 4;
        const auto start = fileOffset(address, chainOffset);
        std::uint64_t lastBucket = 0;
        for (std::uint64_t index = 0; index < bucketCount; ++index) {
            const auto bucket = Io::ReadU32(bytes, start + bucketOffset + index * 4);
            if (bucket != 0 && bucket < firstSymbol)
                throw Domain::RelinkerException("Dynamic symbols: invalid GNU hash bucket", address);
            lastBucket = std::max(lastBucket, static_cast<std::uint64_t>(bucket));
        }
        symbolCount = firstSymbol;
        if (lastBucket != 0) {
            symbolCount = lastBucket;
            for (;;) {
                if (symbolCount - firstSymbol > (std::numeric_limits<std::uint64_t>::max() - chainOffset) / 4)
                    throw Domain::RelinkerException("Dynamic symbols: invalid GNU hash chain", address);
                const auto offset = chainOffset + (symbolCount - firstSymbol) * 4;
                if (offset > std::numeric_limits<std::uint64_t>::max() - address)
                    throw Domain::RelinkerException("Dynamic symbols: invalid GNU hash chain", address);
                const auto chain = Io::ReadU32(bytes, fileOffset(address + offset, 4));
                ++symbolCount;
                if (chain & 1) break;
            }
        }
    }
    if (symbolCount > std::numeric_limits<std::uint64_t>::max() / 24)
        throw Domain::RelinkerException("Dynamic symbols: invalid dynamic symbol table size");
    if (symbolCount != 0 && standardSymbolAddress)
        fileOffset(*standardSymbolAddress, symbolCount * 24);
    return symbolCount;
}

}
