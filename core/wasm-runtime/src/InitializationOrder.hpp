#ifndef CORE_WASM_RUNTIME_SRC_INITIALIZATIONORDER_HPP
#define CORE_WASM_RUNTIME_SRC_INITIALIZATIONORDER_HPP

#include <cstddef>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Metadata.hpp"

namespace WasmRuntime {

inline std::vector<std::size_t> InitializationOrder(const std::vector<ImageMeta>& images, std::size_t root, std::vector<bool>& initialized) {
    const auto path = [](std::string_view name) { return name.starts_with("$ORIGIN/") ? name.substr(8) : name; };
    std::unordered_map<std::string_view, std::size_t> byPath;
    for (std::size_t index = 0; index < images.size(); ++index) byPath.emplace(path(images[index].name), index);
    std::vector<std::size_t> order;
    const auto visit = [&](const auto& self, std::size_t index) -> void {
        if (initialized[index]) return;
        initialized[index] = true;
        for (const auto& needed : images[index].needed) {
            if (const auto found = byPath.find(path(needed)); found != byPath.end()) self(self, found->second);
        }
        order.push_back(index);
    };
    visit(visit, root);
    return order;
}

inline std::vector<std::size_t> InitializationOrder(const std::vector<ImageMeta>& images) {
    std::vector<bool> initialized(images.size(), false);
    return images.empty() ? std::vector<std::size_t>{} : InitializationOrder(images, 0, initialized);
}

}  // namespace WasmRuntime

#endif
