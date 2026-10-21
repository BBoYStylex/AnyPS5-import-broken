#include <graphics/GraphicsDriver.hpp>
#include <iostream>
#include <string_view>

static void PrintUsage(const char* executableName) {
    std::cout << "Usage: " << executableName << " [options]\n"
              << "Options:\n"
              << "  -i, --input <path>      Input shader asset package\n"
              << "  -o, --out-dir <dir>     Output directory for transpiled SPIR-V\n"
              << "  -c, --cache <path>      Output Vulkan pipeline cache file\n"
              << "  -w, --warmup            Pre-compile and warm up pipelines\n"
              << "  -h, --help              Display this help message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        PrintUsage(argv[0]);
        return 1;
    }

    Aot::Graphics::GraphicsProcessingOptions options{};

    for (int i = 1; i < argc; ++i) {
        std::string_view argument(argv[i]);

        if ((argument == "-i" || argument == "--input") && i + 1 < argc) {
            options.InputAssetPath = argv[++i];
        } else if ((argument == "-o" || argument == "--out-dir") && i + 1 < argc) {
            options.OutputSpirvDirectory = argv[++i];
        } else if ((argument == "-c" || argument == "--cache") && i + 1 < argc) {
            options.OutputCachePath = argv[++i];
        } else if (argument == "-w" || argument == "--warmup") {
            options.WarmupPipelines = true;
        } else if (argument == "-h" || argument == "--help") {
            PrintUsage(argv[0]);
            return 0;
        }
    }

    if (options.InputAssetPath.empty()) {
        std::cerr << "Error: No input asset package specified.\n";
        return 1;
    }

    Aot::Graphics::GraphicsDriver driver;
    auto summary = driver.ProcessAssets(options);

    std::cout << "Shader processing completed:\n"
              << "  Shaders extracted: " << summary.ShadersExtracted << "\n"
              << "  Shaders transpiled to SPIR-V: " << summary.ShadersTranspiled << "\n"
              << "  Pipelines compiled: " << summary.PipelinesCompiled << "\n";

    return 0;
}
