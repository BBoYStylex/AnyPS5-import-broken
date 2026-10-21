#ifndef CORE_AOT_COMPILER_LLVM_IR_INSTRUCTIONS_H
#define CORE_AOT_COMPILER_LLVM_IR_INSTRUCTIONS_H

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Constants.h>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>

namespace llvm {

class BasicBlock;
class Function;
class Module;

class StringRef : public std::string_view {
public:
    using std::string_view::string_view;
    StringRef(const std::string& s) : std::string_view(s) {}
    StringRef(const char* s) : std::string_view(s ? s : "") {}
    bool contains(std::string_view sub) const {
        return find(sub) != npos;
    }
};

class Type {
public:
    virtual ~Type() = default;

    static Type* getInt64Ty(LLVMContext& context) { static_cast<void>(context); static Type t; return &t; }
    static Type* getInt32Ty(LLVMContext& context) { static_cast<void>(context); static Type t; return &t; }
    static Type* getInt8Ty(LLVMContext& context) { static_cast<void>(context); static Type t; return &t; }
    static Type* getInt1Ty(LLVMContext& context) { static_cast<void>(context); static Type t; return &t; }
    static Type* getVoidTy(LLVMContext& context) { static_cast<void>(context); static Type t; return &t; }

    virtual bool isVoidTy() const { return false; }
};

class FunctionType : public Type {
public:
    static FunctionType* get(Type* returnType, const std::vector<Type*>& params, bool isVarArg) {
        static_cast<void>(isVarArg);
        static FunctionType ft;
        ft.retType = returnType;
        ft.paramTypes = params;
        return &ft;
    }

    Type* getReturnType() const { return retType; }
    const std::vector<Type*>& getParams() const { return paramTypes; }

private:
    Type* retType = nullptr;
    std::vector<Type*> paramTypes;
};

class Instruction : public Value {
public:
    Instruction(Type* t = nullptr, BasicBlock* parent = nullptr)
        : Value(t), parentBlock(parent) {}

    BasicBlock* getParent() const { return parentBlock; }
    void setParent(BasicBlock* bb) { parentBlock = bb; }
    void eraseFromParent();

private:
    BasicBlock* parentBlock = nullptr;
};

class AllocaInst : public Instruction {
public:
    AllocaInst(Type* allocatedType, std::string_view name = "")
        : Instruction(allocatedType), varName(name) {}

private:
    std::string varName;
};

class CallInst : public Instruction {
public:
    CallInst(Value* callee, const std::vector<Value*>& args)
        : Instruction(nullptr), calledOperand(callee), arguments(args) {}

    bool isInlineAsm() const;
    Value* getCalledOperand() const { return calledOperand; }
    bool arg_empty() const { return arguments.empty(); }
    size_t arg_size() const { return arguments.size(); }
    Value* getArgOperand(size_t index) const {
        return (index < arguments.size()) ? arguments[index] : nullptr;
    }

private:
    Value* calledOperand = nullptr;
    std::vector<Value*> arguments;
};

class BasicBlock {
public:
    BasicBlock(std::string_view name = "", Function* parent = nullptr)
        : blockName(name), parentFunction(parent) {}

    ~BasicBlock() {
        for (auto* inst : instructions) {
            delete inst;
        }
    }

    static BasicBlock* Create(LLVMContext& context, std::string_view name, Function* parent = nullptr);

    Function* getParent() const { return parentFunction; }

    void addInstruction(Instruction* inst) {
        inst->setParent(this);
        instructions.push_back(inst);
    }

    void removeInstruction(Instruction* inst) {
        auto it = std::find(instructions.begin(), instructions.end(), inst);
        if (it != instructions.end()) {
            instructions.erase(it);
        }
    }

    Instruction* getTerminator() const {
        return instructions.empty() ? nullptr : instructions.back();
    }

    struct InstructionIterator {
        std::vector<Instruction*>::iterator it;
        Instruction& operator*() const { return **it; }
        Instruction* operator->() const { return *it; }
        InstructionIterator& operator++() { ++it; return *this; }
        bool operator!=(const InstructionIterator& other) const { return it != other.it; }
    };

    InstructionIterator begin() { return InstructionIterator{instructions.begin()}; }
    InstructionIterator end() { return InstructionIterator{instructions.end()}; }

private:
    std::string blockName;
    Function* parentFunction = nullptr;
    std::vector<Instruction*> instructions;
};

inline void Instruction::eraseFromParent() {
    if (parentBlock != nullptr) {
        parentBlock->removeInstruction(this);
    }
}

enum LinkageTypes {
    ExternalLinkage,
    InternalLinkage
};

class Function : public Value {
public:
    static constexpr LinkageTypes ExternalLinkage = llvm::ExternalLinkage;
    static constexpr LinkageTypes InternalLinkage = llvm::InternalLinkage;

    Function(FunctionType* type, LinkageTypes linkage, std::string_view name, Module* parentMod = nullptr)
        : Value(type), funcType(type), funcLinkage(linkage), funcName(name), parentModule(parentMod) {}

    ~Function() {
        for (auto* bb : basicBlocks) {
            delete bb;
        }
    }

    static Function* Create(FunctionType* type, LinkageTypes linkage, std::string_view name, Module& module);

    const std::string& getName() const { return funcName; }
    bool isDeclaration() const { return basicBlocks.empty(); }

    void addBasicBlock(BasicBlock* bb) {
        basicBlocks.push_back(bb);
    }

    BasicBlock& getEntryBlock() {
        if (basicBlocks.empty()) {
            LLVMContext dummy;
            addBasicBlock(new BasicBlock("entry", this));
        }
        return *basicBlocks.front();
    }

    struct BasicBlockIterator {
        std::vector<BasicBlock*>::iterator it;
        BasicBlock& operator*() const { return **it; }
        BasicBlock* operator->() const { return *it; }
        BasicBlockIterator& operator++() { ++it; return *this; }
        bool operator!=(const BasicBlockIterator& other) const { return it != other.it; }
    };

    BasicBlockIterator begin() { return BasicBlockIterator{basicBlocks.begin()}; }
    BasicBlockIterator end() { return BasicBlockIterator{basicBlocks.end()}; }

private:
    FunctionType* funcType = nullptr;
    LinkageTypes funcLinkage;
    std::string funcName;
    Module* parentModule = nullptr;
    std::vector<BasicBlock*> basicBlocks;
};

inline BasicBlock* BasicBlock::Create(LLVMContext& context, std::string_view name, Function* parent) {
    static_cast<void>(context);
    auto* bb = new BasicBlock(name, parent);
    if (parent != nullptr) {
        parent->addBasicBlock(bb);
    }
    return bb;
}

class FunctionCallee {
public:
    FunctionCallee() = default;
    FunctionCallee(Function* fn) : callee(fn) {}
    Function* getCallee() const { return callee; }
    Type* getType() const { return callee ? callee->getType() : nullptr; }

private:
    Function* callee = nullptr;
};

} // namespace llvm
#endif
