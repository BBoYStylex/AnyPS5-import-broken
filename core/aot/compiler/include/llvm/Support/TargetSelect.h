#ifndef CORE_AOT_COMPILER_LLVM_SUPPORT_TARGETSELECT_H
#define CORE_AOT_COMPILER_LLVM_SUPPORT_TARGETSELECT_H

namespace llvm {

inline void InitializeAllTargetInfos() {}
inline void InitializeAllTargets() {}
inline void InitializeAllTargetMCs() {}
inline void InitializeAllAsmParsers() {}
inline void InitializeAllAsmPrinters() {}

} // namespace llvm
#endif
