#include <compiler/SyscallReplacementPass.hpp>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Constants.h>
#include <vector>

namespace Aot::Compiler {

llvm::PreservedAnalyses SyscallReplacementPass::run(llvm::Module& module, llvm::ModuleAnalysisManager& analysisManager) {
    bool modified = false;

    for (llvm::Function& function : module) {
        if (!function.isDeclaration()) {
            modified |= processFunction(function, module);
        }
    }

    if (!modified) {
        return llvm::PreservedAnalyses::all();
    }

    llvm::PreservedAnalyses preservedAnalyses;
    preservedAnalyses.preserveSet<llvm::CFGAnalyses>();
    return preservedAnalyses;
}

bool SyscallReplacementPass::processFunction(llvm::Function& function, llvm::Module& module) {
    bool modified = false;
    std::vector<llvm::CallInst*> candidateCalls;

    for (llvm::BasicBlock& basicBlock : function) {
        for (llvm::Instruction& instruction : basicBlock) {
            if (auto* callInst = llvm::dyn_cast<llvm::CallInst>(&instruction)) {
                if (callInst->isInlineAsm()) {
                    auto* inlineAsm = llvm::cast<llvm::InlineAsm>(callInst->getCalledOperand());
                    llvm::StringRef asmString = inlineAsm->getAsmString();
                    if (asmString.contains("syscall") || asmString.contains("int 0x80")) {
                        candidateCalls.push_back(callInst);
                    }
                }
            }
        }
    }

    for (llvm::CallInst* callInst : candidateCalls) {
        modified |= processCallInstruction(callInst, module);
    }

    return modified;
}

bool SyscallReplacementPass::processCallInstruction(llvm::CallInst* callInst, llvm::Module& module) {
    if (callInst->arg_empty()) {
        return false;
    }

    uint32_t syscallId = 0;
    bool hasConstantId = false;
    unsigned int argIndexOffset = 0;

    for (unsigned int index = 0; index < callInst->arg_size(); ++index) {
        if (auto* constantInt = llvm::dyn_cast<llvm::ConstantInt>(callInst->getArgOperand(index))) {
            syscallId = static_cast<uint32_t>(constantInt->getZExtValue());
            hasConstantId = true;
            argIndexOffset = index + 1;
            break;
        }
    }

    llvm::IRBuilder<> builder(callInst);

    if (hasConstantId) {
        auto signatureOpt = SyscallRegistry::Instance().Find(syscallId);
        if (signatureOpt.has_value()) {
            llvm::FunctionCallee targetCallee = getOrInsertNativeCall(module, *signatureOpt);
            std::vector<llvm::Value*> forwardedArgs;

            for (unsigned int index = argIndexOffset; index < callInst->arg_size() && forwardedArgs.size() < signatureOpt->ParameterCount; ++index) {
                forwardedArgs.push_back(callInst->getArgOperand(index));
            }

            while (forwardedArgs.size() < signatureOpt->ParameterCount) {
                forwardedArgs.push_back(builder.getInt64(0));
            }

            llvm::CallInst* directCall = builder.CreateCall(targetCallee, forwardedArgs);
            if (!callInst->use_empty()) {
                if (callInst->getType() == directCall->getType()) {
                    callInst->replaceAllUsesWith(directCall);
                } else if (!callInst->getType()->isVoidTy()) {
                    llvm::Value* castValue = builder.CreateZExtOrTrunc(directCall, callInst->getType());
                    callInst->replaceAllUsesWith(castValue);
                }
            }

            callInst->eraseFromParent();
            return true;
        }
    }

    llvm::FunctionCallee fallbackCallee = getOrInsertFallbackHandler(module);
    llvm::Value* idVal = hasConstantId ? builder.getInt32(syscallId) : builder.CreateTrunc(callInst->getArgOperand(0), builder.getInt32Ty());
    llvm::CallInst* fallbackCall = builder.CreateCall(fallbackCallee, {idVal});

    if (!callInst->use_empty()) {
        if (callInst->getType() == fallbackCall->getType()) {
            callInst->replaceAllUsesWith(fallbackCall);
        } else if (!callInst->getType()->isVoidTy()) {
            llvm::Value* castValue = builder.CreateZExtOrTrunc(fallbackCall, callInst->getType());
            callInst->replaceAllUsesWith(castValue);
        }
    }

    callInst->eraseFromParent();
    return true;
}

llvm::FunctionCallee SyscallReplacementPass::getOrInsertNativeCall(llvm::Module& module, const SyscallSignature& signature) {
    llvm::LLVMContext& context = module.getContext();
    llvm::Type* returnType = signature.ReturnsVoid ? llvm::Type::getVoidTy(context) : llvm::Type::getInt64Ty(context);

    std::vector<llvm::Type*> parameterTypes(signature.ParameterCount, llvm::Type::getInt64Ty(context));
    llvm::FunctionType* functionType = llvm::FunctionType::get(returnType, parameterTypes, false);

    return module.getOrInsertFunction(signature.NativeSymbol, functionType);
}

llvm::FunctionCallee SyscallReplacementPass::getOrInsertFallbackHandler(llvm::Module& module) {
    llvm::LLVMContext& context = module.getContext();
    llvm::Type* returnType = llvm::Type::getInt64Ty(context);
    llvm::FunctionType* functionType = llvm::FunctionType::get(returnType, {llvm::Type::getInt32Ty(context)}, false);

    return module.getOrInsertFunction("aps5_syscall_unimplemented_fallback", functionType);
}

} // namespace Aot::Compiler
