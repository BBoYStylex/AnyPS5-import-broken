#ifndef CORE_AOT_COMPILER_LLVM_IR_LEGACYPASSMANAGER_H
#define CORE_AOT_COMPILER_LLVM_IR_LEGACYPASSMANAGER_H

namespace llvm {

class Module;

namespace legacy {

class PassManager {
public:
    PassManager() = default;
    bool run(Module& m) {
        static_cast<void>(m);
        return true;
    }
};

} // namespace legacy
} // namespace llvm
#endif
