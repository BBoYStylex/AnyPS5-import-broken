#include <graphics/ConsoleShaderExtractor.hpp>
#include <graphics/ShaderInstructionDecoder.hpp>
#include <graphics/ShaderTranspiler.hpp>
#include <graphics/GraphicsDriver.hpp>
#include <graphics/VulkanPresentationHook.hpp>
#include <cassert>
#include <vector>

static uint64_t gMockPresentCallCount = 0;
static VkResult MockVkQueuePresentKHR(VkQueue, const VkPresentInfoKHR*) {
    ++gMockPresentCallCount;
    return VK_SUCCESS;
}

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

    bool foundEntryPoint = false;
    bool foundTypeFloat = false;
    bool foundTypeVector = false;

    for (size_t i = 5; i < spirv.size(); ) {
        uint32_t word = spirv[i];
        uint16_t op = word & 0xFFFF;
        uint16_t len = word >> 16;
        if (len == 0) break;

        if (op == 15) {
            assert(len == 5);
            foundEntryPoint = true;
        } else if (op == 22) {
            assert(len == 3);
            foundTypeFloat = true;
        } else if (op == 23) {
            assert(len == 4);
            foundTypeVector = true;
        }
        i += len;
    }
    if (!foundEntryPoint || !foundTypeFloat || !foundTypeVector) {
        return 1;
    }

    Aot::Graphics::ExtractedShaderPackage pkg{};
    pkg.Stage = Aot::Graphics::ConsoleShaderStage::Vertex;
    pkg.RawBytecode = dummyBytecode;

    Aot::Graphics::ShaderTranspiler transpiler;
    auto resultOpt = transpiler.Transpile(pkg);
    if (!resultOpt.has_value() || resultOpt->SpirvBytecode[0] != 0x07230203) {
        return 2;
    }

    std::vector<uint8_t> multiBytecode = {
        0x02, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x18,
        0x00, 0x00, 0x00, 0x1A,
        0x00, 0x00, 0x80, 0xBF
    };
    auto multiDecoded = decoder.DecodeBytecode(multiBytecode);
    if (multiDecoded.size() != 4 || multiDecoded[0].OpClass != Aot::Graphics::ShaderOpcodeClass::VectorAlu) {
        return 3;
    }

    auto multiSpirv = decoder.GenerateSpirvFromInstructions(multiDecoded, 0);
    if (multiSpirv.size() <= 5) {
        return 4;
    }

    Aot::Graphics::ExtractedShaderPackage multiPkg{};
    multiPkg.Stage = Aot::Graphics::ConsoleShaderStage::Vertex;
    multiPkg.RawBytecode = multiBytecode;
    auto multiTranspiled = transpiler.Transpile(multiPkg);
    if (!multiTranspiled.has_value()) {
        return 5;
    }

    bool foundFAdd = false;
    bool foundFMod = false;
    bool foundUDiv = false;
    for (size_t i = 5; i < multiTranspiled->SpirvBytecode.size(); ) {
        uint32_t word = multiTranspiled->SpirvBytecode[i];
        uint16_t op = word & 0xFFFF;
        uint16_t len = word >> 16;
        if (len == 0) break;
        if (op == 129) {
            foundFAdd = true;
        } else if (op == 141) {
            foundFMod = true;
        } else if (op == 134) {
            foundUDiv = true;
        }
        i += len;
    }
    if (!foundFAdd || !foundFMod || !foundUDiv) {
        return 6;
    }

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
    if (!bindingResult.has_value() || bindingResult->Bindings.size() != 1) {
        return 7;
    }

    bool foundOpVariable = false;
    bool foundOpDecorateBinding = false;
    for (size_t i = 5; i < bindingResult->SpirvBytecode.size(); ) {
        uint32_t word = bindingResult->SpirvBytecode[i];
        uint16_t op = word & 0xFFFF;
        uint16_t len = word >> 16;
        if (len == 0) break;
        if (op == 59) {
            foundOpVariable = true;
        } else if (op == 71 && len >= 4) {
            if (bindingResult->SpirvBytecode[i + 2] == 33 && bindingResult->SpirvBytecode[i + 3] == 1) {
                foundOpDecorateBinding = true;
            }
        }
        i += len;
    }
    if (!foundOpDecorateBinding || !foundOpVariable) {
        return 8;
    }

    std::vector<uint8_t> samplerBytecode = {
        0x00, 0x00, 0x00, 0xF0,
        0x00, 0x00, 0x81, 0xBF
    };
    Aot::Graphics::ExtractedShaderPackage samplerPkg{};
    samplerPkg.Stage = Aot::Graphics::ConsoleShaderStage::Fragment;
    samplerPkg.RawBytecode = samplerBytecode;
    samplerPkg.ResourceBindings.push_back(Aot::Graphics::ResourceBindingInfo{
        .Binding = 0,
        .DescriptorSet = 0,
        .DescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .Count = 1
    });

    auto samplerResult = transpiler.Transpile(samplerPkg);
    if (!samplerResult.has_value() || samplerResult->Bindings.empty()) {
        return 19;
    }

    bool foundOpTypeImage = false;
    bool foundOpTypeSampledImage = false;
    bool foundOpImageSample = false;
    for (size_t i = 5; i < samplerResult->SpirvBytecode.size(); ) {
        uint32_t word = samplerResult->SpirvBytecode[i];
        uint16_t op = word & 0xFFFF;
        uint16_t len = word >> 16;
        if (len == 0) break;
        if (op == 25) {
            foundOpTypeImage = true;
        } else if (op == 27) {
            foundOpTypeSampledImage = true;
        } else if (op == 87) {
            foundOpImageSample = true;
        }
        i += len;
    }
    if (!foundOpTypeImage || !foundOpTypeSampledImage || !foundOpImageSample) {
        return 20;
    }

    std::vector<uint8_t> intAluBytecode = {
        0x00, 0x00, 0x00, 0x80,
        0x00, 0x00, 0x80, 0x80,
        0x00, 0x00, 0x00, 0x81,
        0x00, 0x00, 0x80, 0x81,
        0x00, 0x00, 0x81, 0xBF
    };
    Aot::Graphics::ExtractedShaderPackage intPkg{};
    intPkg.Stage = Aot::Graphics::ConsoleShaderStage::Compute;
    intPkg.RawBytecode = intAluBytecode;
    auto intResult = transpiler.Transpile(intPkg);
    if (!intResult.has_value()) {
        return 21;
    }

    bool foundIAdd = false;
    bool foundISub = false;
    bool foundIMul = false;
    bool foundSelect = false;
    for (size_t i = 5; i < intResult->SpirvBytecode.size(); ) {
        uint32_t word = intResult->SpirvBytecode[i];
        uint16_t op = word & 0xFFFF;
        uint16_t len = word >> 16;
        if (len == 0) break;
        if (op == 128) {
            foundIAdd = true;
        } else if (op == 130) {
            foundISub = true;
        } else if (op == 132) {
            foundIMul = true;
        } else if (op == 169) {
            foundSelect = true;
        }
        i += len;
    }
    if (!foundIAdd || !foundISub || !foundIMul || !foundSelect) {
        return 22;
    }

    std::vector<uint8_t> storageImgBytecode = {
        0x00, 0x00, 0x00, 0xF4,
        0x00, 0x00, 0x00, 0xF8,
        0x00, 0x00, 0x81, 0xBF
    };
    Aot::Graphics::ExtractedShaderPackage storagePkg{};
    storagePkg.Stage = Aot::Graphics::ConsoleShaderStage::Compute;
    storagePkg.RawBytecode = storageImgBytecode;
    storagePkg.ResourceBindings.push_back(Aot::Graphics::ResourceBindingInfo{
        .Binding = 2,
        .DescriptorSet = 0,
        .DescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .Count = 1
    });
    auto storageResult = transpiler.Transpile(storagePkg);
    if (!storageResult.has_value()) {
        return 23;
    }

    bool foundImageRead = false;
    bool foundImageWrite = false;
    for (size_t i = 5; i < storageResult->SpirvBytecode.size(); ) {
        uint32_t word = storageResult->SpirvBytecode[i];
        uint16_t op = word & 0xFFFF;
        uint16_t len = word >> 16;
        if (len == 0) break;
        if (op == 98) {
            foundImageRead = true;
        } else if (op == 99) {
            foundImageWrite = true;
        }
        i += len;
    }
    if (!foundImageRead || !foundImageWrite) {
        return 24;
    }

    Aot::Graphics::ExtractedShaderPackage computePkg{};
    computePkg.Stage = Aot::Graphics::ConsoleShaderStage::Compute;
    computePkg.RawBytecode = dummyBytecode;
    auto computeResult = transpiler.Transpile(computePkg);
    if (!computeResult.has_value()) {
        return 25;
    }

    bool foundLocalSize = false;
    for (size_t i = 5; i < computeResult->SpirvBytecode.size(); ) {
        uint32_t word = computeResult->SpirvBytecode[i];
        uint16_t op = word & 0xFFFF;
        uint16_t len = word >> 16;
        if (len == 0) break;
        if (op == 16 && len == 6 && computeResult->SpirvBytecode[i + 2] == 17) {
            foundLocalSize = true;
        }
        i += len;
    }
    if (!foundLocalSize) {
        return 26;
    }

    Aot::Graphics::GraphicsDriver driver;
    Aot::Graphics::GraphicsProcessingOptions options{};
    options.WarmupPipelines = true;
    auto summary = driver.ProcessAssets(options);
    assert(summary.ShadersExtracted == 0);
    assert(summary.ShadersTranspiled == 0);

    auto& hook = Aot::Graphics::VulkanPresentationHook::Instance();
    hook.ResetMetrics();
    if (hook.GetTotalPresentedFrames() != 0 || hook.GetTargetPresentFunction() != nullptr) {
        return 9;
    }

    hook.SetTargetPresentFunction(&MockVkQueuePresentKHR);
    if (hook.GetTargetPresentFunction() != &MockVkQueuePresentKHR) {
        return 10;
    }

    uint64_t callbackInvokedCount = 0;
    Aot::Graphics::FramePresentationMetrics lastMetrics{};
    hook.RegisterCallback([&](const Aot::Graphics::FramePresentationMetrics& m) {
        ++callbackInvokedCount;
        lastMetrics = m;
    });

    VkSwapchainKHR mockSwapchain = reinterpret_cast<VkSwapchainKHR>(static_cast<uintptr_t>(0x1234));
    uint32_t mockImageIndex = 2;
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &mockSwapchain;
    presentInfo.pImageIndices = &mockImageIndex;

    VkResult res = hook.Present(VK_NULL_HANDLE, &presentInfo);
    if (res != VK_SUCCESS || gMockPresentCallCount != 1 || callbackInvokedCount != 1) {
        return 11;
    }
    if (lastMetrics.FrameIndex != 1 || lastMetrics.Swapchain != mockSwapchain || lastMetrics.ImageIndex != mockImageIndex) {
        return 12;
    }
    if (lastMetrics.PresentResult != VK_SUCCESS || lastMetrics.CaptureRequested) {
        return 13;
    }

    hook.RequestCaptureNextFrame();
    if (!hook.IsCaptureRequested()) {
        return 14;
    }

    res = hook.Present(VK_NULL_HANDLE, &presentInfo);
    if (res != VK_SUCCESS || gMockPresentCallCount != 2 || callbackInvokedCount != 2) {
        return 15;
    }
    if (lastMetrics.FrameIndex != 2 || !lastMetrics.CaptureRequested || hook.IsCaptureRequested()) {
        return 16;
    }

    hook.ClearCallbacks();
    res = hook.Present(VK_NULL_HANDLE, &presentInfo);
    if (res != VK_SUCCESS || gMockPresentCallCount != 3 || callbackInvokedCount != 2) {
        return 17;
    }
    if (hook.GetTotalPresentedFrames() != 3) {
        return 18;
    }

    return 0;
}
