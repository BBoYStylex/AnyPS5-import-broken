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

        if ((byte & 0xF8) == 0x50) {
            uint8_t regIndex = (byte & 0x07) + (rexB ? 8 : 0);
            llvm::Value* rspVal = lifterContext.ReadRegister(X86Register::Rsp, builder);
            llvm::Value* newRsp = builder.CreateSub(rspVal, builder.getInt64(8));
            lifterContext.WriteRegister(X86Register::Rsp, newRsp, builder);
            llvm::Value* regVal = lifterContext.ReadRegister(static_cast<X86Register>(regIndex), builder);
            builder.CreateStore(regVal, newRsp);
            offset = currentOffset + 1;
            continue;
        }

        if ((byte & 0xF8) == 0x58) {
            uint8_t regIndex = (byte & 0x07) + (rexB ? 8 : 0);
            llvm::Value* rspVal = lifterContext.ReadRegister(X86Register::Rsp, builder);
            llvm::Value* loadedVal = builder.CreateLoad(builder.getInt64Ty(), rspVal);
            lifterContext.WriteRegister(static_cast<X86Register>(regIndex), loadedVal, builder);
            llvm::Value* newRsp = builder.CreateAdd(rspVal, builder.getInt64(8));
            lifterContext.WriteRegister(X86Register::Rsp, newRsp, builder);
            offset = currentOffset + 1;
            continue;
        }

        if (byte == 0xEB && currentOffset + 1 < byteCount) {
            llvm::BasicBlock* targetBB = llvm::BasicBlock::Create(context, "jmp_target", function);
            builder.CreateBr(targetBB);
            builder.SetInsertPoint(targetBB);
            offset = currentOffset + 2;
            continue;
        }

        if (byte == 0xE9 && currentOffset + 4 < byteCount) {
            llvm::BasicBlock* targetBB = llvm::BasicBlock::Create(context, "jmp_target", function);
            builder.CreateBr(targetBB);
            builder.SetInsertPoint(targetBB);
            offset = currentOffset + 5;
            continue;
        }

        if (byte == 0x74 && currentOffset + 1 < byteCount) {
            llvm::Value* cond = lifterContext.ReadZeroFlag(builder);
            llvm::BasicBlock* trueBB = llvm::BasicBlock::Create(context, "jz_taken", function);
            llvm::BasicBlock* contBB = llvm::BasicBlock::Create(context, "jz_fallthrough", function);
            builder.CreateCondBr(cond, trueBB, contBB);
            builder.SetInsertPoint(contBB);
            offset = currentOffset + 2;
            continue;
        }

        if (byte == 0x75 && currentOffset + 1 < byteCount) {
            llvm::Value* zf = lifterContext.ReadZeroFlag(builder);
            llvm::Value* cond = builder.CreateICmpEQ(zf, builder.getInt1(false));
            llvm::BasicBlock* trueBB = llvm::BasicBlock::Create(context, "jnz_taken", function);
            llvm::BasicBlock* contBB = llvm::BasicBlock::Create(context, "jnz_fallthrough", function);
            builder.CreateCondBr(cond, trueBB, contBB);
            builder.SetInsertPoint(contBB);
            offset = currentOffset + 2;
            continue;
        }

        if (byte == 0x72 && currentOffset + 1 < byteCount) {
            llvm::Value* cond = lifterContext.ReadCarryFlag(builder);
            llvm::BasicBlock* trueBB = llvm::BasicBlock::Create(context, "jc_taken", function);
            llvm::BasicBlock* contBB = llvm::BasicBlock::Create(context, "jc_fallthrough", function);
            builder.CreateCondBr(cond, trueBB, contBB);
            builder.SetInsertPoint(contBB);
            offset = currentOffset + 2;
            continue;
        }

        if (byte == 0x73 && currentOffset + 1 < byteCount) {
            llvm::Value* cf = lifterContext.ReadCarryFlag(builder);
            llvm::Value* cond = builder.CreateICmpEQ(cf, builder.getInt1(false));
            llvm::BasicBlock* trueBB = llvm::BasicBlock::Create(context, "jnc_taken", function);
            llvm::BasicBlock* contBB = llvm::BasicBlock::Create(context, "jnc_fallthrough", function);
            builder.CreateCondBr(cond, trueBB, contBB);
            builder.SetInsertPoint(contBB);
            offset = currentOffset + 2;
            continue;
        }

        if (byte == 0xE8 && currentOffset + 4 < byteCount) {
            llvm::Value* rspVal = lifterContext.ReadRegister(X86Register::Rsp, builder);
            llvm::Value* newRsp = builder.CreateSub(rspVal, builder.getInt64(8));
            lifterContext.WriteRegister(X86Register::Rsp, newRsp, builder);
            offset = currentOffset + 5;
            continue;
        }

        if (byte == 0x8D && currentOffset + 1 < byteCount) {
            uint8_t modrm = codeBytes[currentOffset + 1];
            uint8_t mod = (modrm >> 6) & 0x03;
            uint8_t regField = ((modrm >> 3) & 0x07) + (rexR ? 8 : 0);
            uint8_t rmField = (modrm & 0x07) + (rexB ? 8 : 0);
            auto regDst = static_cast<X86Register>(regField);
            auto regBase = static_cast<X86Register>(rmField);

            if (mod == 0) {
                llvm::Value* base = lifterContext.ReadRegister(regBase, builder);
                lifterContext.WriteRegister(regDst, base, builder);
                offset = currentOffset + 2;
                continue;
            } else if (mod == 1 && currentOffset + 2 < byteCount) {
                int8_t disp = static_cast<int8_t>(codeBytes[currentOffset + 2]);
                llvm::Value* base = lifterContext.ReadRegister(regBase, builder);
                llvm::Value* addr = builder.CreateAdd(base, builder.getInt64(disp));
                lifterContext.WriteRegister(regDst, addr, builder);
                offset = currentOffset + 3;
                continue;
            } else if (mod == 2 && currentOffset + 5 < byteCount) {
                int32_t disp = 0;
                std::memcpy(&disp, &codeBytes[currentOffset + 2], sizeof(int32_t));
                llvm::Value* base = lifterContext.ReadRegister(regBase, builder);
                llvm::Value* addr = builder.CreateAdd(base, builder.getInt64(disp));
                lifterContext.WriteRegister(regDst, addr, builder);
                offset = currentOffset + 6;
                continue;
            }
        }

        if ((byte == 0x80 || byte == 0x81 || byte == 0x83) && currentOffset + 1 < byteCount) {
            uint8_t modrm = codeBytes[currentOffset + 1];
            uint8_t mod = (modrm >> 6) & 0x03;
            uint8_t regOp = (modrm >> 3) & 0x07;
            uint8_t rmField = (modrm & 0x07) + (rexB ? 8 : 0);

            if (mod == 3) {
                auto regDst = static_cast<X86Register>(rmField);
                int64_t imm = 0;
                size_t nextOffset = 0;

                if (byte == 0x81 && currentOffset + 5 < byteCount) {
                    int32_t imm32 = 0;
                    std::memcpy(&imm32, &codeBytes[currentOffset + 2], sizeof(int32_t));
                    imm = static_cast<int64_t>(imm32);
                    nextOffset = currentOffset + 6;
                } else if ((byte == 0x83 || byte == 0x80) && currentOffset + 2 < byteCount) {
                    int8_t imm8 = static_cast<int8_t>(codeBytes[currentOffset + 2]);
                    imm = static_cast<int64_t>(imm8);
                    nextOffset = currentOffset + 3;
                }

                if (nextOffset != 0) {
                    llvm::Value* v1 = lifterContext.ReadRegister(regDst, builder);
                    llvm::Value* v2 = builder.getInt64(static_cast<uint64_t>(imm));
                    X86AluOp aluOp = X86AluOp::Add;
                    if (regOp == 0) aluOp = X86AluOp::Add;
                    else if (regOp == 1) aluOp = X86AluOp::Or;
                    else if (regOp == 4) aluOp = X86AluOp::And;
                    else if (regOp == 5) aluOp = X86AluOp::Sub;
                    else if (regOp == 6) aluOp = X86AluOp::Xor;
                    else if (regOp == 7) aluOp = X86AluOp::Cmp;

                    llvm::Value* res = lifterContext.EvaluateAlu(aluOp, v1, v2, builder);
                    if (regOp != 7) {
                        lifterContext.WriteRegister(regDst, res, builder);
                    }
                    offset = nextOffset;
                    continue;
                }
            }
        }

        if (currentOffset + 1 < byteCount) {
            uint8_t modrm = codeBytes[currentOffset + 1];
            uint8_t mod = (modrm >> 6) & 0x03;
            uint8_t regField = ((modrm >> 3) & 0x07) + (rexR ? 8 : 0);
            uint8_t rmField = (modrm & 0x07) + (rexB ? 8 : 0);

            if (mod != 3 && (byte == 0x89 || byte == 0x8B)) {
                auto regDst = static_cast<X86Register>(rmField);
                auto regSrc = static_cast<X86Register>(regField);
                llvm::Value* base = lifterContext.ReadRegister(regDst, builder);
                llvm::Value* addr = base;
                size_t nextOffset = currentOffset + 2;

                if (mod == 1 && currentOffset + 2 < byteCount) {
                    int8_t disp = static_cast<int8_t>(codeBytes[currentOffset + 2]);
                    addr = builder.CreateAdd(base, builder.getInt64(disp));
                    nextOffset = currentOffset + 3;
                } else if (mod == 2 && currentOffset + 5 < byteCount) {
                    int32_t disp = 0;
                    std::memcpy(&disp, &codeBytes[currentOffset + 2], sizeof(int32_t));
                    addr = builder.CreateAdd(base, builder.getInt64(disp));
                    nextOffset = currentOffset + 6;
                }

                if (byte == 0x89) {
                    llvm::Value* srcVal = lifterContext.ReadRegister(regSrc, builder);
                    builder.CreateStore(srcVal, addr);
                } else {
                    llvm::Value* loaded = builder.CreateLoad(builder.getInt64Ty(), addr);
                    lifterContext.WriteRegister(regSrc, loaded, builder);
                }
                offset = nextOffset;
                continue;
            }

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

    for (auto& bb : *function) {
        if (bb.getTerminator() == nullptr) {
            llvm::IRBuilder<> termBuilder(&bb);
            llvm::Value* retVal = lifterContext.ReadRegister(X86Register::Rax, termBuilder);
            termBuilder.CreateRet(retVal);
        }
    }

    return function;
}

} // namespace Aot::Compiler
