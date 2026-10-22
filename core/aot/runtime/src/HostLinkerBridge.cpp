#include <runtime/HostLinkerBridge.hpp>

namespace Aot::Runtime {

HostLinkerBridge& HostLinkerBridge::Instance() {
    static HostLinkerBridge instance;
    return instance;
}

HostLinkerBridge::HostLinkerBridge() = default;
HostLinkerBridge::~HostLinkerBridge() = default;

void HostLinkerBridge::RegisterExport(std::string_view libraryName, std::string_view symbolName, HostFunctionPointer functionPtr) {
    std::lock_guard<std::mutex> lock(bridgeMutex);
    std::string key = std::string(libraryName) + "::" + std::string(symbolName);
    symbolMap[key] = functionPtr;
}

void HostLinkerBridge::RegisterNid(std::string_view libraryName, std::string_view nid, HostFunctionPointer functionPtr) {
    std::lock_guard<std::mutex> lock(bridgeMutex);
    std::string key = std::string(libraryName) + "::" + std::string(nid);
    nidMap[key] = functionPtr;
}

HostFunctionPointer HostLinkerBridge::ResolveSymbol(std::string_view libraryName, std::string_view symbolName) const {
    std::lock_guard<std::mutex> lock(bridgeMutex);
    std::string key = std::string(libraryName) + "::" + std::string(symbolName);
    auto it = symbolMap.find(key);
    if (it != symbolMap.end()) {
        return it->second;
    }
    return nullptr;
}

HostFunctionPointer HostLinkerBridge::ResolveSymbol(std::string_view symbolName) const {
    std::lock_guard<std::mutex> lock(bridgeMutex);
    auto it = symbolMap.find(std::string(symbolName));
    if (it != symbolMap.end()) {
        return it->second;
    }
    std::string suffix = "::" + std::string(symbolName);
    for (const auto& [key, ptr] : symbolMap) {
        if (key.size() >= suffix.size() &&
            key.compare(key.size() - suffix.size(), suffix.size(), suffix) == 0) {
            return ptr;
        }
    }
    return nullptr;
}

HostFunctionPointer HostLinkerBridge::ResolveNid(std::string_view libraryName, std::string_view nid) const {
    std::lock_guard<std::mutex> lock(bridgeMutex);
    std::string key = std::string(libraryName) + "::" + std::string(nid);
    auto it = nidMap.find(key);
    if (it != nidMap.end()) {
        return it->second;
    }
    return nullptr;
}

HostFunctionPointer HostLinkerBridge::ResolveNid(std::string_view nid) const {
    std::lock_guard<std::mutex> lock(bridgeMutex);
    auto it = nidMap.find(std::string(nid));
    if (it != nidMap.end()) {
        return it->second;
    }
    std::string suffix = "::" + std::string(nid);
    for (const auto& [key, ptr] : nidMap) {
        if (key.size() >= suffix.size() &&
            key.compare(key.size() - suffix.size(), suffix.size(), suffix) == 0) {
            return ptr;
        }
    }
    return nullptr;
}

std::unordered_map<std::string, uint64_t> HostLinkerBridge::ExportSymbolTable() const {
    std::lock_guard<std::mutex> lock(bridgeMutex);
    std::unordered_map<std::string, uint64_t> result;
    for (const auto& [key, ptr] : symbolMap) {
        uint64_t addr = reinterpret_cast<uint64_t>(ptr);
        result[key] = addr;
        auto sepPos = key.find("::");
        if (sepPos != std::string::npos) {
            std::string unqualified = key.substr(sepPos + 2);
            result.try_emplace(unqualified, addr);
        }
    }
    for (const auto& [key, ptr] : nidMap) {
        uint64_t addr = reinterpret_cast<uint64_t>(ptr);
        result[key] = addr;
        auto sepPos = key.find("::");
        if (sepPos != std::string::npos) {
            std::string unqualified = key.substr(sepPos + 2);
            result.try_emplace(unqualified, addr);
        }
    }
    return result;
}

size_t HostLinkerBridge::GetRegisteredCount() const {
    std::lock_guard<std::mutex> lock(bridgeMutex);
    return symbolMap.size() + nidMap.size();
}

size_t HostLinkerBridge::GetExportCount() const {
    std::lock_guard<std::mutex> lock(bridgeMutex);
    return symbolMap.size();
}

size_t HostLinkerBridge::GetNidCount() const {
    std::lock_guard<std::mutex> lock(bridgeMutex);
    return nidMap.size();
}

} // namespace Aot::Runtime
