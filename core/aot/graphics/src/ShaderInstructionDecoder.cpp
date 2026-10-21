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
        } else {
            instr.OpClass = ShaderOpcodeClass::ScalarAlu;
            instr.TargetSpirvOp = DecodedSpirvOp::Load;
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

    for (const auto& instr : instructions) {
        if (instr.IsEndProgram) {
            break;
        }
    }

    spirv.push_back((1 << 16) | 253);

    spirv.push_back((1 << 16) | 56);

    return spirv;
}

} // namespace Aot::Graphics
