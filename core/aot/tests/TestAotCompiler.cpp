#include <compiler/ElfBinaryParser.hpp>
#include <compiler/SyscallRegistry.hpp>
#include <compiler/ElfRelocationResolver.hpp>
#include <compiler/AotCompilerDriver.hpp>
#include <cassert>
#include <vector>
#include <cstring>

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
    resolver.SetImportedSymbol("sceKernelExitProcess", 0x7FFF0000ULL);
    auto foundSym = resolver.FindImportedSymbol("sceKernelExitProcess");
    assert(foundSym.has_value());
    assert(*foundSym == 0x7FFF0000ULL);
    assert(!resolver.FindImportedSymbol("nonExistent").has_value());

    std::unordered_map<std::string, uint64_t> batchSymbols = {
        {"sceKernelCreateEqueue", 0x7FFF1000ULL},
        {"sceKernelDeleteEqueue", 0x7FFF2000ULL}
    };
    resolver.SetImportedSymbols(batchSymbols);
    assert(resolver.FindImportedSymbol("sceKernelCreateEqueue") == 0x7FFF1000ULL);
    assert(resolver.FindImportedSymbol("sceKernelDeleteEqueue") == 0x7FFF2000ULL);
    assert(resolver.GetImportedSymbols().size() == 3);

    resolver.SetTlsBaseAddress(0x40);
    assert(resolver.GetTlsBaseAddress() == 0x40);

    resolver.AddSymbol(Aot::Compiler::ElfSymbolInfo{
        .Name = "sceKernelExitProcess",
        .Value = 0,
        .Size = 8,
        .Binding = 1,
        .Type = 2,
        .SectionIndex = 1
    });
    resolver.AddSymbol(Aot::Compiler::ElfSymbolInfo{
        .Name = "tls_var_1",
        .Value = 0x100,
        .Size = 4,
        .Binding = 1,
        .Type = 6,
        .SectionIndex = 2
    });

    resolver.AddRelocation(Aot::Compiler::ElfRelocationEntry{
        .Offset = 0x1000,
        .SymbolIndex = 0,
        .Type = Aot::Compiler::X86RelocationType::Relative,
        .Addend = 0x20
    });
    resolver.AddRelocation(Aot::Compiler::ElfRelocationEntry{
        .Offset = 0x1008,
        .SymbolIndex = 0,
        .Type = Aot::Compiler::X86RelocationType::JumpSlot,
        .Addend = 0
    });
    resolver.AddRelocation(Aot::Compiler::ElfRelocationEntry{
        .Offset = 0x1010,
        .SymbolIndex = 0,
        .Type = Aot::Compiler::X86RelocationType::GlobDat,
        .Addend = 0x10
    });
    resolver.AddRelocation(Aot::Compiler::ElfRelocationEntry{
        .Offset = 0x1018,
        .SymbolIndex = 0,
        .Type = Aot::Compiler::X86RelocationType::Direct32,
        .Addend = 0
    });
    resolver.AddRelocation(Aot::Compiler::ElfRelocationEntry{
        .Offset = 0x1020,
        .SymbolIndex = 0,
        .Type = Aot::Compiler::X86RelocationType::PcRelative32,
        .Addend = 0
    });
    resolver.AddRelocation(Aot::Compiler::ElfRelocationEntry{
        .Offset = 0x1028,
        .SymbolIndex = 1,
        .Type = Aot::Compiler::X86RelocationType::TpOff32,
        .Addend = 4
    });
    resolver.AddRelocation(Aot::Compiler::ElfRelocationEntry{
        .Offset = 0x1030,
        .SymbolIndex = 1,
        .Type = Aot::Compiler::X86RelocationType::TpOff64,
        .Addend = 8
    });
    resolver.AddRelocation(Aot::Compiler::ElfRelocationEntry{
        .Offset = 0x1038,
        .SymbolIndex = 0,
        .Type = Aot::Compiler::X86RelocationType::Pc64,
        .Addend = 0
    });

    Aot::Compiler::ElfSection synthSec{};
    synthSec.VirtualAddress = 0x1000;
    synthSec.Data.assign(64, 0);

    Aot::Compiler::ParsedElfBinary synthBinary{};
    synthBinary.Sections.push_back(synthSec);

    size_t applied = resolver.ApplyRelocations(synthBinary, 0x400000ULL);
    assert(applied == 8);

    uint64_t val0 = 0;
    std::memcpy(&val0, synthBinary.Sections[0].Data.data() + 0, sizeof(uint64_t));
    assert(val0 == 0x400020ULL);

    uint64_t val1 = 0;
    std::memcpy(&val1, synthBinary.Sections[0].Data.data() + 8, sizeof(uint64_t));
    assert(val1 == 0x7FFF0000ULL);

    uint64_t val2 = 0;
    std::memcpy(&val2, synthBinary.Sections[0].Data.data() + 16, sizeof(uint64_t));
    assert(val2 == 0x7FFF0010ULL);

    uint32_t val3 = 0;
    std::memcpy(&val3, synthBinary.Sections[0].Data.data() + 24, sizeof(uint32_t));
    assert(val3 == 0x7FFF0000U);

    uint32_t val4 = 0;
    std::memcpy(&val4, synthBinary.Sections[0].Data.data() + 32, sizeof(uint32_t));
    assert(val4 == static_cast<uint32_t>(0x7FFF0000ULL - 0x1020ULL));

    int32_t val5 = 0;
    std::memcpy(&val5, synthBinary.Sections[0].Data.data() + 40, sizeof(int32_t));
    assert(val5 == (0x100 + 4 - 0x40));

    int64_t val6 = 0;
    std::memcpy(&val6, synthBinary.Sections[0].Data.data() + 48, sizeof(int64_t));
    assert(val6 == (0x100 + 8 - 0x40));

    uint64_t val7 = 0;
    std::memcpy(&val7, synthBinary.Sections[0].Data.data() + 56, sizeof(uint64_t));
    assert(val7 == 0x7FFF0000ULL - 0x1038ULL);

    resolver.Clear();
    assert(resolver.GetSymbols().empty());
    assert(resolver.GetRelocations().empty());
    assert(resolver.GetImportedSymbols().empty());
    assert(resolver.GetTlsBaseAddress() == 0);

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
        0x48, 0x0F, 0xAF, 0xC1,
        0x48, 0x0F, 0xB6, 0xC1,
        0x48, 0x0F, 0xB7, 0xC1,
        0x48, 0x0F, 0xBE, 0xC1,
        0x48, 0x0F, 0xBF, 0xC1,
        0x48, 0x63, 0xC1,
        0x48, 0xD1, 0xE0,
        0x48, 0xC1, 0xE8, 0x02,
        0x48, 0xD3, 0xF8,
        0x48, 0xF7, 0xD0,
        0x48, 0xF7, 0xD8,
        0x48, 0x0F, 0x44, 0xC1,
        0x48, 0x0F, 0x45, 0xC1,
        0x48, 0x0F, 0x4C, 0xC1,
        0x48, 0x0F, 0x4D, 0xC1,
        0x0F, 0x94, 0xC0,
        0x0F, 0x95, 0xC0,
        0x0F, 0x9C, 0xC0,
        0x0F, 0x9D, 0xC0,
        0x48, 0x0F, 0xA3, 0xC8,
        0x48, 0x0F, 0xBA, 0xE0, 0x03,
        0x0F, 0xC8,
        0x48, 0x0F, 0xC8,
        0x48, 0x0F, 0xBC, 0xC1,
        0x48, 0x0F, 0xBD, 0xC1,
        0xAA,
        0xF3, 0x48, 0xAB,
        0xA4,
        0xF3, 0x48, 0xA5,
        0x99,
        0x48, 0x99,
        0xF7, 0xF1,
        0xF7, 0xF9,
        0x48, 0xF7, 0xF1,
        0x48, 0xF7, 0xF9,
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
