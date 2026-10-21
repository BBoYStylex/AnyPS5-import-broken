#ifndef CORE_AOT_COMPILER_LLVM_IR_VERIFIER_H
#define CORE_AOT_COMPILER_LLVM_IR_VERIFIER_H

namespace llvm {

class Module;
class Function;

inline bool verifyModule(const Module&, void* = nullptr) { return false; }
inline bool verifyFunction(const Function&, void* = nullptr) { return false; }

} // namespace llvm
#endif
