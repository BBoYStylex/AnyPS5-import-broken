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
        auto* inst = new Instruction(getVoidTy());
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateBr(BasicBlock* dest) {
        static_cast<void>(dest);
        auto* inst = new Instruction(getVoidTy());
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateCondBr(Value* cond, BasicBlock* trueBB, BasicBlock* falseBB) {
        static_cast<void>(cond);
        static_cast<void>(trueBB);
        static_cast<void>(falseBB);
        auto* inst = new Instruction(getVoidTy());
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
        auto* inst = new Instruction(destTy);
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateZExt(Value* val, Type* destTy) {
        static_cast<void>(val);
        auto* inst = new Instruction(destTy);
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateSExt(Value* val, Type* destTy) {
        static_cast<void>(val);
        auto* inst = new Instruction(destTy);
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateZExtOrTrunc(Value* val, Type* destTy) {
        static_cast<void>(val);
        auto* inst = new Instruction(destTy);
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
        auto* inst = new Instruction(ty);
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateStore(Value* val, Value* ptr) {
        static_cast<void>(val);
        static_cast<void>(ptr);
        auto* inst = new Instruction(getVoidTy());
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

    Value* CreateAdd(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs); }
    Value* CreateSub(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs); }
    Value* CreateMul(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs); }
    Value* CreateXor(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs); }
    Value* CreateAnd(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs); }
    Value* CreateOr(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs); }
    Value* CreateShl(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs); }
    Value* CreateLShr(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs); }
    Value* CreateAShr(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs); }
    Value* CreateFAdd(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs); }
    Value* CreateFSub(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs); }
    Value* CreateFMul(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs); }
    Value* CreateFDiv(Value* lhs, Value* rhs) { return makeBinaryOp(lhs, rhs); }
    Value* CreateICmpEQ(Value* lhs, Value* rhs) { return makeCmpOp(lhs, rhs); }
    Value* CreateICmpNE(Value* lhs, Value* rhs) { return makeCmpOp(lhs, rhs); }
    Value* CreateICmpULT(Value* lhs, Value* rhs) { return makeCmpOp(lhs, rhs); }
    Value* CreateICmpSLT(Value* lhs, Value* rhs) { return makeCmpOp(lhs, rhs); }

    Value* CreateSelect(Value* cond, Value* trueVal, Value* falseVal) {
        static_cast<void>(cond);
        static_cast<void>(falseVal);
        auto* inst = new Instruction(trueVal ? trueVal->getType() : getInt64Ty());
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateNot(Value* val) {
        auto* inst = new Instruction(val ? val->getType() : getInt64Ty());
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateNeg(Value* val) {
        auto* inst = new Instruction(val ? val->getType() : getInt64Ty());
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateBSwap(Value* val) {
        auto* inst = new Instruction(val ? val->getType() : getInt64Ty());
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateCTTZ(Value* val) {
        auto* inst = new Instruction(val ? val->getType() : getInt64Ty());
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateCTLZ(Value* val) {
        auto* inst = new Instruction(val ? val->getType() : getInt64Ty());
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateMemSet(Value* dest, Value* val, Value* size, unsigned align) {
        static_cast<void>(dest);
        static_cast<void>(val);
        static_cast<void>(size);
        static_cast<void>(align);
        auto* inst = new Instruction(getVoidTy());
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* CreateMemCpy(Value* dest, unsigned destAlign, Value* src, unsigned srcAlign, Value* size) {
        static_cast<void>(dest);
        static_cast<void>(destAlign);
        static_cast<void>(src);
        static_cast<void>(srcAlign);
        static_cast<void>(size);
        auto* inst = new Instruction(getVoidTy());
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

private:
    Value* makeBinaryOp(Value* lhs, Value* rhs) {
        static_cast<void>(rhs);
        auto* inst = new Instruction(lhs ? lhs->getType() : getInt64Ty());
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    Value* makeCmpOp(Value* lhs, Value* rhs) {
        static_cast<void>(lhs);
        static_cast<void>(rhs);
        auto* inst = new Instruction(getInt1Ty());
        if (currentBlock) currentBlock->addInstruction(inst);
        return inst;
    }

    BasicBlock* currentBlock = nullptr;
};

} // namespace llvm
#endif
