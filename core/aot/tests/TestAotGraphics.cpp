#include <graphics/ConsoleShaderExtractor.hpp>
#include <graphics/ShaderInstructionDecoder.hpp>
#include <graphics/ShaderTranspiler.hpp>
#include <cassert>
#include <vector>

int main() {
    Aot::Graphics::ConsoleShaderExtractor extractor;
    auto extracted = extractor.ExtractFromMemory({});
    assert(extracted.empty());

    Aot::Graphics::ShaderInstructionDecoder decoder;
    std::vector<uint8_t> dummyBytecode = {
        0x00, 0x00, 0x80, 0xBF
    };
    auto decodedInstructions = decoder.DecodeBytecode(dummyBytecode);
    assert(decodedInstructions.size() == 1);
    assert(decodedInstructions[0].IsEndProgram);

    auto spirv = decoder.GenerateSpirvFromInstructions(decodedInstructions, 0);
    assert(spirv.size() >= 5);
    assert(spirv[0] == 0x07230203);

    Aot::Graphics::ExtractedShaderPackage pkg{};
    pkg.Stage = Aot::Graphics::ConsoleShaderStage::Vertex;
    pkg.RawBytecode = dummyBytecode;

    Aot::Graphics::ShaderTranspiler transpiler;
    auto resultOpt = transpiler.Transpile(pkg);
    assert(resultOpt.has_value());
    assert(resultOpt->SpirvBytecode[0] == 0x07230203);

    return 0;
}
