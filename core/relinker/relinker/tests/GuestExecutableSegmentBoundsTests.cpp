#include "ElfFixture.hpp"
#include "RelinkerProcess.hpp"
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace RelinkerTests;

void require(const bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

const Bytes kMainCode = {0xC3};
const Bytes kModuleCode = {0xC3};

void RunGuestAndRequireRejected(const std::string& binary, const Bytes& moduleImage, const std::string& expectedError) {
    const TempDirectory directory;
    const auto input = directory.Path() / "input.elf";
    const auto output = directory.Path() / "output.elf";
    const auto modules = directory.Path() / "sce_module";
    std::filesystem::create_directory(modules);
    WriteFile(input, MakeExecutable(kMainCode));
    WriteFile(modules / "sample.prx", moduleImage);

    const auto run = RunRelinker(binary, {input.string(), output.string()}, directory.Path() / "relinker.log");
    require(run.ExitCode != 0, "Relinker accepted invalid guest module and exited 0:\n" + run.Output);
    require(run.Output.find(expectedError) != std::string::npos,
            "Relinker did not report expected error '" + expectedError + "':\n" + run.Output);
}

}

int main(const int argc, char** argv) {
    try {
        require(argc == 2, "usage: guest_executable_segment_bounds_tests <relinker>");
        const std::string binary = argv[1];

        {
            Bytes module = MakeModule(kModuleCode);
            WriteProgramHeader(module, 0, 1, 5, kCodeOffset, 0, module.size() + 0x1000, 0x4000);
            RunGuestAndRequireRejected(binary, module, "Guest executable segment exceeds image");
        }

        {
            Bytes module = MakeModule(kModuleCode);
            WriteProgramHeader(module, 0, 1, 5, module.size() + 0x1000, 0, kCodeSize, 0x4000);
            RunGuestAndRequireRejected(binary, module, "Guest executable segment exceeds image");
        }

        {
            Bytes module = MakeModule(kModuleCode);
            WriteProgramHeader(module, 0, 1, 5, kCodeOffset, 0xFFFFFFFFFFFFF000ull, 0x2000, 0x4000);
            RunGuestAndRequireRejected(binary, module, "Guest executable segment range exceeds address space");
        }

        std::cout << "Guest executable segment bounds tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "guest_executable_segment_bounds_tests: " << error.what() << '\n';
        return 1;
    }
}
