#ifndef CORE_AOT_COMPILER_LLVM_IR_IRBUILDER_H
#define CORE_AOT_COMPILER_LLVM_IR_IRBUILDER_H

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include <vector>

namespace llvm {

template <typename T = void>
class IRBuilder {
public:
    IRBuilder(BasicBlock* bb) : currentBlock(bb) {}
    IRBuilder(BasicBlock* bb, BasicBlock::InstructionIterator it) : currentBlock(bb) { static_cast<void>(it); }
    IRBuilder(Instruction* inst) : currentBlock(inst ? inst->getParent() : nullptr) {}

    Type* getInt64Ty() { static Type t; return &t; }
    Type* getInt32Ty() { static Type t; return &t; }
    Type* getInt16Ty() { static Type t; return &t; }
    Type* getInt8Ty() { static Type t; return &t; }
    Type* getInt1Ty() { static Type t; return &t; }
    Type* getFloatTy() { static Type t; return &t; }
    Type* getDoubleTy() { static Type t; return &t; }
    Type* getVectorTy() { static Type t; return &t; }
    Type* getVoidTy() { static Type t; return &t; }

    Value* getInt64(uint64_t val) { return new ConstantInt(getInt64Ty(), val); }
    Value* getInt32(uint32_t val) { return new ConstantInt(getInt32Ty(), val); }
    Value* getInt1(bool val) { return new ConstantInt(getInt1Ty(), val ? 1 : 0); }

    Value* CreateRet(Value* val) {
        static_cast<void>(val);
        auto* inst = new Instruction(getVoidTy(), nullptr, "ret i64 %rax");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateBr(BasicBlock* dest) {
        auto* inst = new Instruction(getVoidTy(), nullptr, dest ? "br label %" + dest->getName() : "br label %target");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateCondBr(Value* cond, BasicBlock* trueBB, BasicBlock* falseBB) {
        static_cast<void>(cond);
        std::string target = "br i1 %cond, label %" + (trueBB ? trueBB->getName() : "t") + ", label %" + (falseBB ? falseBB->getName() : "f");
        auto* inst = new Instruction(getVoidTy(), nullptr, target);
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    void SetInsertPoint(BasicBlock* bb) {
        currentBlock = bb;
    }

    BasicBlock* GetInsertBlock() const {
        return currentBlock;
    }

    Value* CreateTrunc(Value* val, Type* destTy) {
        static_cast<void>(val);
        auto* inst = new Instruction(destTy, nullptr, "%trunc = trunc i64 %val to i32");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateZExt(Value* val, Type* destTy) {
        static_cast<void>(val);
        auto* inst = new Instruction(destTy, nullptr, "%zext = zext i32 %val to i64");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateSExt(Value* val, Type* destTy) {
        static_cast<void>(val);
        auto* inst = new Instruction(destTy, nullptr, "%sext = sext i32 %val to i64");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateZExtOrTrunc(Value* val, Type* destTy) {
        static_cast<void>(val);
        auto* inst = new Instruction(destTy, nullptr, "%cast = zext/trunc %val");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    AllocaInst* CreateAlloca(Type* ty, Value* arraySize = nullptr, std::string_view name = "") {
        static_cast<void>(arraySize);
        auto* inst = new AllocaInst(ty, name);
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateLoad(Type* ty, Value* ptr) {
        static_cast<void>(ptr);
        auto* inst = new Instruction(ty, nullptr, "%load = load i64, ptr %ptr");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateStore(Value* val, Value* ptr) {
        static_cast<void>(val);
        static_cast<void>(ptr);
        auto* inst = new Instruction(getVoidTy(), nullptr, "store i64 %val, ptr %ptr");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    CallInst* CreateCall(FunctionCallee callee, const std::vector<Value*>& args = {}) {
        auto* inst = new CallInst(callee.getCallee(), args);
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    CallInst* CreateCall(Value* callee, const std::vector<Value*>& args = {}) {
        auto* inst = new CallInst(callee, args);
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateAdd(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "add"); }
    Value* CreateSub(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "sub"); }
    Value* CreateMul(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "mul"); }
    Value* CreateXor(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "xor"); }
    Value* CreateAnd(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "and"); }
    Value* CreateOr(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "or"); }
    Value* CreateShl(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "shl"); }
    Value* CreateLShr(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "lshr"); }
    Value* CreateAShr(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "ashr"); }
    Value* CreateUDiv(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "udiv"); }
    Value* CreateSDiv(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "sdiv"); }
    Value* CreateURem(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "urem"); }
    Value* CreateSRem(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "srem"); }
    Value* CreateFAdd(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "fadd"); }
    Value* CreateFSub(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "fsub"); }
    Value* CreateFMul(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "fmul"); }
    Value* CreateFDiv(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "fdiv"); }
    Value* CreateFSqrt(Value* val) {
        auto* inst = new Instruction(val ? val->getType() : getVectorTy(), nullptr, "%sqrt = call @llvm.sqrt(%val)");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }
    Value* CreateFMin(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "fmin"); }
    Value* CreateFMax(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs, "fmax"); }
    Value* CreateICmpEQ(Value* lhs, Value* rhs) { return makeCmpOp(lhs, rhs, "icmp eq"); }
    Value* CreateICmpNE(Value* lhs, Value* rhs) { return makeCmpOp(lhs, rhs, "icmp ne"); }
    Value* CreateICmpULT(Value* lhs, Value* rhs) { return makeCmpOp(lhs, rhs, "icmp ult"); }
    Value* CreateICmpSLT(Value* lhs, Value* rhs) { return makeCmpOp(lhs, rhs, "icmp slt"); }

    Value* CreateSelect(Value* cond, Value* trueVal, Value* falseVal) {
        static_cast<void>(cond);
        static_cast<void>(falseVal);
        auto* inst = new Instruction(trueVal ? trueVal->getType() : getInt64Ty(), nullptr, "%sel = select i1 %cond, %t, %f");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateNot(Value* val) {
        auto* inst = new Instruction(val ? val->getType() : getInt64Ty(), nullptr, "%not = xor i64 %val, -1");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateNeg(Value* val) {
        auto* inst = new Instruction(val ? val->getType() : getInt64Ty(), nullptr, "%neg = sub i64 0, %val");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateBSwap(Value* val) {
        auto* inst = new Instruction(val ? val->getType() : getInt64Ty(), nullptr, "%bswap = call @llvm.bswap(%val)");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateCTTZ(Value* val) {
        auto* inst = new Instruction(val ? val->getType() : getInt64Ty(), nullptr, "%cttz = call @llvm.cttz(%val)");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateCTLZ(Value* val) {
        auto* inst = new Instruction(val ? val->getType() : getInt64Ty(), nullptr, "%ctlz = call @llvm.ctlz(%val)");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateCtPop(Value* val) {
        auto* inst = new Instruction(val ? val->getType() : getInt64Ty(), nullptr, "%ctpop = call @llvm.ctpop(%val)");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateMemSet(Value* dest, Value* val, Value* size, unsigned align) {
        static_cast<void>(dest);
        static_cast<void>(val);
        static_cast<void>(size);
        static_cast<void>(align);
        auto* inst = new Instruction(getVoidTy(), nullptr, "call @llvm.memset(%dest, %val, %size)");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateMemCpy(Value* dest, unsigned destAlign, Value* src, unsigned srcAlign, Value* size) {
        static_cast<void>(dest);
        static_cast<void>(destAlign);
        static_cast<void>(src);
        static_cast<void>(srcAlign);
        static_cast<void>(size);
        auto* inst = new Instruction(getVoidTy(), nullptr, "call @llvm.memcpy(%dest, %src, %size)");
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

private:
    Value* makeBinaryOp(Value* lhs, Value* rhs, std::string_view opName) {
        static_cast<void>(rhs);
        std::string desc = "%op = " + std::string(opName) + " i64 %lhs, %rhs";
        auto* inst = new Instruction(lhs ? lhs->getType() : getInt64Ty(), nullptr, desc);
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* makeCmpOp(Value* lhs, Value* rhs, std::string_view cmpName) {
        static_cast<void>(lhs);
        static_cast<void>(rhs);
        std::string desc = "%cmp = " + std::string(cmpName) + " %lhs, %rhs";
        auto* inst = new Instruction(getInt1Ty(), nullptr, desc);
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    BasicBlock* currentBlock = nullptr;
};

} // namespace llvm
#endif
