#include <compiler/ElfBinaryParser.hpp>
#include <compiler/SyscallRegistry.hpp>
#include <compiler/ElfRelocationResolver.hpp>
#include <compiler/AotCompilerDriver.hpp>
#include <cassert>
#include <vector>

int main() {
    auto& registry = Aot::Compiler::SyscallRegistry::Instance();
    assert(registry.Has(1));
    assert(registry.Has(3));
    assert(registry.Has(20));
    assert(registry.Has(232));
    assert(registry.Has(240));
    assert(registry.Has(327));
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

    Aot::Compiler::CompilerOptions options{};
    assert(options.OptimizationLevel == 2);
    assert(!options.EmitLlvmIr);
    assert(!options.EmitAssembly);
    options.EmitLlvmIr = true;
    options.OutputLlvmIrPath = "dummy.ll";
    options.EmitAssembly = true;
    options.OutputAssemblyPath = "dummy.s";
    assert(options.EmitLlvmIr);
    assert(options.EmitAssembly);

    Aot::Compiler::AotCompilerDriver driver;
    auto stats = driver.Compile(options);
    assert(!stats.Success);

    Aot::Compiler::ElfSection codeSection{};
    codeSection.Name = ".text";
    codeSection.Flags = 0x6;
    codeSection.Data = {
        0x50,
        0x48, 0xB8, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x48, 0x31, 0xD2,
        0x48, 0x01, 0xD0,
        0x48, 0x83, 0xC0, 0x05,
        0x48, 0x81, 0xEB, 0x00, 0x01, 0x00, 0x00,
        0x48, 0x8D, 0x44, 0x24, 0x10,
        0x48, 0x8D, 0x44, 0x94, 0x20,
        0x48, 0x8D, 0x05, 0x10, 0x00, 0x00, 0x00,
        0x48, 0x89, 0x44, 0x24, 0x08,
        0x48, 0x8B, 0x4C, 0x24, 0x08,
        0x0F, 0x57, 0xC0,
        0x0F, 0x28, 0xC8,
        0x0F, 0x58, 0xC8,
        0x0F, 0x59, 0xC8,
        0x0F, 0x5C, 0xC8,
        0x0F, 0x5E, 0xC8,
        0x0F, 0x29, 0x0C, 0x24,
        0x0F, 0x28, 0x0C, 0x24,
        0x74, 0x02,
        0x76, 0x00,
        0x77, 0x00,
        0x7C, 0x00,
        0x7D, 0x00,
        0x7E, 0x00,
        0x7F, 0x00,
        0x0F, 0x84, 0x00, 0x00, 0x00, 0x00,
        0x0F, 0x85, 0x00, 0x00, 0x00, 0x00,
        0xEB, 0x00,
        0x58,
        0xC3
    };

    Aot::Compiler::ParsedElfBinary binary{};
    binary.Sections.push_back(codeSection);

    Aot::Compiler::LlvmLifter lifter;
    auto lifted = lifter.LiftBinary(binary, "test_module");
    assert(lifted.Module != nullptr);
    assert(lifted.LiftedFunctionCount == 1);
    assert(lifted.LiftedInstructionCount == codeSection.Data.size());

    return 0;
}
