#ifndef CORE_AOT_COMPILER_SYSCALLREGISTRY_HPP
#define CORE_AOT_COMPILER_SYSCALLREGISTRY_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>

namespace Aot::Compiler {

struct SyscallSignature {
    std::string NativeSymbol;
    uint32_t ParameterCount;
    bool ReturnsVoid;
};

class SyscallRegistry {
public:
    static SyscallRegistry& Instance();

    void Register(uint32_t syscallId, std::string_view nativeSymbol, uint32_t parameterCount, bool returnsVoid = false);
    [[nodiscard]] std::optional<SyscallSignature> Find(uint32_t syscallId) const;
    [[nodiscard]] bool Has(uint32_t syscallId) const;

private:
    SyscallRegistry();
    std::unordered_map<uint32_t, SyscallSignature> registryMap;
};

} // namespace Aot::Compiler
#endif
