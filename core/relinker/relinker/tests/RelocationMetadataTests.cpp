#include <relinker/analysis/UnusedNidFilter/IRelativeRelocationIndex.hpp>
#include <elfpatcher/general/ElfConstants.hpp>
#include <io/BufferUtils.hpp>
#include <iostream>
#include <stdexcept>

using namespace Elfpatcher;
static void Require(bool ok) { if (!ok) throw std::runtime_error("Relocation metadata validation failed"); }

static std::vector<std::uint8_t> Fixture(bool plt, std::uint64_t strideTag, std::uint64_t stride, std::uint64_t pltTag, std::uint64_t pltType) {
    std::vector<std::uint8_t> b(1024, 0);
    b[0] = 0x7F; b[1] = 'E'; b[2] = 'L'; b[3] = 'F'; b[4] = 2; b[5] = 1; b[6] = 1;
    Io::WriteU64(b, 32, 64); Io::WriteU16(b, 54, 56); Io::WriteU16(b, 56, 2);
    Io::WriteU32(b, 64, PT_LOAD); Io::WriteU64(b, 80, 0x1000); Io::WriteU64(b, 96, b.size());
    Io::WriteU32(b, 120, PT_DYNAMIC); Io::WriteU64(b, 128, 256); Io::WriteU64(b, 152, 80);
    Io::WriteU64(b, 256, plt ? DT_JMPREL : DT_RELA); Io::WriteU64(b, 264, 0x1200);
    Io::WriteU64(b, 272, plt ? DT_PLTRELSZ : DT_RELASZ); Io::WriteU64(b, 280, 24);
    Io::WriteU64(b, 288, strideTag); Io::WriteU64(b, 296, stride);
    Io::WriteU64(b, 304, pltTag); Io::WriteU64(b, 312, pltType);
    Io::WriteU64(b, 512, 0x1400); Io::WriteU64(b, 520, R_X86_64_RELATIVE); Io::WriteU64(b, 528, 0x1500);
    return b;
}

int main() {
    try {
        for (const auto strideTag : {DT_RELAENT, DT_OS_RELAENT}) {
            for (const bool plt : {false, true}) {
                const auto valid = Fixture(plt, strideTag, 24, DT_PLTREL, DT_RELA);
                Require(Relinker::UnusedNidFilter::BuildRelativeRelocationIndex(valid)->TargetOfSlot(0x1400) == 0x1500);
                for (const auto stride : {0, 16, 32}) {
                    bool rejected = false;
                    try { Relinker::UnusedNidFilter::BuildRelativeRelocationIndex(Fixture(plt, strideTag, stride, DT_PLTREL, DT_RELA)); }
                    catch (const std::exception&) { rejected = true; }
                    Require(rejected);
                }
            }
        }
        for (const auto tag : {DT_PLTREL, DT_OS_PLTREL}) {
            bool rejected = false;
            try { Relinker::UnusedNidFilter::BuildRelativeRelocationIndex(Fixture(true, DT_RELAENT, 24, tag, 17)); }
            catch (const std::exception&) { rejected = true; }
            Require(rejected);
        }
        auto partial = Fixture(false, DT_RELAENT, 24, DT_PLTREL, DT_RELA);
        Io::WriteU64(partial, 152, 79);
        bool rejected = false;
        try { Relinker::UnusedNidFilter::BuildRelativeRelocationIndex(partial); }
        catch (const std::exception&) { rejected = true; }
        Require(rejected);
        std::cout << "19 relocation metadata cases passed\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
