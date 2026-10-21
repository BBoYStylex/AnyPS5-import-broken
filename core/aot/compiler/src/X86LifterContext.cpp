#include <compiler/X86LifterContext.hpp>

namespace Aot::Compiler {

static const char* RegisterNames[16] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};

X86LifterContext::X86LifterContext(llvm::Function* function, llvm::IRBuilder<>& builder)
    : flagZeroAlloca(nullptr), flagSignAlloca(nullptr), flagCarryAlloca(nullptr), flagOverflowAlloca(nullptr) {
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
    flagCarryAlloca = entryBuilder.CreateAlloca(i1Type, nullptr, "flag_cf");
    flagOverflowAlloca = entryBuilder.CreateAlloca(i1Type, nullptr, "flag_of");
    entryBuilder.CreateStore(entryBuilder.getInt1(false), flagZeroAlloca);
    entryBuilder.CreateStore(entryBuilder.getInt1(false), flagSignAlloca);
    entryBuilder.CreateStore(entryBuilder.getInt1(false), flagCarryAlloca);
    entryBuilder.CreateStore(entryBuilder.getInt1(false), flagOverflowAlloca);
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
        case X86AluOp::Add: {
            result = builder.CreateAdd(lhs, rhs);
            llvm::Value* carry = builder.CreateICmpULT(result, lhs);
            builder.CreateStore(carry, flagCarryAlloca);
            llvm::Value* lhsXorRes = builder.CreateXor(lhs, result);
            llvm::Value* rhsXorRes = builder.CreateXor(rhs, result);
            llvm::Value* ofMask = builder.CreateAnd(lhsXorRes, rhsXorRes);
            llvm::Value* overflow = builder.CreateICmpSLT(ofMask, builder.getInt64(0));
            builder.CreateStore(overflow, flagOverflowAlloca);
            break;
        }
        case X86AluOp::Sub:
        case X86AluOp::Cmp: {
            result = builder.CreateSub(lhs, rhs);
            llvm::Value* carry = builder.CreateICmpULT(lhs, rhs);
            builder.CreateStore(carry, flagCarryAlloca);
            llvm::Value* lhsXorRhs = builder.CreateXor(lhs, rhs);
            llvm::Value* lhsXorRes = builder.CreateXor(lhs, result);
            llvm::Value* ofMask = builder.CreateAnd(lhsXorRhs, lhsXorRes);
            llvm::Value* overflow = builder.CreateICmpSLT(ofMask, builder.getInt64(0));
            builder.CreateStore(overflow, flagOverflowAlloca);
            break;
        }
        case X86AluOp::Xor:
            result = builder.CreateXor(lhs, rhs);
            builder.CreateStore(builder.getInt1(false), flagCarryAlloca);
            builder.CreateStore(builder.getInt1(false), flagOverflowAlloca);
            break;
        case X86AluOp::And:
        case X86AluOp::Test:
            result = builder.CreateAnd(lhs, rhs);
            builder.CreateStore(builder.getInt1(false), flagCarryAlloca);
            builder.CreateStore(builder.getInt1(false), flagOverflowAlloca);
            break;
        case X86AluOp::Or:
            result = builder.CreateOr(lhs, rhs);
            builder.CreateStore(builder.getInt1(false), flagCarryAlloca);
            builder.CreateStore(builder.getInt1(false), flagOverflowAlloca);
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
        case X86AluOp::Not:
            result = builder.CreateNot(lhs);
            break;
        case X86AluOp::Neg: {
            result = builder.CreateNeg(lhs);
            llvm::Value* carry = builder.CreateICmpNE(lhs, builder.getInt64(0));
            builder.CreateStore(carry, flagCarryAlloca);
            llvm::Value* overflow = builder.CreateICmpEQ(lhs, builder.getInt64(static_cast<int64_t>(1ULL << 63)));
            builder.CreateStore(overflow, flagOverflowAlloca);
            break;
        }
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

llvm::Value* X86LifterContext::ReadCarryFlag(llvm::IRBuilder<>& builder) const {
    return builder.CreateLoad(builder.getInt1Ty(), flagCarryAlloca);
}

llvm::Value* X86LifterContext::ReadOverflowFlag(llvm::IRBuilder<>& builder) const {
    return builder.CreateLoad(builder.getInt1Ty(), flagOverflowAlloca);
}

} // namespace Aot::Compiler
