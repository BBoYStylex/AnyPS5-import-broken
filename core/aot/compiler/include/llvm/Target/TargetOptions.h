#ifndef CORE_AOT_COMPILER_LLVM_TARGET_TARGETOPTIONS_H
#define CORE_AOT_COMPILER_LLVM_TARGET_TARGETOPTIONS_H

namespace llvm {

class TargetOptions {
public:
    TargetOptions() = default;
};

namespace Reloc {
enum Model {
    Static,
    PIC_,
    DynamicNoPIC,
    ROPI,
    RWPI,
    ROPI_RWPI
};
} // namespace Reloc

enum class CodeGenOptLevel {
    None,
    Less,
    Default,
    Aggressive
};

namespace CodeGenFileType {
enum CodeGenFileTypeEnum {
    AssemblyFile,
    ObjectFile,
    CGFT_AssemblyFile = AssemblyFile,
    CGFT_ObjectFile = ObjectFile
};
} // namespace CodeGenFileType

} // namespace llvm
#endif
