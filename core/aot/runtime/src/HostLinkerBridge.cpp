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

HostFunctionPointer HostLinkerBridge::ResolveNid(std::string_view libraryName, std::string_view nid) const {
    std::lock_guard<std::mutex> lock(bridgeMutex);
    std::string key = std::string(libraryName) + "::" + std::string(nid);
    auto it = nidMap.find(key);
    if (it != nidMap.end()) {
        return it->second;
    }
    return nullptr;
}

size_t HostLinkerBridge::GetRegisteredCount() const {
    std::lock_guard<std::mutex> lock(bridgeMutex);
    return symbolMap.size() + nidMap.size();
}

} // namespace Aot::Runtime
