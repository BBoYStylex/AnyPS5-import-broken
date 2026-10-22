#ifndef CORE_AOT_GRAPHICS_SHADERINSTRUCTIONDECODER_HPP
#define CORE_AOT_GRAPHICS_SHADERINSTRUCTIONDECODER_HPP

#include <cstdint>
#include <vector>
#include <span>
#include <string>

namespace Aot::Graphics {

enum class ShaderOpcodeClass : uint8_t {
    ScalarAlu,
    VectorAlu,
    VectorMemory,
    ScalarMemory,
    FlowControl,
    Unknown
};

enum class DecodedSpirvOp : uint16_t {
    None = 0,
    IAdd = 128,
    FAdd = 129,
    ISub = 130,
    FSub = 131,
    IMul = 132,
    FMul = 133,
    UDiv = 134,
    FDiv = 135,
    FMod = 141,
    Dot = 148,
    Select = 169,
    Load = 61,
    Store = 62,
    ImageSample = 87,
    Branch = 249,
    BranchConditional = 250,
    Return = 253
};

struct DecodedShaderInstruction {
    uint32_t RawWord;
    ShaderOpcodeClass OpClass;
    DecodedSpirvOp TargetSpirvOp;
    uint32_t DstRegister;
    uint32_t Src0Register;
    uint32_t Src1Register;
    bool IsEndProgram;
};

class ShaderInstructionDecoder {
public:
    ShaderInstructionDecoder();
    ~ShaderInstructionDecoder();

    [[nodiscard]] std::vector<DecodedShaderInstruction> DecodeBytecode(std::span<const uint8_t> bytecode);
    [[nodiscard]] std::vector<uint32_t> GenerateSpirvFromInstructions(
        const std::vector<DecodedShaderInstruction>& instructions,
        uint32_t executionModel
    );
};

} // namespace Aot::Graphics
#endif
