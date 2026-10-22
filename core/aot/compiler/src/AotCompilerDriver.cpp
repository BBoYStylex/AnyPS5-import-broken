#include <compiler/AotCompilerDriver.hpp>
#include <compiler/ElfRelocationResolver.hpp>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Passes/PassBuilder.h>
#include <iostream>

namespace Aot::Compiler {

AotCompilerDriver::AotCompilerDriver() = default;
AotCompilerDriver::~AotCompilerDriver() = default;

CompilationStatistics AotCompilerDriver::Compile(const CompilerOptions& options) {
    CompilationStatistics stats{};

    ElfBinaryParser parser;
    auto binaryOpt = parser.ParseFromFile(options.InputExecutablePath);
    if (!binaryOpt.has_value()) {
        return stats;
    }

    ParsedElfBinary binary = *binaryOpt;
    stats.TotalSectionsParsed = binary.Sections.size();

    ElfRelocationResolver relocationResolver;
    relocationResolver.LoadSymbolsAndRelocations(binary, {});
    stats.RelocationsApplied = relocationResolver.ApplyRelocations(binary, binary.BaseVirtualAddress);

    LlvmLifter lifter;
    std::string moduleName = options.InputExecutablePath.stem().string();
    LiftedModuleResult lifted = lifter.LiftBinary(binary, moduleName);

    stats.FunctionsLifted = lifted.LiftedFunctionCount;
    stats.InstructionsLifted = lifted.LiftedInstructionCount;

    if (lifted.Module == nullptr) {
        return stats;
    }

    if (options.EnableSyscallReplacement) {
        llvm::LoopAnalysisManager lam;
        llvm::FunctionAnalysisManager fam;
        llvm::CGSCCAnalysisManager cgam;
        llvm::ModuleAnalysisManager mam;

        llvm::PassBuilder passBuilder;
        passBuilder.registerModuleAnalyses(mam);
        passBuilder.registerCGSCCAnalyses(cgam);
        passBuilder.registerFunctionAnalyses(fam);
        passBuilder.registerLoopAnalyses(lam);
        passBuilder.crossRegisterProxies(lam, fam, cgam, mam);

        SyscallReplacementPass syscallPass;
        llvm::PreservedAnalyses pa = syscallPass.run(*lifted.Module, mam);
        stats.SyscallPassModified = !pa.areAllPreserved();
    }

    std::string targetTriple = options.TargetTriple;
    if (targetTriple.empty()) {
#if defined(_WIN32)
        targetTriple = "x86_64-pc-windows-msvc";
#else
        targetTriple = "x86_64-unknown-linux-gnu";
#endif
    }

    if (options.EmitLlvmIr && !options.OutputLlvmIrPath.empty()) {
        if (options.OutputLlvmIrPath.has_parent_path()) {
            std::filesystem::create_directories(options.OutputLlvmIrPath.parent_path());
        }
        std::error_code errorCode;
        llvm::raw_fd_ostream irStream(options.OutputLlvmIrPath.string(), errorCode, llvm::sys::fs::OF_None);
        if (!errorCode) {
            lifted.Module->print(irStream, nullptr);
            irStream.flush();
        }
    }

    if (options.EmitAssembly && !options.OutputAssemblyPath.empty()) {
        emitAssemblyFile(*lifted.Module, options.OutputAssemblyPath, targetTriple, options.OptimizationLevel);
    }

    stats.Success = emitObjectFile(
        *lifted.Module,
        options.OutputObjectPath,
        targetTriple,
        options.OptimizationLevel
    );

    if (stats.Success && options.EmitExecutable && !options.OutputExecutablePath.empty()) {
        stats.Success = LinkExecutable(
            options.OutputObjectPath,
            options.OutputExecutablePath,
            options.AdditionalLibraries
        );
    }

    return stats;
}

bool AotCompilerDriver::LinkExecutable(
    const std::filesystem::path& objectPath,
    const std::filesystem::path& executablePath,
    const std::vector<std::filesystem::path>& additionalLibraries) {
    if (!std::filesystem::exists(objectPath)) {
        return false;
    }

    if (executablePath.has_parent_path()) {
        std::filesystem::create_directories(executablePath.parent_path());
    }

    std::string command;
#if defined(_WIN32)
    command = "lld-link /NOLOGO /SUBSYSTEM:CONSOLE /OUT:\"" + executablePath.string() + "\" \"" + objectPath.string() + "\"";
    for (const auto& lib : additionalLibraries) {
        command += " \"" + lib.string() + "\"";
    }
#else
    command = "clang++ -o \"" + executablePath.string() + "\" \"" + objectPath.string() + "\"";
    for (const auto& lib : additionalLibraries) {
        command += " \"" + lib.string() + "\"";
    }
#endif

    int result = std::system(command.c_str());
    return result == 0;
}

bool AotCompilerDriver::emitObjectFile(
    llvm::Module& module,
    const std::filesystem::path& outputPath,
    const std::string& targetTriple,
    uint32_t optLevel) {
    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (target == nullptr) {
        return false;
    }

    llvm::CodeGenOptLevel codegenOpt = llvm::CodeGenOptLevel::Default;
    if (optLevel == 0) {
        codegenOpt = llvm::CodeGenOptLevel::None;
    } else if (optLevel == 1) {
        codegenOpt = llvm::CodeGenOptLevel::Less;
    } else if (optLevel >= 3) {
        codegenOpt = llvm::CodeGenOptLevel::Aggressive;
    }

    llvm::TargetOptions targetOptions;
    std::unique_ptr<llvm::TargetMachine> targetMachine(
        target->createTargetMachine(
            targetTriple,
            "generic",
            "",
            targetOptions,
            std::optional<llvm::Reloc::Model>(llvm::Reloc::PIC_),
            std::nullopt,
            codegenOpt
        )
    );

    if (!targetMachine) {
        return false;
    }

    module.setDataLayout(targetMachine->createDataLayout());
    module.setTargetTriple(targetTriple);

    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }

    std::error_code errorCode;
    llvm::raw_fd_ostream destinationFile(
        outputPath.string(),
        errorCode,
        llvm::sys::fs::OF_None
    );

    if (errorCode) {
        return false;
    }

    llvm::legacy::PassManager passManager;
    if (targetMachine->addPassesToEmitFile(passManager, destinationFile, nullptr, llvm::CodeGenFileType::ObjectFile)) {
        return false;
    }

    if (!passManager.run(module)) {
        return false;
    }
    destinationFile.flush();

    return true;
}

bool AotCompilerDriver::emitAssemblyFile(
    llvm::Module& module,
    const std::filesystem::path& outputPath,
    const std::string& targetTriple,
    uint32_t optLevel) {
    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (target == nullptr) {
        return false;
    }

    llvm::CodeGenOptLevel codegenOpt = llvm::CodeGenOptLevel::Default;
    if (optLevel == 0) {
        codegenOpt = llvm::CodeGenOptLevel::None;
    } else if (optLevel == 1) {
        codegenOpt = llvm::CodeGenOptLevel::Less;
    } else if (optLevel >= 3) {
        codegenOpt = llvm::CodeGenOptLevel::Aggressive;
    }

    llvm::TargetOptions targetOptions;
    std::unique_ptr<llvm::TargetMachine> targetMachine(
        target->createTargetMachine(
            targetTriple,
            "generic",
            "",
            targetOptions,
            std::optional<llvm::Reloc::Model>(llvm::Reloc::PIC_),
            std::nullopt,
            codegenOpt
        )
    );

    if (!targetMachine) {
        return false;
    }

    module.setDataLayout(targetMachine->createDataLayout());
    module.setTargetTriple(targetTriple);

    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }

    std::error_code errorCode;
    llvm::raw_fd_ostream destinationFile(
        outputPath.string(),
        errorCode,
        llvm::sys::fs::OF_None
    );

    if (errorCode) {
        return false;
    }

    llvm::legacy::PassManager passManager;
    if (targetMachine->addPassesToEmitFile(passManager, destinationFile, nullptr, llvm::CodeGenFileType::AssemblyFile)) {
        return false;
    }

    if (!passManager.run(module)) {
        return false;
    }
    destinationFile.flush();

    return true;
}

} // namespace Aot::Compiler
