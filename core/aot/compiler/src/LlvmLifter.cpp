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

struct DecodedEffectiveAddress {
    llvm::Value* Address;
    size_t ConsumedBytes;
};

static DecodedEffectiveAddress decodeAddress(
    const uint8_t* codeBytes,
    size_t currentOffset,
    size_t byteCount,
    uint8_t mod,
    uint8_t rm,
    bool rexX,
    bool rexB,
    uint64_t virtualAddress,
    llvm::IRBuilder<>& builder,
    X86LifterContext& lifterContext) {
    if (mod == 3) {
        return {nullptr, 0};
    }

    if (mod == 0 && (rm & 0x07) == 5) {
        if (currentOffset + 4 <= byteCount) {
            int32_t disp = 0;
            std::memcpy(&disp, &codeBytes[currentOffset], sizeof(int32_t));
            uint64_t ripTarget = virtualAddress + currentOffset + 4 + disp;
            return {builder.getInt64(ripTarget), 4};
        }
        return {builder.getInt64(0), 4};
    }

    if ((rm & 0x07) == 4 && currentOffset < byteCount) {
        uint8_t sib = codeBytes[currentOffset];
        uint8_t scale = (sib >> 6) & 0x03;
        uint8_t indexReg = ((sib >> 3) & 0x07) + (rexX ? 8 : 0);
        uint8_t baseReg = (sib & 0x07) + (rexB ? 8 : 0);

        llvm::Value* baseVal = nullptr;
        size_t dispOffset = currentOffset + 1;
        int32_t disp = 0;

        if (mod == 0 && (sib & 0x07) == 5) {
            if (dispOffset + 4 <= byteCount) {
                std::memcpy(&disp, &codeBytes[dispOffset], sizeof(int32_t));
                dispOffset += 4;
            }
            baseVal = builder.getInt64(disp);
        } else {
            baseVal = lifterContext.ReadRegister(static_cast<X86Register>(baseReg), builder);
            if (mod == 1 && dispOffset < byteCount) {
                disp = static_cast<int8_t>(codeBytes[dispOffset]);
                dispOffset += 1;
                baseVal = builder.CreateAdd(baseVal, builder.getInt64(disp));
            } else if (mod == 2 && dispOffset + 4 <= byteCount) {
                std::memcpy(&disp, &codeBytes[dispOffset], sizeof(int32_t));
                dispOffset += 4;
                baseVal = builder.CreateAdd(baseVal, builder.getInt64(disp));
            }
        }

        if (indexReg != 4 || rexX) {
            llvm::Value* indexVal = lifterContext.ReadRegister(static_cast<X86Register>(indexReg), builder);
            if (scale > 0) {
                indexVal = builder.CreateShl(indexVal, builder.getInt64(scale));
            }
            baseVal = builder.CreateAdd(baseVal, indexVal);
        }

        return {baseVal, dispOffset - currentOffset};
    }

    auto regBase = static_cast<X86Register>(rm);
    llvm::Value* baseVal = lifterContext.ReadRegister(regBase, builder);
    if (mod == 0) {
        return {baseVal, 0};
    } else if (mod == 1 && currentOffset < byteCount) {
        int8_t disp = static_cast<int8_t>(codeBytes[currentOffset]);
        return {builder.CreateAdd(baseVal, builder.getInt64(disp)), 1};
    } else if (mod == 2 && currentOffset + 4 <= byteCount) {
        int32_t disp = 0;
        std::memcpy(&disp, &codeBytes[currentOffset], sizeof(int32_t));
        return {builder.CreateAdd(baseVal, builder.getInt64(disp)), 4};
    }

    return {baseVal, 0};
}

static llvm::Value* evaluateConditionCode(
    uint8_t condCode,
    llvm::IRBuilder<>& builder,
    X86LifterContext& lifterContext) {
    switch (condCode & 0x0F) {
        case 0x0:
            return lifterContext.ReadOverflowFlag(builder);
        case 0x1:
            return builder.CreateICmpEQ(lifterContext.ReadOverflowFlag(builder), builder.getInt1(false));
        case 0x2:
            return lifterContext.ReadCarryFlag(builder);
        case 0x3:
            return builder.CreateICmpEQ(lifterContext.ReadCarryFlag(builder), builder.getInt1(false));
        case 0x4:
            return lifterContext.ReadZeroFlag(builder);
        case 0x5:
            return builder.CreateICmpEQ(lifterContext.ReadZeroFlag(builder), builder.getInt1(false));
        case 0x6:
            return builder.CreateOr(lifterContext.ReadCarryFlag(builder), lifterContext.ReadZeroFlag(builder));
        case 0x7: {
            llvm::Value* notCf = builder.CreateICmpEQ(lifterContext.ReadCarryFlag(builder), builder.getInt1(false));
            llvm::Value* notZf = builder.CreateICmpEQ(lifterContext.ReadZeroFlag(builder), builder.getInt1(false));
            return builder.CreateAnd(notCf, notZf);
        }
        case 0x8:
            return lifterContext.ReadSignFlag(builder);
        case 0x9:
            return builder.CreateICmpEQ(lifterContext.ReadSignFlag(builder), builder.getInt1(false));
        case 0xC:
            return builder.CreateICmpNE(lifterContext.ReadSignFlag(builder), lifterContext.ReadOverflowFlag(builder));
        case 0xD:
            return builder.CreateICmpEQ(lifterContext.ReadSignFlag(builder), lifterContext.ReadOverflowFlag(builder));
        case 0xE:
            return builder.CreateOr(
                lifterContext.ReadZeroFlag(builder),
                builder.CreateICmpNE(lifterContext.ReadSignFlag(builder), lifterContext.ReadOverflowFlag(builder))
            );
        case 0xF: {
            llvm::Value* notZf = builder.CreateICmpEQ(lifterContext.ReadZeroFlag(builder), builder.getInt1(false));
            llvm::Value* sOfSame = builder.CreateICmpEQ(lifterContext.ReadSignFlag(builder), lifterContext.ReadOverflowFlag(builder));
            return builder.CreateAnd(notZf, sOfSame);
        }
        default:
            return builder.getInt1(true);
    }
}

llvm::Function* LlvmLifter::liftFunctionAtAddress(
    const uint8_t* codeBytes,
    size_t byteCount,
    uint64_t virtualAddress,
    std::string_view name,
    llvm::Module& module) {
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

        bool hasPrefix66 = false;
        bool hasPrefixF2 = false;
        bool hasPrefixF3 = false;
        size_t currentOffset = offset;

        while (currentOffset < byteCount) {
            uint8_t p = codeBytes[currentOffset];
            if (p == 0x66) {
                hasPrefix66 = true;
                currentOffset++;
            } else if (p == 0xF2) {
                hasPrefixF2 = true;
                currentOffset++;
            } else if (p == 0xF3) {
                hasPrefixF3 = true;
                currentOffset++;
            } else {
                break;
            }
        }

        if (currentOffset >= byteCount) {
            offset++;
            continue;
        }

        bool rexW = false;
        bool rexR = false;
        bool rexX = false;
        bool rexB = false;

        if ((codeBytes[currentOffset] & 0xF0) == 0x40) {
            uint8_t rex = codeBytes[currentOffset];
            rexW = (rex & 0x08) != 0;
            rexR = (rex & 0x04) != 0;
            rexX = (rex & 0x02) != 0;
            rexB = (rex & 0x01) != 0;
            currentOffset++;
        }

        if (currentOffset >= byteCount) {
            offset++;
            continue;
        }

        byte = codeBytes[currentOffset];

        if (byte == 0x0F && currentOffset + 1 < byteCount) {
            uint8_t op2 = codeBytes[currentOffset + 1];

            if (op2 == 0x05) {
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
                offset = currentOffset + 2;
                continue;
            }

            if ((op2 >= 0x80 && op2 <= 0x8F) && currentOffset + 5 < byteCount) {
                int32_t rel32 = 0;
                std::memcpy(&rel32, &codeBytes[currentOffset + 2], sizeof(int32_t));
                llvm::Value* cond = evaluateConditionCode(op2 & 0x0F, builder, lifterContext);
                llvm::BasicBlock* trueBB = llvm::BasicBlock::Create(context, "jcc_taken", function);
                llvm::BasicBlock* contBB = llvm::BasicBlock::Create(context, "jcc_fallthrough", function);
                builder.CreateCondBr(cond, trueBB, contBB);
                builder.SetInsertPoint(contBB);
                offset = currentOffset + 6;
                continue;
            }

            if (currentOffset + 2 < byteCount) {
                uint8_t modrm = codeBytes[currentOffset + 2];
                uint8_t mod = (modrm >> 6) & 0x03;
                uint8_t regField = ((modrm >> 3) & 0x07) + (rexR ? 8 : 0);
                uint8_t rmField = (modrm & 0x07) + (rexB ? 8 : 0);

                if (op2 >= 0x40 && op2 <= 0x4F) {
                    auto regDst = static_cast<X86Register>(regField);
                    llvm::Value* cond = evaluateConditionCode(op2 & 0x0F, builder, lifterContext);
                    llvm::Value* curVal = lifterContext.ReadRegister(regDst, builder);
                    if (mod == 3) {
                        auto regSrc = static_cast<X86Register>(rmField);
                        llvm::Value* srcVal = lifterContext.ReadRegister(regSrc, builder);
                        llvm::Value* selected = builder.CreateSelect(cond, srcVal, curVal);
                        lifterContext.WriteRegister(regDst, selected, builder);
                        offset = currentOffset + 3;
                        continue;
                    } else {
                        auto decoded = decodeAddress(codeBytes, currentOffset + 3, byteCount, mod, rmField, rexX, rexB, virtualAddress, builder, lifterContext);
                        llvm::Type* opTy = rexW ? builder.getInt64Ty() : builder.getInt32Ty();
                        llvm::Value* loaded = builder.CreateLoad(opTy, decoded.Address);
                        if (!rexW) {
                            loaded = builder.CreateZExt(loaded, builder.getInt64Ty());
                        }
                        llvm::Value* selected = builder.CreateSelect(cond, loaded, curVal);
                        lifterContext.WriteRegister(regDst, selected, builder);
                        offset = currentOffset + 3 + decoded.ConsumedBytes;
                        continue;
                    }
                }

                if (op2 >= 0x90 && op2 <= 0x9F) {
                    llvm::Value* cond = evaluateConditionCode(op2 & 0x0F, builder, lifterContext);
                    if (mod == 3) {
                        auto regDst = static_cast<X86Register>(rmField);
                        llvm::Value* curVal = lifterContext.ReadRegister(regDst, builder);
                        llvm::Value* clearLow8 = builder.CreateAnd(curVal, builder.getInt64(~0xFFULL));
                        llvm::Value* condExt = builder.CreateZExt(cond, builder.getInt64Ty());
                        llvm::Value* finalVal = builder.CreateOr(clearLow8, condExt);
                        lifterContext.WriteRegister(regDst, finalVal, builder);
                        offset = currentOffset + 3;
                        continue;
                    } else {
                        auto decoded = decodeAddress(codeBytes, currentOffset + 3, byteCount, mod, rmField, rexX, rexB, virtualAddress, builder, lifterContext);
                        llvm::Value* condByte = builder.CreateZExt(cond, builder.getInt8Ty());
                        builder.CreateStore(condByte, decoded.Address);
                        offset = currentOffset + 3 + decoded.ConsumedBytes;
                        continue;
                    }
                }

                if (op2 == 0xA3) {
                    llvm::Value* bitIdx = lifterContext.ReadRegister(static_cast<X86Register>(regField), builder);
                    llvm::Value* mask = builder.getInt64(rexW ? 0x3F : 0x1F);
                    llvm::Value* maskedIdx = builder.CreateAnd(bitIdx, mask);
                    llvm::Value* baseVal = nullptr;
                    size_t nextOffset = 0;
                    if (mod == 3) {
                        baseVal = lifterContext.ReadRegister(static_cast<X86Register>(rmField), builder);
                        nextOffset = currentOffset + 3;
                    } else {
                        auto decoded = decodeAddress(codeBytes, currentOffset + 3, byteCount, mod, rmField, rexX, rexB, virtualAddress, builder, lifterContext);
                        baseVal = builder.CreateLoad(builder.getInt64Ty(), decoded.Address);
                        nextOffset = currentOffset + 3 + decoded.ConsumedBytes;
                    }
                    llvm::Value* shifted = builder.CreateLShr(baseVal, maskedIdx);
                    llvm::Value* bit = builder.CreateTrunc(shifted, builder.getInt1Ty());
                    lifterContext.WriteCarryFlag(bit, builder);
                    offset = nextOffset;
                    continue;
                }

                if (op2 == 0xBA) {
                    uint8_t regOp = (modrm >> 3) & 0x07;
                    if (regOp == 4) {
                        llvm::Value* baseVal = nullptr;
                        size_t immOffset = 0;
                        if (mod == 3) {
                            baseVal = lifterContext.ReadRegister(static_cast<X86Register>(rmField), builder);
                            immOffset = currentOffset + 3;
                        } else {
                            auto decoded = decodeAddress(codeBytes, currentOffset + 3, byteCount, mod, rmField, rexX, rexB, virtualAddress, builder, lifterContext);
                            baseVal = builder.CreateLoad(builder.getInt64Ty(), decoded.Address);
                            immOffset = currentOffset + 3 + decoded.ConsumedBytes;
                        }
                        if (immOffset < byteCount) {
                            uint8_t bitIdxImm = codeBytes[immOffset];
                            uint8_t mask = rexW ? 0x3F : 0x1F;
                            llvm::Value* shiftVal = builder.getInt64(bitIdxImm & mask);
                            llvm::Value* shifted = builder.CreateLShr(baseVal, shiftVal);
                            llvm::Value* bit = builder.CreateTrunc(shifted, builder.getInt1Ty());
                            lifterContext.WriteCarryFlag(bit, builder);
                            offset = immOffset + 1;
                            continue;
                        }
                    }
                }

                if (op2 == 0x28 || op2 == 0x10) {
                    if (mod == 3) {
                        llvm::Value* srcVal = lifterContext.ReadXmmRegister(rmField, builder);
                        lifterContext.WriteXmmRegister(regField, srcVal, builder);
                        offset = currentOffset + 3;
                        continue;
                    } else {
                        auto decoded = decodeAddress(codeBytes, currentOffset + 3, byteCount, mod, rmField, rexX, rexB, virtualAddress, builder, lifterContext);
                        llvm::Value* loaded = builder.CreateLoad(builder.getVectorTy(), decoded.Address);
                        lifterContext.WriteXmmRegister(regField, loaded, builder);
                        offset = currentOffset + 3 + decoded.ConsumedBytes;
                        continue;
                    }
                } else if (op2 == 0x29 || op2 == 0x11) {
                    if (mod == 3) {
                        llvm::Value* srcVal = lifterContext.ReadXmmRegister(regField, builder);
                        lifterContext.WriteXmmRegister(rmField, srcVal, builder);
                        offset = currentOffset + 3;
                        continue;
                    } else {
                        auto decoded = decodeAddress(codeBytes, currentOffset + 3, byteCount, mod, rmField, rexX, rexB, virtualAddress, builder, lifterContext);
                        llvm::Value* srcVal = lifterContext.ReadXmmRegister(regField, builder);
                        builder.CreateStore(srcVal, decoded.Address);
                        offset = currentOffset + 3 + decoded.ConsumedBytes;
                        continue;
                    }
                } else if (op2 == 0x58 || op2 == 0x5C || op2 == 0x59 || op2 == 0x5E ||
                           op2 == 0x54 || op2 == 0x56 || op2 == 0x57 || op2 == 0xEF) {
                    llvm::Value* v1 = lifterContext.ReadXmmRegister(regField, builder);
                    llvm::Value* v2 = nullptr;
                    size_t nextOffset = currentOffset + 3;

                    if (mod == 3) {
                        v2 = lifterContext.ReadXmmRegister(rmField, builder);
                    } else {
                        auto decoded = decodeAddress(codeBytes, currentOffset + 3, byteCount, mod, rmField, rexX, rexB, virtualAddress, builder, lifterContext);
                        v2 = builder.CreateLoad(builder.getVectorTy(), decoded.Address);
                        nextOffset = currentOffset + 3 + decoded.ConsumedBytes;
                    }

                    llvm::Value* res = nullptr;
                    if (op2 == 0x58) {
                        res = builder.CreateFAdd(v1, v2);
                    } else if (op2 == 0x5C) {
                        res = builder.CreateFSub(v1, v2);
                    } else if (op2 == 0x59) {
                        res = builder.CreateFMul(v1, v2);
                    } else if (op2 == 0x5E) {
                        res = builder.CreateFDiv(v1, v2);
                    } else if (op2 == 0x54) {
                        res = builder.CreateAnd(v1, v2);
                    } else if (op2 == 0x56) {
                        res = builder.CreateOr(v1, v2);
                    } else if (op2 == 0x57 || op2 == 0xEF) {
                        res = builder.CreateXor(v1, v2);
                    }

                    lifterContext.WriteXmmRegister(regField, res, builder);
                    offset = nextOffset;
                    continue;
                } else if (op2 == 0xAF) {
                    llvm::Value* v1 = lifterContext.ReadRegister(static_cast<X86Register>(regField), builder);
                    llvm::Value* v2 = nullptr;
                    size_t nextOffset = currentOffset + 3;
                    if (mod == 3) {
                        v2 = lifterContext.ReadRegister(static_cast<X86Register>(rmField), builder);
                    } else {
                        auto decoded = decodeAddress(codeBytes, currentOffset + 3, byteCount, mod, rmField, rexX, rexB, virtualAddress, builder, lifterContext);
                        v2 = builder.CreateLoad(builder.getInt64Ty(), decoded.Address);
                        nextOffset = currentOffset + 3 + decoded.ConsumedBytes;
                    }
                    llvm::Value* res = builder.CreateMul(v1, v2);
                    lifterContext.WriteRegister(static_cast<X86Register>(regField), res, builder);
                    lifterContext.UpdateFlags(res, builder);
                    offset = nextOffset;
                    continue;
                } else if (op2 == 0xB6 || op2 == 0xB7) {
                    llvm::Value* val = nullptr;
                    size_t nextOffset = currentOffset + 3;
                    llvm::Type* srcTy = (op2 == 0xB6) ? builder.getInt8Ty() : builder.getInt16Ty();
                    if (mod == 3) {
                        llvm::Value* raw = lifterContext.ReadRegister(static_cast<X86Register>(rmField), builder);
                        val = builder.CreateTrunc(raw, srcTy);
                    } else {
                        auto decoded = decodeAddress(codeBytes, currentOffset + 3, byteCount, mod, rmField, rexX, rexB, virtualAddress, builder, lifterContext);
                        val = builder.CreateLoad(srcTy, decoded.Address);
                        nextOffset = currentOffset + 3 + decoded.ConsumedBytes;
                    }
                    llvm::Value* extended = builder.CreateZExt(val, builder.getInt64Ty());
                    lifterContext.WriteRegister(static_cast<X86Register>(regField), extended, builder);
                    offset = nextOffset;
                    continue;
                } else if (op2 == 0xBE || op2 == 0xBF) {
                    llvm::Value* val = nullptr;
                    size_t nextOffset = currentOffset + 3;
                    llvm::Type* srcTy = (op2 == 0xBE) ? builder.getInt8Ty() : builder.getInt16Ty();
                    if (mod == 3) {
                        llvm::Value* raw = lifterContext.ReadRegister(static_cast<X86Register>(rmField), builder);
                        val = builder.CreateTrunc(raw, srcTy);
                    } else {
                        auto decoded = decodeAddress(codeBytes, currentOffset + 3, byteCount, mod, rmField, rexX, rexB, virtualAddress, builder, lifterContext);
                        val = builder.CreateLoad(srcTy, decoded.Address);
                        nextOffset = currentOffset + 3 + decoded.ConsumedBytes;
                    }
                    llvm::Value* extended = builder.CreateSExt(val, builder.getInt64Ty());
                    lifterContext.WriteRegister(static_cast<X86Register>(regField), extended, builder);
                    offset = nextOffset;
                    continue;
                }
            }
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

        if ((byte >= 0x70 && byte <= 0x7F) && currentOffset + 1 < byteCount) {
            llvm::Value* cond = evaluateConditionCode(byte & 0x0F, builder, lifterContext);
            llvm::BasicBlock* trueBB = llvm::BasicBlock::Create(context, "jcc_taken", function);
            llvm::BasicBlock* contBB = llvm::BasicBlock::Create(context, "jcc_fallthrough", function);
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

            if (mod != 3) {
                auto decoded = decodeAddress(codeBytes, currentOffset + 2, byteCount, mod, rmField, rexX, rexB, virtualAddress, builder, lifterContext);
                lifterContext.WriteRegister(regDst, decoded.Address, builder);
                offset = currentOffset + 2 + decoded.ConsumedBytes;
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

        if (byte == 0x63 && currentOffset + 1 < byteCount) {
            uint8_t modrm = codeBytes[currentOffset + 1];
            uint8_t mod = (modrm >> 6) & 0x03;
            uint8_t regField = ((modrm >> 3) & 0x07) + (rexR ? 8 : 0);
            uint8_t rmField = (modrm & 0x07) + (rexB ? 8 : 0);
            llvm::Value* val = nullptr;
            size_t nextOffset = currentOffset + 2;
            if (mod == 3) {
                llvm::Value* raw = lifterContext.ReadRegister(static_cast<X86Register>(rmField), builder);
                val = builder.CreateTrunc(raw, builder.getInt32Ty());
            } else {
                auto decoded = decodeAddress(codeBytes, currentOffset + 2, byteCount, mod, rmField, rexX, rexB, virtualAddress, builder, lifterContext);
                val = builder.CreateLoad(builder.getInt32Ty(), decoded.Address);
                nextOffset = currentOffset + 2 + decoded.ConsumedBytes;
            }
            llvm::Value* extended = builder.CreateSExt(val, builder.getInt64Ty());
            lifterContext.WriteRegister(static_cast<X86Register>(regField), extended, builder);
            offset = nextOffset;
            continue;
        }

        if ((byte == 0xC1 || byte == 0xD1 || byte == 0xD3) && currentOffset + 1 < byteCount) {
            uint8_t modrm = codeBytes[currentOffset + 1];
            uint8_t mod = (modrm >> 6) & 0x03;
            uint8_t regOp = (modrm >> 3) & 0x07;
            uint8_t rmField = (modrm & 0x07) + (rexB ? 8 : 0);

            if (regOp == 4 || regOp == 5 || regOp == 7) {
                X86AluOp aluOp = (regOp == 4) ? X86AluOp::Shl : ((regOp == 5) ? X86AluOp::Shr : X86AluOp::Sar);
                llvm::Value* shiftAmount = nullptr;
                size_t nextOffset = 0;

                if (mod == 3) {
                    if (byte == 0xD1) {
                        shiftAmount = builder.getInt64(1);
                        nextOffset = currentOffset + 2;
                    } else if (byte == 0xC1 && currentOffset + 2 < byteCount) {
                        uint8_t imm8 = codeBytes[currentOffset + 2];
                        uint8_t mask = rexW ? 0x3F : 0x1F;
                        shiftAmount = builder.getInt64(imm8 & mask);
                        nextOffset = currentOffset + 3;
                    } else if (byte == 0xD3) {
                        llvm::Value* rcxVal = lifterContext.ReadRegister(X86Register::Rcx, builder);
                        uint64_t mask = rexW ? 0x3F : 0x1F;
                        shiftAmount = builder.CreateAnd(rcxVal, builder.getInt64(mask));
                        nextOffset = currentOffset + 2;
                    }

                    if (shiftAmount != nullptr) {
                        auto regDst = static_cast<X86Register>(rmField);
                        llvm::Value* v1 = lifterContext.ReadRegister(regDst, builder);
                        llvm::Value* res = lifterContext.EvaluateAlu(aluOp, v1, shiftAmount, builder);
                        lifterContext.WriteRegister(regDst, res, builder);
                        offset = nextOffset;
                        continue;
                    }
                } else {
                    auto decoded = decodeAddress(codeBytes, currentOffset + 2, byteCount, mod, rmField, rexX, rexB, virtualAddress, builder, lifterContext);
                    size_t immOffset = currentOffset + 2 + decoded.ConsumedBytes;

                    if (byte == 0xD1) {
                        shiftAmount = builder.getInt64(1);
                        nextOffset = immOffset;
                    } else if (byte == 0xC1 && immOffset < byteCount) {
                        uint8_t imm8 = codeBytes[immOffset];
                        uint8_t mask = rexW ? 0x3F : 0x1F;
                        shiftAmount = builder.getInt64(imm8 & mask);
                        nextOffset = immOffset + 1;
                    } else if (byte == 0xD3) {
                        llvm::Value* rcxVal = lifterContext.ReadRegister(X86Register::Rcx, builder);
                        uint64_t mask = rexW ? 0x3F : 0x1F;
                        shiftAmount = builder.CreateAnd(rcxVal, builder.getInt64(mask));
                        nextOffset = immOffset;
                    }

                    if (shiftAmount != nullptr) {
                        llvm::Value* v1 = builder.CreateLoad(builder.getInt64Ty(), decoded.Address);
                        llvm::Value* res = lifterContext.EvaluateAlu(aluOp, v1, shiftAmount, builder);
                        builder.CreateStore(res, decoded.Address);
                        offset = nextOffset;
                        continue;
                    }
                }
            }
        }

        if ((byte == 0xF6 || byte == 0xF7) && currentOffset + 1 < byteCount) {
            uint8_t modrm = codeBytes[currentOffset + 1];
            uint8_t mod = (modrm >> 6) & 0x03;
            uint8_t regOp = (modrm >> 3) & 0x07;
            uint8_t rmField = (modrm & 0x07) + (rexB ? 8 : 0);

            if (regOp == 2 || regOp == 3) {
                X86AluOp aluOp = (regOp == 2) ? X86AluOp::Not : X86AluOp::Neg;
                if (mod == 3) {
                    auto regDst = static_cast<X86Register>(rmField);
                    llvm::Value* v1 = lifterContext.ReadRegister(regDst, builder);
                    llvm::Value* res = lifterContext.EvaluateAlu(aluOp, v1, builder.getInt64(0), builder);
                    lifterContext.WriteRegister(regDst, res, builder);
                    offset = currentOffset + 2;
                    continue;
                } else {
                    auto decoded = decodeAddress(codeBytes, currentOffset + 2, byteCount, mod, rmField, rexX, rexB, virtualAddress, builder, lifterContext);
                    llvm::Value* v1 = builder.CreateLoad(builder.getInt64Ty(), decoded.Address);
                    llvm::Value* res = lifterContext.EvaluateAlu(aluOp, v1, builder.getInt64(0), builder);
                    builder.CreateStore(res, decoded.Address);
                    offset = currentOffset + 2 + decoded.ConsumedBytes;
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
                auto decoded = decodeAddress(codeBytes, currentOffset + 2, byteCount, mod, rmField, rexX, rexB, virtualAddress, builder, lifterContext);

                if (byte == 0x89) {
                    llvm::Value* srcVal = lifterContext.ReadRegister(regSrc, builder);
                    builder.CreateStore(srcVal, decoded.Address);
                } else {
                    llvm::Value* loaded = builder.CreateLoad(builder.getInt64Ty(), decoded.Address);
                    lifterContext.WriteRegister(regSrc, loaded, builder);
                }
                offset = currentOffset + 2 + decoded.ConsumedBytes;
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
