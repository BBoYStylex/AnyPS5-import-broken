#include <compiler/AotCompilerDriver.hpp>
#include <iostream>
#include <string_view>

static void PrintUsage(const char* executableName) {
    std::cout << "Usage: " << executableName << " [options]\n"
              << "Options:\n"
              << "  -i, --input <path>      Input console ELF binary\n"
              << "  -o, --output <path>     Output native object file\n"
              << "  -O, --opt <0-3>         Optimization level (default: 2)\n"
              << "  --triple <target>       Target LLVM triple\n"
              << "  -h, --help              Display this help message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        PrintUsage(argv[0]);
        return 1;
    }

    Aot::Compiler::CompilerOptions options{};

    for (int i = 1; i < argc; ++i) {
        std::string_view argument(argv[i]);

        if ((argument == "-i" || argument == "--input") && i + 1 < argc) {
            options.InputExecutablePath = argv[++i];
        } else if ((argument == "-o" || argument == "--output") && i + 1 < argc) {
            options.OutputObjectPath = argv[++i];
        } else if ((argument == "-O" || argument == "--opt") && i + 1 < argc) {
            options.OptimizationLevel = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if (argument == "--triple" && i + 1 < argc) {
            options.TargetTriple = argv[++i];
        } else if (argument == "-h" || argument == "--help") {
            PrintUsage(argv[0]);
            return 0;
        }
    }

    if (options.InputExecutablePath.empty()) {
        std::cerr << "Error: No input executable specified.\n";
        return 1;
    }

    if (options.OutputObjectPath.empty()) {
        options.OutputObjectPath = options.InputExecutablePath.stem().string() + ".obj";
    }

    Aot::Compiler::AotCompilerDriver driver;
    auto statistics = driver.Compile(options);

    if (!statistics.Success) {
        std::cerr << "Error: AOT compilation failed.\n";
        return 1;
    }

    std::cout << "AOT Compilation succeeded:\n"
              << "  Sections parsed: " << statistics.TotalSectionsParsed << "\n"
              << "  Functions lifted: " << statistics.FunctionsLifted << "\n"
              << "  Syscall pass applied: " << (statistics.SyscallPassModified ? "yes" : "no") << "\n"
              << "  Output: " << options.OutputObjectPath.string() << "\n";

    return 0;
}
