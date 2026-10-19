#include <relinker/analysis/MonoAotRanges.hpp>
#include <algorithm>
#include <optional>
#include <set>
#include <string>
#include <string_view>

namespace Relinker {

namespace {

using Domain::VirtualAddress;
using Range = std::pair<VirtualAddress, VirtualAddress>;

constexpr VirtualAddress EntrySize = 16;
constexpr VirtualAddress PltEntrySize = 10;
constexpr VirtualAddress PltJumpSize = 6;
constexpr std::size_t MaxNameLength = 128;

class Image {
public:
    Image(const std::vector<std::uint8_t>& bytes, const std::vector<Domain::ProgramHeader>& headers) : bytes(bytes), headers(headers) {}

    const Domain::ProgramHeader* Segment(VirtualAddress address) const {
        for (const auto& header : headers)
            if (header.Type == 1 && address >= header.MappedAddress && address - header.MappedAddress < header.FileSize
                && header.Offset <= bytes.size() && header.FileSize <= bytes.size() - header.Offset)
                return &header;
        return nullptr;
    }

    std::optional<std::string_view> String(VirtualAddress address) const {
        const auto* header = Segment(address);
        if (!header) return std::nullopt;
        const auto* first = bytes.data() + header->Offset + (address - header->MappedAddress);
        const auto* limit = bytes.data() + header->Offset + header->FileSize;
        const auto* last = std::find(first, limit, std::uint8_t{0});
        if (last == limit) return std::nullopt;
        return std::string_view(reinterpret_cast<const char*>(first), static_cast<std::size_t>(last - first));
    }

    std::optional<std::uint64_t> Read(VirtualAddress address, std::size_t size) const {
        const auto* header = Segment(address);
        if (!header || header->FileSize - (address - header->MappedAddress) < size) return std::nullopt;
        std::uint64_t value = 0;
        const auto offset = header->Offset + (address - header->MappedAddress);
        for (std::size_t index = 0; index < size; ++index) value |= static_cast<std::uint64_t>(bytes[offset + index]) << (8 * index);
        return value;
    }

private:
    const std::vector<std::uint8_t>& bytes;
    const std::vector<Domain::ProgramHeader>& headers;
};

bool IsIdentifier(std::string_view text) {
    const auto letter = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; };
    if (text.empty() || text.size() > MaxNameLength || !letter(text.front())) return false;
    return std::all_of(text.begin(), text.end(), [&](char c) { return letter(c) || (c >= '0' && c <= '9'); });
}

bool Executable(const Domain::ProgramHeader* header) { return header && (header->Flags & 1) != 0; }

}

std::vector<Range> MonoAotDataRanges(const std::vector<std::uint8_t>& bytes, const std::vector<Domain::ProgramHeader>& headers, const std::map<VirtualAddress, VirtualAddress>& relativeTargets) {
    const Image image(bytes, headers);
    const auto nameAt = [&](VirtualAddress slot) -> std::optional<std::string_view> {
        const auto target = relativeTargets.find(slot);
        if (target == relativeTargets.end()) return std::nullopt;
        const auto text = image.String(target->second);
        if (!text || !IsIdentifier(*text)) return std::nullopt;
        return text;
    };
    std::set<VirtualAddress> tables;
    for (const auto& [slot, target] : relativeTargets) {
        if (image.String(target) != std::optional<std::string_view>("mono_aot_file_info")) continue;
        auto start = slot;
        while (start >= EntrySize && nameAt(start - EntrySize)) start -= EntrySize;
        tables.insert(start);
    }
    std::vector<Range> blocks;
    std::vector<Range> data;
    for (const auto start : tables) {
        std::map<std::string, VirtualAddress> globals;
        VirtualAddress stringsEnd = 0;
        auto slot = start;
        for (; const auto name = nameAt(slot); slot += EntrySize) {
            const auto value = relativeTargets.find(slot + 8);
            if (value == relativeTargets.end()) throw Domain::RelinkerException("Mono AOT global without a relocated value: " + std::string(*name), slot + 8);
            if (!globals.emplace(*name, value->second).second) throw Domain::RelinkerException("Duplicate Mono AOT global: " + std::string(*name), slot);
            stringsEnd = std::max(stringsEnd, relativeTargets.at(slot) + name->size() + 1);
        }
        if (relativeTargets.contains(slot) || image.Read(slot, 8) != std::optional<std::uint64_t>(0))
            throw Domain::RelinkerException("Unterminated Mono AOT globals table", start);
        for (const auto* key : {"methods", "methods_end", "plt", "plt_end"})
            if (!globals.contains(key)) throw Domain::RelinkerException(std::string("Mono AOT globals table without ") + key, start);
        const auto methods = globals.at("methods");
        const auto plt = globals.at("plt");
        const auto pltEnd = globals.at("plt_end");
        const auto* segment = image.Segment(methods);
        if (!Executable(segment)) throw Domain::RelinkerException("Mono AOT methods outside an executable segment", methods);
        const auto segmentEnd = segment->MappedAddress + segment->FileSize;
        const auto inSegment = [&](VirtualAddress begin, VirtualAddress end) { return begin >= segment->MappedAddress && begin <= end && end <= segmentEnd; };
        for (const auto* key : {"mono_assembly_guid", "mono_aot_version", "mono_runtime_version", "mono_aot_assembly_name"}) {
            if (!globals.contains(key)) continue;
            const auto text = image.String(globals.at(key));
            if (!text) throw Domain::RelinkerException(std::string("Mono AOT string is not file-backed: ") + key, globals.at(key));
            stringsEnd = std::max(stringsEnd, globals.at(key) + text->size() + 1);
        }
        std::set<VirtualAddress> values;
        for (const auto& [key, value] : globals) if (inSegment(value, value)) values.insert(value);
        std::vector<Range> code{{methods, globals.at("methods_end")}, {plt, pltEnd}};
        for (const auto& [key, value] : globals) {
            if (const auto end = globals.find(key + "_p"); end != globals.end() && inSegment(value, value)) code.emplace_back(value, end->second);
            if (key == "specific_trampolines" || key == "static_rgctx_trampolines" || key == "imt_thunks") {
                const auto next = values.upper_bound(value);
                if (next == values.end()) throw Domain::RelinkerException("Mono AOT trampolines without an end: " + key, value);
                code.emplace_back(value, *next);
            }
        }
        std::sort(code.begin(), code.end());
        if (!inSegment(methods, stringsEnd)) throw Domain::RelinkerException("Mono AOT image exceeds its executable segment", methods);
        for (std::size_t index = 0; index < code.size(); ++index) {
            if (!inSegment(code[index].first, code[index].second) || code[index].first < methods || code[index].second > stringsEnd)
                throw Domain::RelinkerException("Mono AOT code range outside its image", code[index].first);
            if (index != 0 && code[index].first < code[index - 1].second) throw Domain::RelinkerException("Overlapping Mono AOT code ranges", code[index].first);
        }
        if ((pltEnd - plt) % PltEntrySize != 0) throw Domain::RelinkerException("Mono AOT PLT size is not a whole number of entries", plt);
        for (auto entry = plt; entry < pltEnd; entry += PltEntrySize) {
            if (image.Read(entry, 2) != std::optional<std::uint64_t>(0x25ff)) throw Domain::RelinkerException("Unexpected Mono AOT PLT entry", entry);
            data.emplace_back(entry + PltJumpSize, entry + PltEntrySize);
        }
        auto position = methods;
        for (const auto& [begin, end] : code) {
            if (begin > position) data.emplace_back(position, begin);
            position = std::max(position, end);
        }
        if (stringsEnd > position) data.emplace_back(position, stringsEnd);
        blocks.emplace_back(methods, stringsEnd);
    }
    std::sort(blocks.begin(), blocks.end());
    for (std::size_t index = 1; index < blocks.size(); ++index)
        if (blocks[index].first < blocks[index - 1].second) throw Domain::RelinkerException("Overlapping Mono AOT images", blocks[index].first);
    std::sort(data.begin(), data.end());
    return data;
}

void FillDataRanges(std::span<std::uint8_t> code, VirtualAddress codeAddress, const std::vector<Range>& ranges) {
    const auto codeEnd = codeAddress + code.size();
    for (const auto& [begin, end] : ranges) {
        const auto first = std::max(begin, codeAddress);
        const auto last = std::min(end, codeEnd);
        if (first < last) std::fill(code.begin() + static_cast<std::ptrdiff_t>(first - codeAddress), code.begin() + static_cast<std::ptrdiff_t>(last - codeAddress), std::uint8_t{0x90});
    }
}

}
