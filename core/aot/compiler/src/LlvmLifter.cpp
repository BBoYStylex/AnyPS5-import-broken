#include <compiler/LlvmLifter.hpp>
#include <compiler/X86LifterContext.hpp>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>

namespace Aot::Compiler {

LlvmLifter::LlvmLifter() {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
}

LlvmLifter::~LlvmLifter() = default;

LiftedModuleResult LlvmLifter::LiftBinary(const ParsedElfBinary& binary, std::string_view moduleName) {
    auto context = std::make_unique<llvm::LLVMContext>();
    auto module = std::make_unique<llvm::Module>(moduleName, *context);

#if defined(_WIN32)
    module->setTargetTriple("x86_64-pc-windows-msvc");
#else
    module->setTargetTriple("x86_64-unknown-linux-gnu");
#endif

    size_t functionCount = 0;
    size_t instructionCount = 0;

    for (const auto& section : binary.Sections) {
        if ((section.Flags & 0x4) != 0 && !section.Data.empty()) {
            if (LiftSection(section, *module)) {
                ++functionCount;
                instructionCount += section.Data.size();
            }
        }
    }

    return LiftedModuleResult{
        .Context = std::move(context),
        .Module = std::move(module),
        .LiftedFunctionCount = functionCount,
        .LiftedInstructionCount = instructionCount
    };
}

bool LlvmLifter::LiftSection(const ElfSection& section, llvm::Module& module) {
    if (section.Data.empty()) {
        return false;
    }

    std::string funcName = "sub_" + section.Name;
    for (char& c : funcName) {
        if (c == '.' || c == '-') {
            c = '_';
        }
    }

    llvm::Function* function = liftFunctionAtAddress(
        section.Data.data(),
        section.Data.size(),
        section.VirtualAddress,
        funcName,
        module
    );

    return function != nullptr;
}

llvm::Function* LlvmLifter::liftFunctionAtAddress(
    const uint8_t* codeBytes,
    size_t byteCount,
    uint64_t virtualAddress,
    std::string_view name,
    llvm::Module& module) {
    static_cast<void>(virtualAddress);
    llvm::LLVMContext& context = module.getContext();

    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::Type::getInt64Ty(context),
        {llvm::Type::getInt64Ty(context)},
        false
    );

    llvm::Function* function = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        name,
        module
    );

    llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(context, "entry", function);
    llvm::IRBuilder<> builder(entryBlock);
    X86LifterContext lifterContext(function, builder);

    size_t offset = 0;
    while (offset < byteCount) {
        uint8_t byte = codeBytes[offset];

        if (byte == 0x90) {
            offset += 1;
        } else if (byte == 0x0F && (offset + 1 < byteCount) && codeBytes[offset + 1] == 0x05) {
            llvm::FunctionType* asmType = llvm::FunctionType::get(
                llvm::Type::getInt64Ty(context),
                {llvm::Type::getInt32Ty(context)},
                false
            );
            llvm::InlineAsm* inlineAsm = llvm::InlineAsm::get(
                asmType,
                "syscall",
                "={rax},{rax},~{rcx},~{r11},~{memory}",
                true
            );
            llvm::Value* raxVal = lifterContext.ReadRegister(X86Register::Rax, builder);
            llvm::Value* sysId = builder.CreateTrunc(raxVal, builder.getInt32Ty());
            llvm::Value* result = builder.CreateCall(inlineAsm, {sysId});
            lifterContext.WriteRegister(X86Register::Rax, result, builder);
            offset += 2;
        } else if (byte == 0x48 && (offset + 2 < byteCount) && codeBytes[offset + 1] == 0x31 && codeBytes[offset + 2] == 0xC0) {
            llvm::Value* raxVal = lifterContext.ReadRegister(X86Register::Rax, builder);
            llvm::Value* res = lifterContext.EvaluateAlu(X86AluOp::Xor, raxVal, raxVal, builder);
            lifterContext.WriteRegister(X86Register::Rax, res, builder);
            offset += 3;
        } else if (byte == 0xC3) {
            llvm::Value* retVal = lifterContext.ReadRegister(X86Register::Rax, builder);
            builder.CreateRet(retVal);
            offset += 1;
            break;
        } else {
            offset += 1;
        }
    }

    if (entryBlock->getTerminator() == nullptr) {
        llvm::Value* retVal = lifterContext.ReadRegister(X86Register::Rax, builder);
        builder.CreateRet(retVal);
    }

    return function;
}

} // namespace Aot::Compiler
