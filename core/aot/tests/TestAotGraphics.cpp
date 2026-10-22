#include <graphics/ConsoleShaderExtractor.hpp>
#include <graphics/ShaderInstructionDecoder.hpp>
#include <graphics/ShaderTranspiler.hpp>
#include <graphics/GraphicsDriver.hpp>
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

    std::vector<uint8_t> multiBytecode = {
        0x02, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x80, 0xBF
    };
    auto multiDecoded = decoder.DecodeBytecode(multiBytecode);
    assert(multiDecoded.size() == 2);
    assert(multiDecoded[0].OpClass == Aot::Graphics::ShaderOpcodeClass::VectorAlu);

    auto multiSpirv = decoder.GenerateSpirvFromInstructions(multiDecoded, 0);
    assert(multiSpirv.size() > 5);

    Aot::Graphics::ExtractedShaderPackage bindingPkg{};
    bindingPkg.Stage = Aot::Graphics::ConsoleShaderStage::Fragment;
    bindingPkg.RawBytecode = dummyBytecode;
    bindingPkg.ResourceBindings.push_back(Aot::Graphics::ResourceBindingInfo{
        .Binding = 1,
        .DescriptorSet = 0,
        .DescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .Count = 1
    });

    auto bindingResult = transpiler.Transpile(bindingPkg);
    assert(bindingResult.has_value());
    assert(bindingResult->Bindings.size() == 1);
    assert(bindingResult->SpirvBytecode[0] == 0x07230203);

    Aot::Graphics::GraphicsDriver driver;
    Aot::Graphics::GraphicsProcessingOptions options{};
    options.WarmupPipelines = true;
    auto summary = driver.ProcessAssets(options);
    assert(summary.ShadersExtracted == 0);
    assert(summary.ShadersTranspiled == 0);

    return 0;
}
