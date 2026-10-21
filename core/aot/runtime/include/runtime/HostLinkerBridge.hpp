#ifndef CORE_AOT_RUNTIME_HOSTLINKERBRIDGE_HPP
#define CORE_AOT_RUNTIME_HOSTLINKERBRIDGE_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <mutex>
#include <optional>

namespace Aot::Runtime {

using HostFunctionPointer = void (*)();

class HostLinkerBridge {
public:
    static HostLinkerBridge& Instance();

    void RegisterExport(std::string_view libraryName, std::string_view symbolName, HostFunctionPointer functionPtr);
    void RegisterNid(std::string_view libraryName, std::string_view nid, HostFunctionPointer functionPtr);

    [[nodiscard]] HostFunctionPointer ResolveSymbol(std::string_view libraryName, std::string_view symbolName) const;
    [[nodiscard]] HostFunctionPointer ResolveNid(std::string_view libraryName, std::string_view nid) const;

    [[nodiscard]] size_t GetRegisteredCount() const;

private:
    HostLinkerBridge();
    ~HostLinkerBridge();

    mutable std::mutex bridgeMutex;
    std::unordered_map<std::string, HostFunctionPointer> symbolMap;
    std::unordered_map<std::string, HostFunctionPointer> nidMap;
};

} // namespace Aot::Runtime
#endif
