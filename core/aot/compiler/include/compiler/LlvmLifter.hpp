#ifndef CORE_AOT_COMPILER_LLVMLIFTER_HPP
#define CORE_AOT_COMPILER_LLVMLIFTER_HPP

#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <compiler/ElfBinaryParser.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <span>
#include <vector>

namespace Aot::Compiler {

struct LiftedModuleResult {
    std::unique_ptr<llvm::LLVMContext> Context;
    std::unique_ptr<llvm::Module> Module;
    size_t LiftedFunctionCount;
    size_t LiftedInstructionCount;
};

class LlvmLifter {
public:
    LlvmLifter();
    ~LlvmLifter();

    LlvmLifter(const LlvmLifter&) = delete;
    LlvmLifter& operator=(const LlvmLifter&) = delete;

    LiftedModuleResult LiftBinary(const ParsedElfBinary& binary, std::string_view moduleName);
    bool LiftSection(const ElfSection& section, llvm::Module& module);

private:
    llvm::Function* liftFunctionAtAddress(
        const uint8_t* codeBytes,
        size_t byteCount,
        uint64_t virtualAddress,
        std::string_view name,
        llvm::Module& module
    );
};

} // namespace Aot::Compiler
#endif
