#ifndef CORE_AOT_COMPILER_LLVM_IR_MODULE_H
#define CORE_AOT_COMPILER_LLVM_IR_MODULE_H

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <string_view>
#include <vector>

namespace llvm {

class DataLayout {
public:
    DataLayout() = default;
};

class Module {
public:
    Module(std::string_view name, LLVMContext& context)
        : moduleName(name), llvmContext(context) {}

    ~Module() {
        for (auto* fn : functionList) {
            delete fn;
        }
    }

    void setTargetTriple(std::string_view triple) { targetTriple = triple; }
    const std::string& getTargetTriple() const { return targetTriple; }
    LLVMContext& getContext() { return llvmContext; }

    void setDataLayout(const DataLayout& dl) { static_cast<void>(dl); }

    const std::string& getName() const { return moduleName; }
    const std::vector<Function*>& getFunctions() const { return functionList; }

    void print(raw_fd_ostream& os, void* annotator = nullptr) const {
        static_cast<void>(annotator);
        os << "; ModuleID = '" << moduleName << "'\n";
        os << "source_filename = \"" << moduleName << "\"\n";
        os << "target triple = \"" << targetTriple << "\"\n\n";

        for (const auto* fn : functionList) {
            if (fn->isDeclaration()) {
                os << "declare i64 @" << fn->getName() << "(...)\n\n";
                continue;
            }
            os << "define i64 @" << fn->getName() << "() {\n";
            for (const auto* bb : fn->getBasicBlocks()) {
                os << bb->getName() << ":\n";
                for (const auto* inst : bb->getInstructions()) {
                    os << "  " << inst->getAsString() << "\n";
                }
            }
            os << "}\n\n";
        }
    }

    FunctionCallee getOrInsertFunction(std::string_view name, FunctionType* type) {
        for (auto* fn : functionList) {
            if (fn->getName() == name) {
                return FunctionCallee(fn);
            }
        }
        auto* fn = Function::Create(type, ExternalLinkage, name, *this);
        return FunctionCallee(fn);
    }

    void addFunction(Function* fn) {
        functionList.push_back(fn);
    }

    struct FunctionIterator {
        std::vector<Function*>::iterator it;
        Function& operator*() const { return **it; }
        Function* operator->() const { return *it; }
        FunctionIterator& operator++() { ++it; return *this; }
        bool operator!=(const FunctionIterator& other) const { return it != other.it; }
    };

    FunctionIterator begin() { return FunctionIterator{functionList.begin()}; }
    FunctionIterator end() { return FunctionIterator{functionList.end()}; }

private:
    std::string moduleName;
    std::string targetTriple;
    LLVMContext& llvmContext;
    std::vector<Function*> functionList;
};

inline Function* Function::Create(FunctionType* type, LinkageTypes linkage, std::string_view name, Module& module) {
    auto* fn = new Function(type, linkage, name, &module);
    module.addFunction(fn);
    return fn;
}

} // namespace llvm
#endif
