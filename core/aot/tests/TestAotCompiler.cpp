#include <compiler/ElfBinaryParser.hpp>
#include <compiler/SyscallRegistry.hpp>
#include <compiler/ElfRelocationResolver.hpp>
#include <cassert>
#include <vector>

int main() {
    auto& registry = Aot::Compiler::SyscallRegistry::Instance();
    assert(registry.Has(1));
    assert(registry.Has(3));
    assert(registry.Has(477));

    auto exitSig = registry.Find(1);
    assert(exitSig.has_value());
    assert(exitSig->NativeSymbol == "aps5_sys_exit");
    assert(exitSig->ReturnsVoid == true);

    auto mmapSig = registry.Find(477);
    assert(mmapSig.has_value());
    assert(mmapSig->NativeSymbol == "aps5_sys_mmap");
    assert(mmapSig->ParameterCount == 6);

    std::vector<uint8_t> mockElf(64, 0);
    mockElf[0] = 0x7F;
    mockElf[1] = 'E';
    mockElf[2] = 'L';
    mockElf[3] = 'F';
    mockElf[4] = 2;

    Aot::Compiler::ElfBinaryParser parser;
    auto parsedOpt = parser.ParseFromMemory(mockElf);
    assert(parsedOpt.has_value());

    Aot::Compiler::ElfRelocationResolver resolver;
    assert(resolver.GetRelocations().empty());

    return 0;
}
