#include <compiler/AotCompilerDriver.hpp>
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

    const ParsedElfBinary& binary = *binaryOpt;
    stats.TotalSectionsParsed = binary.Sections.size();

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

    stats.Success = emitObjectFile(
        *lifted.Module,
        options.OutputObjectPath,
        targetTriple,
        options.OptimizationLevel
    );

    return stats;
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

    passManager.run(module);
    destinationFile.flush();

    return true;
}

} // namespace Aot::Compiler
