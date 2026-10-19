#ifndef RELINKER_ANALYSIS_MONOAOTRANGES_HPP
#define RELINKER_ANALYSIS_MONOAOTRANGES_HPP

#include <domain/Types.hpp>
#include <cstdint>
#include <map>
#include <span>
#include <utility>
#include <vector>

namespace Relinker {

std::vector<std::pair<Domain::VirtualAddress, Domain::VirtualAddress>> MonoAotDataRanges(const std::vector<std::uint8_t>& bytes, const std::vector<Domain::ProgramHeader>& headers, const std::map<Domain::VirtualAddress, Domain::VirtualAddress>& relativeTargets);
void FillDataRanges(std::span<std::uint8_t> code, Domain::VirtualAddress codeAddress, const std::vector<std::pair<Domain::VirtualAddress, Domain::VirtualAddress>>& ranges);

}

#endif
