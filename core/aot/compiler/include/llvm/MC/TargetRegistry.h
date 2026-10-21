#ifndef CORE_AOT_COMPILER_LLVM_MC_TARGETREGISTRY_H
#define CORE_AOT_COMPILER_LLVM_MC_TARGETREGISTRY_H

#include <llvm/Target/TargetMachine.h>
#include <string>
#include <optional>

namespace llvm {

class Target {
public:
    virtual ~Target() = default;

    virtual TargetMachine* createTargetMachine(
        const std::string& triple,
        const std::string& cpu,
        const std::string& features,
        const TargetOptions& options,
        std::optional<Reloc::Model> rm,
        std::optional<int> codeModel = std::nullopt,
        CodeGenOptLevel optLevel = CodeGenOptLevel::Default
    ) const {
        static_cast<void>(triple);
        static_cast<void>(cpu);
        static_cast<void>(features);
        static_cast<void>(options);
        static_cast<void>(rm);
        static_cast<void>(codeModel);
        static_cast<void>(optLevel);
        return new TargetMachine();
    }
};

class TargetRegistry {
public:
    static const Target* lookupTarget(const std::string& triple, std::string& error) {
        static Target defaultTarget;
        if (triple.empty()) {
            error = "Empty target triple";
            return nullptr;
        }
        return &defaultTarget;
    }
};

} // namespace llvm
#endif
