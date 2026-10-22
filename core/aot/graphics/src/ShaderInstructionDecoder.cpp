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
            } else {
                instr.TargetSpirvOp = DecodedSpirvOp::FAdd;
            }
            instr.DstRegister = (raw >> 17) & 0xFF;
            instr.Src0Register = raw & 0x1FF;
            instr.Src1Register = (raw >> 9) & 0xFF;
        } else if (topBits >= 0x3C) {
            instr.OpClass = ShaderOpcodeClass::VectorMemory;
            instr.TargetSpirvOp = DecodedSpirvOp::ImageSample;
            instr.DstRegister = (raw >> 16) & 0xFF;
            instr.Src0Register = raw & 0xFF;
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
    uint32_t executionModel) {
    std::vector<uint32_t> spirv;

    spirv.push_back(0x07230203);
    spirv.push_back(0x00010300);
    spirv.push_back(0x00000000);
    spirv.push_back(16);
    spirv.push_back(0x00000000);

    spirv.push_back((2 << 16) | 17);
    spirv.push_back(1);

    spirv.push_back((3 << 16) | 14);
    spirv.push_back(0);
    spirv.push_back(1);

    spirv.push_back((6 << 16) | 15);
    spirv.push_back(executionModel);
    spirv.push_back(1);
    spirv.push_back(0x6e69616d);
    spirv.push_back(0x00000000);
    spirv.push_back(0);

    if (executionModel == 4) {
        spirv.push_back((3 << 16) | 16);
        spirv.push_back(1);
        spirv.push_back(7);
    }

    spirv.push_back((2 << 16) | 19);
    spirv.push_back(2);

    spirv.push_back((3 << 16) | 33);
    spirv.push_back(3);
    spirv.push_back(2);

    spirv.push_back((3 << 16) | 21);
    spirv.push_back(4);
    spirv.push_back(32);

    spirv.push_back((4 << 16) | 22);
    spirv.push_back(5);
    spirv.push_back(4);
    spirv.push_back(4);

    spirv.push_back((5 << 16) | 54);
    spirv.push_back(2);
    spirv.push_back(1);
    spirv.push_back(0);
    spirv.push_back(3);

    spirv.push_back((2 << 16) | 248);
    spirv.push_back(6);

    uint32_t nextId = 7;
    for (const auto& instr : instructions) {
        if (instr.IsEndProgram) {
            break;
        }

        if (instr.TargetSpirvOp == DecodedSpirvOp::FAdd) {
            spirv.push_back((5 << 16) | 129);
            spirv.push_back(4);
            spirv.push_back(nextId++);
            spirv.push_back(4);
            spirv.push_back(4);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::FSub) {
            spirv.push_back((5 << 16) | 131);
            spirv.push_back(4);
            spirv.push_back(nextId++);
            spirv.push_back(4);
            spirv.push_back(4);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::FMul) {
            spirv.push_back((5 << 16) | 133);
            spirv.push_back(4);
            spirv.push_back(nextId++);
            spirv.push_back(4);
            spirv.push_back(4);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::FDiv) {
            spirv.push_back((5 << 16) | 135);
            spirv.push_back(4);
            spirv.push_back(nextId++);
            spirv.push_back(4);
            spirv.push_back(4);
        } else if (instr.TargetSpirvOp == DecodedSpirvOp::Dot) {
            spirv.push_back((5 << 16) | 148);
            spirv.push_back(4);
            spirv.push_back(nextId++);
            spirv.push_back(5);
            spirv.push_back(5);
        }
    }

    if (nextId >= 16) {
        spirv[3] = nextId + 1;
    }

    spirv.push_back((1 << 16) | 253);

    spirv.push_back((1 << 16) | 56);

    return spirv;
}

} // namespace Aot::Graphics
