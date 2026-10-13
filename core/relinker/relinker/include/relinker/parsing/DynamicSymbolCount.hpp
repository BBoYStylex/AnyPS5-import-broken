#ifndef RELINKER_PARSING_DYNAMICSYMBOLCOUNT_HPP
#define RELINKER_PARSING_DYNAMICSYMBOLCOUNT_HPP

#include <domain/Types.hpp>
#include <optional>

namespace Relinker {

std::uint64_t ReadDynamicSymbolCount(
    const std::vector<std::uint8_t>& bytes,
    const std::vector<Domain::ProgramHeader>& headers,
    std::optional<std::uint64_t> sceByteSize,
    std::optional<std::uint64_t> sysvHashAddress,
    std::optional<std::uint64_t> gnuHashAddress,
    std::optional<std::uint64_t> standardSymbolAddress);

}

#endif
