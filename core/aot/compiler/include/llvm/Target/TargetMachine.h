#ifndef CORE_AOT_COMPILER_LLVM_TARGET_TARGETMACHINE_H
#define CORE_AOT_COMPILER_LLVM_TARGET_TARGETMACHINE_H

#include <llvm/Target/TargetOptions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <memory>
#include <optional>

namespace llvm {

class Module;

class TargetMachine {
public:
    virtual ~TargetMachine() = default;

    DataLayout createDataLayout() const {
        return DataLayout{};
    }

    virtual bool addPassesToEmitFile(
        legacy::PassManager& pm,
        raw_fd_ostream& out,
        raw_pwrite_stream* dwoOut,
        CodeGenFileType::CodeGenFileTypeEnum fileType
    ) {
        static_cast<void>(dwoOut);
        pm.setOutputStream(&out);
        pm.setFileType(fileType);
        return false;
    }
};

} // namespace llvm
#endif
