#include <graphics/ShaderInstructionDecoder.hpp>
#include <cstring>

namespace Aot::Graphics {

ShaderInstructionDecoder::ShaderInstructionDecoder() = default;
ShaderInstructionDecoder::~ShaderInstructionDecoder() = default;

std::vector<DecodedShaderInstruction> ShaderInstructionDecoder::DecodeBytecode(std::span<const uint8_t> bytecode) {
    std::vector<DecodedShaderInstruction> instructions;
    if (bytecode.size() < 4) {
        return instructions;
    }

    size_t wordCount = bytecode.size() / 4;
    const auto* words = reinterpret_cast<const uint32_t*>(bytecode.data());

    for (size_t i = 0; i < wordCount; ++i) {
        uint32_t raw = words[i];
        DecodedShaderInstruction instr{};
        instr.RawWord = raw;

        if (raw == 0xBF810000 || raw == 0xBF800000) {
            instr.OpClass = ShaderOpcodeClass::FlowControl;
            instr.TargetSpirvOp = DecodedSpirvOp::Return;
            instr.IsEndProgram = true;
            instructions.push_back(instr);
            break;
        }

        uint32_t topBits = (raw >> 26) & 0x3F;

        if (topBits >= 0x00 && topBits <= 0x1F) {
            instr.OpClass = ShaderOpcodeClass::VectorAlu;
            uint32_t opId = (raw >> 25) & 0x3F;
            if (opId == 1 || opId == 3) {
                instr.TargetSpirvOp = DecodedSpirvOp::FAdd;
            } else if (opId == 2 || opId == 4) {
                instr.TargetSpirvOp = DecodedSpirvOp::FSub;
            } else if (opId == 5 || opId == 8) {
                instr.TargetSpirvOp = DecodedSpirvOp::FMul;
            } else if (opId == 9) {
                instr.TargetSpirvOp = DecodedSpirvOp::FDiv;
            } else if (opId == 10) {
                instr.TargetSpirvOp = DecodedSpirvOp::Dot;
            } else if (opId == 11) {
                instr.TargetSpirvOp = DecodedSpirvOp::Select;
            } else if (opId == 12) {
                instr.TargetSpirvOp = DecodedSpirvOp::FMod;
            } else if (opId == 13) {
                instr.TargetSpirvOp = DecodedSpirvOp::UDiv;
            } else {
                instr.TargetSpirvOp = DecodedSpirvOp::FAdd;
            }
            instr.DstRegister = (raw >> 17) & 0xFF;
            instr.Src0Register = raw & 0x1FF;
            instr.Src1Register = (raw >> 9) & 0xFF;
        } else if (topBits >= 0x3C) {
            instr.OpClass = ShaderOpcodeClass::VectorMemory;
            if (topBits == 0x3D) {
                instr.TargetSpirvOp = DecodedSpirvOp::ImageRead;
            } else if (topBits == 0x3E) {
                instr.TargetSpirvOp = DecodedSpirvOp::ImageWrite;
            } else {
                instr.TargetSpirvOp = DecodedSpirvOp::ImageSample;
            }
            instr.DstRegister = (raw >> 16) & 0xFF;
            instr.Src0Register = raw & 0xFF;
            instr.Src1Register = (raw >> 8) & 0xFF;
        } else if (topBits >= 0x30 && topBits <= 0x3B) {
            instr.OpClass = ShaderOpcodeClass::FlowControl;
            uint32_t flowOp = (raw >> 20) & 0x3F;
            if (flowOp == 1) {
                instr.TargetSpirvOp = DecodedSpirvOp::BranchConditional;
            } else {
                instr.TargetSpirvOp = DecodedSpirvOp::Branch;
            }
        } else {
            instr.OpClass = ShaderOpcodeClass::ScalarAlu;
            uint32_t scalarOp = (raw >> 23) & 0x3F;
            if (scalarOp == 0) {
                instr.TargetSpirvOp = DecodedSpirvOp::IAdd;
            } else if (scalarOp == 1) {
                instr.TargetSpirvOp = DecodedSpirvOp::ISub;
            } else if (scalarOp == 2) {
                instr.TargetSpirvOp = DecodedSpirvOp::IMul;
            } else if (scalarOp == 3) {
                instr.TargetSpirvOp = DecodedSpirvOp::Select;
            } else if (scalarOp == 4) {
                instr.TargetSpirvOp = DecodedSpirvOp::Load;
            } else if (scalarOp == 5) {
                instr.TargetSpirvOp = DecodedSpirvOp::Store;
            } else if (scalarOp == 6) {
                instr.TargetSpirvOp = DecodedSpirvOp::UDiv;
            } else {
                instr.TargetSpirvOp = DecodedSpirvOp::Load;
            }
            instr.DstRegister = (raw >> 16) & 0x7F;
            instr.Src0Register = raw & 0xFF;
            instr.Src1Register = (raw >> 8) & 0xFF;
        }

        instructions.push_back(instr);
    }

    return instructions;
}

std::vector<uint32_t> ShaderInstructionDecoder::GenerateSpirvFromInstructions(
    const std::vector<DecodedShaderInstruction>& instructions,
    uint32_t executionModel,
    std::span<const ResourceBindingInfo> bindings) {
    std::vector<uint32_t> spirv;

    spirv.push_back(0x07230203);
    spirv.push_back(0x00010300);
    spirv.push_back(0x00000000);
    spirv.push_back(32);
    spirv.push_back(0x00000000);

    spirv.push_back((2 << 16) | 17);
    spirv.push_back(1);

    spirv.push_back((3 << 16) | 14);
    spirv.push_back(0);
    spirv.push_back(1);

    spirv.push_back((5 << 16) | 15);
    spirv.push_back(executionModel);
    spirv.push_back(1);
    spirv.push_back(0x6e69616d);
    spirv.push_back(0x00000000);

    if (executionModel == 4) {
        spirv.push_back((3 << 16) | 16);
        spirv.push_back(1);
        spirv.push_back(7);
    } else if (executionModel == 5) {
        spirv.push_back((6 << 16) | 16);
        spirv.push_back(1);
        spirv.push_back(17);
        spirv.push_back(64);
        spirv.push_back(1);
        spirv.push_back(1);
    }

    bool hasImageSample = false;
    bool hasImageRead = false;
    bool hasImageWrite = false;
    for (const auto& instr : instructions) {
        if (instr.TargetSpirvOp == DecodedSpirvOp::ImageSample) {
            hasImageSample = true;
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::ImageRead) {
            hasImageRead = true;
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::ImageWrite) {
            hasImageWrite = true;
        }
    }
    bool needsDefaultSampler = bindings.empty() && (hasImageSample || hasImageRead || hasImageWrite);
    uint32_t baseVarId = 23;

    if (needsDefaultSampler) {
        spirv.push_back((4 << 16) | 71);
        spirv.push_back(baseVarId);
        spirv.push_back(33);
        spirv.push_back(0);

        spirv.push_back((4 << 16) | 71);
        spirv.push_back(baseVarId);
        spirv.push_back(34);
        spirv.push_back(0);
    } else {
        for (size_t i = 0; i < bindings.size(); ++i) {
            uint32_t varId = baseVarId + static_cast<uint32_t>(i);
            spirv.push_back((4 << 16) | 71);
            spirv.push_back(varId);
            spirv.push_back(33);
            spirv.push_back(bindings[i].Binding);

            spirv.push_back((4 << 16) | 71);
            spirv.push_back(varId);
            spirv.push_back(34);
            spirv.push_back(bindings[i].DescriptorSet);
        }
    }

    spirv.push_back((2 << 16) | 19);
    spirv.push_back(2);

    spirv.push_back((3 << 16) | 33);
    spirv.push_back(3);
    spirv.push_back(2);

    spirv.push_back((3 << 16) | 22);
    spirv.push_back(4);
    spirv.push_back(32);

    spirv.push_back((4 << 16) | 23);
    spirv.push_back(5);
    spirv.push_back(4);
    spirv.push_back(4);

    spirv.push_back((4 << 16) | 43);
    spirv.push_back(4);
    spirv.push_back(6);
    spirv.push_back(0x00000000);

    spirv.push_back((4 << 16) | 43);
    spirv.push_back(4);
    spirv.push_back(7);
    spirv.push_back(0x3F800000);

    spirv.push_back((7 << 16) | 44);
    spirv.push_back(5);
    spirv.push_back(8);
    spirv.push_back(6);
    spirv.push_back(6);
    spirv.push_back(6);
    spirv.push_back(6);

    spirv.push_back((4 << 16) | 21);
    spirv.push_back(9);
    spirv.push_back(32);
    spirv.push_back(0);

    spirv.push_back((4 << 16) | 43);
    spirv.push_back(9);
    spirv.push_back(10);
    spirv.push_back(0x00000000);

    spirv.push_back((4 << 16) | 43);
    spirv.push_back(9);
    spirv.push_back(11);
    spirv.push_back(0x00000001);

    spirv.push_back((4 << 16) | 23);
    spirv.push_back(12);
    spirv.push_back(4);
    spirv.push_back(2);

    spirv.push_back((5 << 16) | 44);
    spirv.push_back(12);
    spirv.push_back(13);
    spirv.push_back(6);
    spirv.push_back(6);

    spirv.push_back((2 << 16) | 20);
    spirv.push_back(14);

    spirv.push_back((3 << 16) | 41);
    spirv.push_back(14);
    spirv.push_back(15);

    spirv.push_back((9 << 16) | 25);
    spirv.push_back(16);
    spirv.push_back(4);
    spirv.push_back(1);
    spirv.push_back(0);
    spirv.push_back(0);
    spirv.push_back(0);
    spirv.push_back(1);
    spirv.push_back(0);

    spirv.push_back((3 << 16) | 27);
    spirv.push_back(17);
    spirv.push_back(16);

    spirv.push_back((9 << 16) | 25);
    spirv.push_back(18);
    spirv.push_back(4);
    spirv.push_back(1);
    spirv.push_back(0);
    spirv.push_back(0);
    spirv.push_back(0);
    spirv.push_back(2);
    spirv.push_back(0);

    spirv.push_back((4 << 16) | 32);
    spirv.push_back(19);
    spirv.push_back(0);
    spirv.push_back(17);

    spirv.push_back((4 << 16) | 32);
    spirv.push_back(20);
    spirv.push_back(0);
    spirv.push_back(18);

    spirv.push_back((4 << 16) | 32);
    spirv.push_back(21);
    spirv.push_back(2);
    spirv.push_back(9);

    spirv.push_back((4 << 16) | 32);
    spirv.push_back(22);
    spirv.push_back(12);
    spirv.push_back(9);

    if (needsDefaultSampler) {
        spirv.push_back((4 << 16) | 59);
        spirv.push_back(19);
        spirv.push_back(baseVarId);
        spirv.push_back(0);
    } else {
        for (size_t i = 0; i < bindings.size(); ++i) {
            uint32_t varId = baseVarId + static_cast<uint32_t>(i);
            uint32_t ptrType = 21;
            uint32_t storageClass = 2;
            if (bindings[i].DescriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                ptrType = 19;
                storageClass = 0;
            } else if (bindings[i].DescriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
                       bindings[i].DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
                ptrType = 20;
                storageClass = 0;
            } else if (bindings[i].DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
                ptrType = 22;
                storageClass = 12;
            }
            spirv.push_back((4 << 16) | 59);
            spirv.push_back(ptrType);
            spirv.push_back(varId);
            spirv.push_back(storageClass);
        }
    }

    spirv.push_back((5 << 16) | 54);
    spirv.push_back(2);
    spirv.push_back(1);
    spirv.push_back(0);
    spirv.push_back(3);

    uint32_t varCount = needsDefaultSampler ? 1 : static_cast<uint32_t>(bindings.size());
    uint32_t labelId = baseVarId + varCount;
    spirv.push_back((2 << 16) | 248);
    spirv.push_back(labelId);

    uint32_t nextId = labelId + 1;
    for (const auto& instr : instructions) {
        if (instr.IsEndProgram) {
            break;
        }

        if (instr.TargetSpirvOp == DecodedSpirvOp::FAdd) {
            spirv.push_back((5 << 16) | 129);
            spirv.push_back(4);
            spirv.push_back(nextId++);
            spirv.push_back(6);
            spirv.push_back(7);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::FSub) {
            spirv.push_back((5 << 16) | 131);
            spirv.push_back(4);
            spirv.push_back(nextId++);
            spirv.push_back(7);
            spirv.push_back(6);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::FMul) {
            spirv.push_back((5 << 16) | 133);
            spirv.push_back(4);
            spirv.push_back(nextId++);
            spirv.push_back(7);
            spirv.push_back(7);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::FDiv) {
            spirv.push_back((5 << 16) | 135);
            spirv.push_back(4);
            spirv.push_back(nextId++);
            spirv.push_back(7);
            spirv.push_back(7);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::FMod) {
            spirv.push_back((5 << 16) | 141);
            spirv.push_back(4);
            spirv.push_back(nextId++);
            spirv.push_back(7);
            spirv.push_back(7);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::UDiv) {
            spirv.push_back((5 << 16) | 134);
            spirv.push_back(9);
            spirv.push_back(nextId++);
            spirv.push_back(11);
            spirv.push_back(11);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::Dot) {
            spirv.push_back((5 << 16) | 148);
            spirv.push_back(4);
            spirv.push_back(nextId++);
            spirv.push_back(8);
            spirv.push_back(8);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::IAdd) {
            spirv.push_back((5 << 16) | 128);
            spirv.push_back(9);
            spirv.push_back(nextId++);
            spirv.push_back(10);
            spirv.push_back(11);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::ISub) {
            spirv.push_back((5 << 16) | 130);
            spirv.push_back(9);
            spirv.push_back(nextId++);
            spirv.push_back(11);
            spirv.push_back(10);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::IMul) {
            spirv.push_back((5 << 16) | 132);
            spirv.push_back(9);
            spirv.push_back(nextId++);
            spirv.push_back(11);
            spirv.push_back(11);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::Select) {
            spirv.push_back((6 << 16) | 169);
            spirv.push_back(4);
            spirv.push_back(nextId++);
            spirv.push_back(15);
            spirv.push_back(7);
            spirv.push_back(6);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::ImageSample) {
            uint32_t samplerVarId = baseVarId;
            for (size_t b = 0; b < bindings.size(); ++b) {
                if (bindings[b].DescriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
                    bindings[b].DescriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) {
                    samplerVarId = baseVarId + static_cast<uint32_t>(b);
                    break;
                }
            }
            uint32_t loadedImageId = nextId++;
            spirv.push_back((4 << 16) | 61);
            spirv.push_back(17);
            spirv.push_back(loadedImageId);
            spirv.push_back(samplerVarId);

            spirv.push_back((5 << 16) | 87);
            spirv.push_back(5);
            spirv.push_back(nextId++);
            spirv.push_back(loadedImageId);
            spirv.push_back(13);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::ImageRead) {
            uint32_t imageVarId = baseVarId;
            for (size_t b = 0; b < bindings.size(); ++b) {
                if (bindings[b].DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
                    bindings[b].DescriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) {
                    imageVarId = baseVarId + static_cast<uint32_t>(b);
                    break;
                }
            }
            uint32_t loadedImageId = nextId++;
            spirv.push_back((4 << 16) | 61);
            spirv.push_back(18);
            spirv.push_back(loadedImageId);
            spirv.push_back(imageVarId);

            spirv.push_back((5 << 16) | 98);
            spirv.push_back(5);
            spirv.push_back(nextId++);
            spirv.push_back(loadedImageId);
            spirv.push_back(10);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::ImageWrite) {
            uint32_t imageVarId = baseVarId;
            for (size_t b = 0; b < bindings.size(); ++b) {
                if (bindings[b].DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
                    imageVarId = baseVarId + static_cast<uint32_t>(b);
                    break;
                }
            }
            uint32_t loadedImageId = nextId++;
            spirv.push_back((4 << 16) | 61);
            spirv.push_back(18);
            spirv.push_back(loadedImageId);
            spirv.push_back(imageVarId);

            spirv.push_back((4 << 16) | 99);
            spirv.push_back(loadedImageId);
            spirv.push_back(10);
            spirv.push_back(8);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::Load) {
            spirv.push_back((4 << 16) | 61);
            spirv.push_back(9);
            spirv.push_back(nextId++);
            spirv.push_back(baseVarId);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::Store) {
            spirv.push_back((3 << 16) | 62);
            spirv.push_back(baseVarId);
            spirv.push_back(10);
        }
    }

    spirv.push_back((1 << 16) | 253);

    spirv.push_back((1 << 16) | 56);

    spirv[3] = nextId;

    return spirv;
}

} // namespace Aot::Graphics
