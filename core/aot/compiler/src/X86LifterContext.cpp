#include <compiler/X86LifterContext.hpp>

namespace Aot::Compiler {

static const char* RegisterNames[16] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};

X86LifterContext::X86LifterContext(llvm::Function* function, llvm::IRBuilder<>& builder)
    : flagZeroAlloca(nullptr), flagSignAlloca(nullptr) {
    llvm::BasicBlock* entryBlock = &function->getEntryBlock();
    llvm::IRBuilder<> entryBuilder(entryBlock, entryBlock->begin());

    llvm::Type* i64Type = entryBuilder.getInt64Ty();
    llvm::Type* i1Type = entryBuilder.getInt1Ty();

    for (size_t i = 0; i < registerAllocas.size(); ++i) {
        registerAllocas[i] = entryBuilder.CreateAlloca(i64Type, nullptr, RegisterNames[i]);
        entryBuilder.CreateStore(entryBuilder.getInt64(0), registerAllocas[i]);
    }

    flagZeroAlloca = entryBuilder.CreateAlloca(i1Type, nullptr, "flag_zf");
    flagSignAlloca = entryBuilder.CreateAlloca(i1Type, nullptr, "flag_sf");
    entryBuilder.CreateStore(entryBuilder.getInt1(false), flagZeroAlloca);
    entryBuilder.CreateStore(entryBuilder.getInt1(false), flagSignAlloca);
}

llvm::Value* X86LifterContext::ReadRegister(X86Register reg, llvm::IRBuilder<>& builder) const {
    size_t index = static_cast<size_t>(reg);
    return builder.CreateLoad(builder.getInt64Ty(), registerAllocas[index]);
}

void X86LifterContext::WriteRegister(X86Register reg, llvm::Value* value, llvm::IRBuilder<>& builder) {
    size_t index = static_cast<size_t>(reg);
    if (value->getType() != builder.getInt64Ty()) {
        value = builder.CreateZExtOrTrunc(value, builder.getInt64Ty());
    }
    builder.CreateStore(value, registerAllocas[index]);
}

llvm::Value* X86LifterContext::EvaluateAlu(
    X86AluOp op,
    llvm::Value* lhs,
    llvm::Value* rhs,
    llvm::IRBuilder<>& builder) {
    llvm::Type* i64 = builder.getInt64Ty();
    if (lhs->getType() != i64) lhs = builder.CreateZExtOrTrunc(lhs, i64);
    if (rhs->getType() != i64) rhs = builder.CreateZExtOrTrunc(rhs, i64);

    llvm::Value* result = nullptr;
    switch (op) {
        case X86AluOp::Add:
            result = builder.CreateAdd(lhs, rhs);
            break;
        case X86AluOp::Sub:
            result = builder.CreateSub(lhs, rhs);
            break;
        case X86AluOp::Xor:
            result = builder.CreateXor(lhs, rhs);
            break;
        case X86AluOp::And:
            result = builder.CreateAnd(lhs, rhs);
            break;
        case X86AluOp::Or:
            result = builder.CreateOr(lhs, rhs);
            break;
        case X86AluOp::Shl:
            result = builder.CreateShl(lhs, rhs);
            break;
        case X86AluOp::Shr:
            result = builder.CreateLShr(lhs, rhs);
            break;
        case X86AluOp::Sar:
            result = builder.CreateAShr(lhs, rhs);
            break;
    }

    UpdateFlags(result, builder);
    return result;
}

void X86LifterContext::UpdateFlags(llvm::Value* result, llvm::IRBuilder<>& builder) {
    llvm::Value* isZero = builder.CreateICmpEQ(result, builder.getInt64(0));
    llvm::Value* isNegative = builder.CreateICmpSLT(result, builder.getInt64(0));

    builder.CreateStore(isZero, flagZeroAlloca);
    builder.CreateStore(isNegative, flagSignAlloca);
}

llvm::Value* X86LifterContext::ReadZeroFlag(llvm::IRBuilder<>& builder) const {
    return builder.CreateLoad(builder.getInt1Ty(), flagZeroAlloca);
}

llvm::Value* X86LifterContext::ReadSignFlag(llvm::IRBuilder<>& builder) const {
    return builder.CreateLoad(builder.getInt1Ty(), flagSignAlloca);
}

} // namespace Aot::Compiler
