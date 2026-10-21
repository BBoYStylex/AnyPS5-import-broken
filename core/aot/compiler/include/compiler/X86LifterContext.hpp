#ifndef CORE_AOT_COMPILER_X86LIFTERCONTEXT_HPP
#define CORE_AOT_COMPILER_X86LIFTERCONTEXT_HPP

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <cstdint>
#include <array>

namespace Aot::Compiler {

enum class X86Register : uint8_t {
    Rax = 0,
    Rcx = 1,
    Rdx = 2,
    Rbx = 3,
    Rsp = 4,
    Rbp = 5,
    Rsi = 6,
    Rdi = 7,
    R8 = 8,
    R9 = 9,
    R10 = 10,
    R11 = 11,
    R12 = 12,
    R13 = 13,
    R14 = 14,
    R15 = 15,
    Count = 16
};

enum class X86AluOp : uint8_t {
    Add,
    Sub,
    Xor,
    And,
    Or,
    Shl,
    Shr,
    Sar
};

class X86LifterContext {
public:
    explicit X86LifterContext(llvm::Function* function, llvm::IRBuilder<>& builder);

    [[nodiscard]] llvm::Value* ReadRegister(X86Register reg, llvm::IRBuilder<>& builder) const;
    void WriteRegister(X86Register reg, llvm::Value* value, llvm::IRBuilder<>& builder);

    [[nodiscard]] llvm::Value* EvaluateAlu(
        X86AluOp op,
        llvm::Value* lhs,
        llvm::Value* rhs,
        llvm::IRBuilder<>& builder
    );

    void UpdateFlags(llvm::Value* result, llvm::IRBuilder<>& builder);
    [[nodiscard]] llvm::Value* ReadZeroFlag(llvm::IRBuilder<>& builder) const;
    [[nodiscard]] llvm::Value* ReadSignFlag(llvm::IRBuilder<>& builder) const;

private:
    std::array<llvm::AllocaInst*, static_cast<size_t>(X86Register::Count)> registerAllocas;
    llvm::AllocaInst* flagZeroAlloca;
    llvm::AllocaInst* flagSignAlloca;
};

} // namespace Aot::Compiler
#endif
