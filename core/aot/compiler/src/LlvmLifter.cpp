#include <compiler/LlvmLifter.hpp>
#include <compiler/X86LifterContext.hpp>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <cstring>

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
            continue;
        }

        if (byte == 0xC3) {
            llvm::Value* retVal = lifterContext.ReadRegister(X86Register::Rax, builder);
            builder.CreateRet(retVal);
            offset += 1;
            break;
        }

        if (byte == 0x0F && (offset + 1 < byteCount) && codeBytes[offset + 1] == 0x05) {
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
            continue;
        }

        bool rexW = false;
        bool rexR = false;
        bool rexB = false;

        size_t currentOffset = offset;
        if ((byte & 0xF0) == 0x40) {
            rexW = (byte & 0x08) != 0;
            rexR = (byte & 0x04) != 0;
            rexB = (byte & 0x01) != 0;
            currentOffset++;
            if (currentOffset >= byteCount) {
                offset++;
                continue;
            }
            byte = codeBytes[currentOffset];
        }

        if ((byte & 0xF8) == 0xB8) {
            uint8_t regIndex = (byte & 0x07) + (rexB ? 8 : 0);
            if (rexW && currentOffset + 8 < byteCount) {
                uint64_t imm = 0;
                std::memcpy(&imm, &codeBytes[currentOffset + 1], sizeof(uint64_t));
                lifterContext.WriteRegister(static_cast<X86Register>(regIndex), builder.getInt64(imm), builder);
                offset = currentOffset + 9;
                continue;
            } else if (!rexW && currentOffset + 4 < byteCount) {
                uint32_t imm = 0;
                std::memcpy(&imm, &codeBytes[currentOffset + 1], sizeof(uint32_t));
                lifterContext.WriteRegister(static_cast<X86Register>(regIndex), builder.getInt64(imm), builder);
                offset = currentOffset + 5;
                continue;
            }
        }

        if (currentOffset + 1 < byteCount) {
            uint8_t modrm = codeBytes[currentOffset + 1];
            uint8_t mod = (modrm >> 6) & 0x03;
            uint8_t regField = ((modrm >> 3) & 0x07) + (rexR ? 8 : 0);
            uint8_t rmField = (modrm & 0x07) + (rexB ? 8 : 0);

            if (mod == 3) {
                auto regDst = static_cast<X86Register>(rmField);
                auto regSrc = static_cast<X86Register>(regField);

                if (byte == 0x89) {
                    llvm::Value* srcVal = lifterContext.ReadRegister(regSrc, builder);
                    lifterContext.WriteRegister(regDst, srcVal, builder);
                    offset = currentOffset + 2;
                    continue;
                } else if (byte == 0x8B) {
                    llvm::Value* srcVal = lifterContext.ReadRegister(regDst, builder);
                    lifterContext.WriteRegister(regSrc, srcVal, builder);
                    offset = currentOffset + 2;
                    continue;
                } else if (byte == 0x01 || byte == 0x03) {
                    auto dst = (byte == 0x01) ? regDst : regSrc;
                    auto src = (byte == 0x01) ? regSrc : regDst;
                    llvm::Value* v1 = lifterContext.ReadRegister(dst, builder);
                    llvm::Value* v2 = lifterContext.ReadRegister(src, builder);
                    llvm::Value* res = lifterContext.EvaluateAlu(X86AluOp::Add, v1, v2, builder);
                    lifterContext.WriteRegister(dst, res, builder);
                    offset = currentOffset + 2;
                    continue;
                } else if (byte == 0x29 || byte == 0x2B) {
                    auto dst = (byte == 0x29) ? regDst : regSrc;
                    auto src = (byte == 0x29) ? regSrc : regDst;
                    llvm::Value* v1 = lifterContext.ReadRegister(dst, builder);
                    llvm::Value* v2 = lifterContext.ReadRegister(src, builder);
                    llvm::Value* res = lifterContext.EvaluateAlu(X86AluOp::Sub, v1, v2, builder);
                    lifterContext.WriteRegister(dst, res, builder);
                    offset = currentOffset + 2;
                    continue;
                } else if (byte == 0x31 || byte == 0x33) {
                    auto dst = (byte == 0x31) ? regDst : regSrc;
                    auto src = (byte == 0x31) ? regSrc : regDst;
                    llvm::Value* v1 = lifterContext.ReadRegister(dst, builder);
                    llvm::Value* v2 = lifterContext.ReadRegister(src, builder);
                    llvm::Value* res = lifterContext.EvaluateAlu(X86AluOp::Xor, v1, v2, builder);
                    lifterContext.WriteRegister(dst, res, builder);
                    offset = currentOffset + 2;
                    continue;
                } else if (byte == 0x21 || byte == 0x23) {
                    auto dst = (byte == 0x21) ? regDst : regSrc;
                    auto src = (byte == 0x21) ? regSrc : regDst;
                    llvm::Value* v1 = lifterContext.ReadRegister(dst, builder);
                    llvm::Value* v2 = lifterContext.ReadRegister(src, builder);
                    llvm::Value* res = lifterContext.EvaluateAlu(X86AluOp::And, v1, v2, builder);
                    lifterContext.WriteRegister(dst, res, builder);
                    offset = currentOffset + 2;
                    continue;
                } else if (byte == 0x09 || byte == 0x0B) {
                    auto dst = (byte == 0x09) ? regDst : regSrc;
                    auto src = (byte == 0x09) ? regSrc : regDst;
                    llvm::Value* v1 = lifterContext.ReadRegister(dst, builder);
                    llvm::Value* v2 = lifterContext.ReadRegister(src, builder);
                    llvm::Value* res = lifterContext.EvaluateAlu(X86AluOp::Or, v1, v2, builder);
                    lifterContext.WriteRegister(dst, res, builder);
                    offset = currentOffset + 2;
                    continue;
                } else if (byte == 0x39 || byte == 0x3B) {
                    auto dst = (byte == 0x39) ? regDst : regSrc;
                    auto src = (byte == 0x39) ? regSrc : regDst;
                    llvm::Value* v1 = lifterContext.ReadRegister(dst, builder);
                    llvm::Value* v2 = lifterContext.ReadRegister(src, builder);
                    static_cast<void>(lifterContext.EvaluateAlu(X86AluOp::Cmp, v1, v2, builder));
                    offset = currentOffset + 2;
                    continue;
                } else if (byte == 0x85) {
                    llvm::Value* v1 = lifterContext.ReadRegister(regDst, builder);
                    llvm::Value* v2 = lifterContext.ReadRegister(regSrc, builder);
                    static_cast<void>(lifterContext.EvaluateAlu(X86AluOp::Test, v1, v2, builder));
                    offset = currentOffset + 2;
                    continue;
                }
            }
        }

        offset += 1;
    }

    if (entryBlock->getTerminator() == nullptr) {
        llvm::Value* retVal = lifterContext.ReadRegister(X86Register::Rax, builder);
        builder.CreateRet(retVal);
    }

    return function;
}

} // namespace Aot::Compiler
