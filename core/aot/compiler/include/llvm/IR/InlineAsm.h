#ifndef CORE_AOT_COMPILER_LLVM_IR_INLINEASM_H
#define CORE_AOT_COMPILER_LLVM_IR_INLINEASM_H

#include <llvm/IR/Instructions.h>
#include <string>
#include <string_view>

namespace llvm {

class InlineAsm : public Value {
public:
    InlineAsm(std::string_view asmStr, std::string_view constraints, bool hasSideEffects)
        : Value(nullptr), asmString(asmStr), constraintString(constraints), sideEffects(hasSideEffects) {}

    static InlineAsm* get(FunctionType* type, std::string_view asmStr, std::string_view constraints, bool hasSideEffects) {
        static_cast<void>(type);
        return new InlineAsm(asmStr, constraints, hasSideEffects);
    }

    std::string getAsmString() const { return asmString; }
    std::string getConstraintString() const { return constraintString; }

private:
    std::string asmString;
    std::string constraintString;
    bool sideEffects = false;
};

inline bool CallInst::isInlineAsm() const {
    return dynamic_cast<const InlineAsm*>(calledOperand) != nullptr;
}

inline std::string CallInst::getAsString() const {
    if (isInlineAsm()) {
        auto* inlineAsm = dynamic_cast<const InlineAsm*>(calledOperand);
        return "call void asm sideeffect \"" + (inlineAsm ? inlineAsm->getAsmString() : "") + "\"";
    }
    auto* fn = dynamic_cast<const Function*>(calledOperand);
    std::string s = "call i64 @" + (fn ? fn->getName() : "callee") + "(";
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) {
            s += ", ";
        }
        s += "i64 %arg" + std::to_string(i);
    }
    s += ")";
    return s;
}

} // namespace llvm
#endif
