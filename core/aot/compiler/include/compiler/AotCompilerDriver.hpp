#ifndef CORE_AOT_COMPILER_AOTCOMPILERDRIVER_HPP
#define CORE_AOT_COMPILER_AOTCOMPILERDRIVER_HPP

#include <compiler/ElfBinaryParser.hpp>
#include <compiler/LlvmLifter.hpp>
#include <compiler/SyscallReplacementPass.hpp>
#include <filesystem>
#include <string>
#include <string_view>
#include <optional>

namespace Aot::Compiler {

struct CompilerOptions {
    std::filesystem::path InputExecutablePath;
    std::filesystem::path OutputObjectPath;
    std::string TargetTriple;
    uint32_t OptimizationLevel = 2;
    bool EnableSyscallReplacement = true;
    bool VerboseLogging = false;
};

struct CompilationStatistics {
    size_t TotalSectionsParsed = 0;
    size_t CodeSectionsLifted = 0;
    size_t FunctionsLifted = 0;
    size_t InstructionsLifted = 0;
    size_t RelocationsApplied = 0;
    bool SyscallPassModified = false;
    bool Success = false;
};

class AotCompilerDriver {
public:
    AotCompilerDriver();
    ~AotCompilerDriver();

    CompilationStatistics Compile(const CompilerOptions& options);

private:
    bool emitObjectFile(
        llvm::Module& module,
        const std::filesystem::path& outputPath,
        const std::string& targetTriple,
        uint32_t optLevel
    );
};

} // namespace Aot::Compiler
#endif
