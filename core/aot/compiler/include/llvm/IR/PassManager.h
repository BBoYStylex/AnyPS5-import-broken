#ifndef CORE_AOT_COMPILER_LLVM_IR_PASSMANAGER_H
#define CORE_AOT_COMPILER_LLVM_IR_PASSMANAGER_H

#include <llvm/IR/Module.h>

namespace llvm {

template <typename T>
class PassInfoMixin {};

class CFGAnalyses {};

class PreservedAnalyses {
public:
    static PreservedAnalyses all() {
        PreservedAnalyses pa;
        pa.preserved = true;
        return pa;
    }

    static PreservedAnalyses none() {
        PreservedAnalyses pa;
        pa.preserved = false;
        return pa;
    }

    template <typename T>
    void preserveSet() {
        preserved = false;
    }

    bool areAllPreserved() const {
        return preserved;
    }

private:
    bool preserved = true;
};

class LoopAnalysisManager {};
class FunctionAnalysisManager {};
class CGSCCAnalysisManager {};
class ModuleAnalysisManager {};

} // namespace llvm
#endif
