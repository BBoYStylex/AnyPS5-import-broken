#ifndef CORE_AOT_COMPILER_LLVM_PASSES_PASSBUILDER_H
#define CORE_AOT_COMPILER_LLVM_PASSES_PASSBUILDER_H

#include <llvm/IR/PassManager.h>

namespace llvm {

class PassBuilder {
public:
    void registerModuleAnalyses(ModuleAnalysisManager& mam) { static_cast<void>(mam); }
    void registerCGSCCAnalyses(CGSCCAnalysisManager& cgam) { static_cast<void>(cgam); }
    void registerFunctionAnalyses(FunctionAnalysisManager& fam) { static_cast<void>(fam); }
    void registerLoopAnalyses(LoopAnalysisManager& lam) { static_cast<void>(lam); }
    void crossRegisterProxies(LoopAnalysisManager& lam, FunctionAnalysisManager& fam, CGSCCAnalysisManager& cgam, ModuleAnalysisManager& mam) {
        static_cast<void>(lam);
        static_cast<void>(fam);
        static_cast<void>(cgam);
        static_cast<void>(mam);
    }
};

} // namespace llvm
#endif
