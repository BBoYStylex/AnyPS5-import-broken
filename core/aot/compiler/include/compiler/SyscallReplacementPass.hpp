#ifndef CORE_AOT_COMPILER_SYSCALLREPLACEMENTPASS_HPP
#define CORE_AOT_COMPILER_SYSCALLREPLACEMENTPASS_HPP

#include <llvm/IR/PassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <compiler/SyscallRegistry.hpp>

namespace Aot::Compiler {

class SyscallReplacementPass : public llvm::PassInfoMixin<SyscallReplacementPass> {
public:
    llvm::PreservedAnalyses run(llvm::Module& module, llvm::ModuleAnalysisManager& analysisManager);

private:
    bool processFunction(llvm::Function& function, llvm::Module& module);
    bool processCallInstruction(llvm::CallInst* callInst, llvm::Module& module);
    llvm::FunctionCallee getOrInsertNativeCall(llvm::Module& module, const SyscallSignature& signature);
    llvm::FunctionCallee getOrInsertFallbackHandler(llvm::Module& module);
};

} // namespace Aot::Compiler
#endif
