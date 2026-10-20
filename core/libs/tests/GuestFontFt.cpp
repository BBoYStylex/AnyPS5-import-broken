#include "prx/libc/include/general/VabiMacros.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

extern "C" {
int APS5_VABI sceFontFtSupportFontFormats();
int APS5_VABI sceFontFtSupportSystemFonts();
int APS5_VABI sceFontFtSupportTrueTypeGx();
int APS5_VABI sceFontFtSupportBdf();
int APS5_VABI sceFontFtSupportTrueType();
}

namespace {

constexpr const char* FormatModules[] = {"bdf", "cff", "pcf", "pfr", "t1cid", "truetype", "type1", "type42", "winfonts"};

void Require(bool value, const char* message) {
    if (value) return;
    std::fprintf(stderr, "guest font ft: %s\n", message);
    std::abort();
}

bool HasModule(const char* name) {
    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0) return false;
    const bool present = FT_Get_Module(library, name) != nullptr;
    FT_Done_FreeType(library);
    return present;
}

bool AllFormatModulesPresent() {
    for (const char* module : FormatModules) {
        if (!HasModule(module)) return false;
    }
    return true;
}

constexpr int Succeeded = 0;
constexpr int ModuleMissing = 1;
constexpr int Unexpected = 2;

int SupportOutcome(const char* name, int (*call)()) {
    try {
        return call() == 0 ? Succeeded : Unexpected;
    } catch (const std::runtime_error& error) {
        const std::string text = error.what();
        if (text.rfind(name, 0) == 0 && text.find("is not built in") != std::string::npos) return ModuleMissing;
        return Unexpected;
    }
}

}

int main() {
    Require(SupportOutcome("sceFontFtSupportFontFormats", sceFontFtSupportFontFormats) ==
                (AllFormatModulesPresent() ? Succeeded : ModuleMissing),
            "supportFontFormats did not follow the formats the build has");
    Require(SupportOutcome("sceFontFtSupportTrueTypeGx", sceFontFtSupportTrueTypeGx) ==
                (HasModule("gxvalid") ? Succeeded : ModuleMissing),
            "supportTrueTypeGx did not follow the gxvalid module the build has");
    Require(SupportOutcome("sceFontFtSupportBdf", sceFontFtSupportBdf) == (HasModule("bdf") ? Succeeded : ModuleMissing),
            "supportBdf did not follow the bdf module the build has");
    Require(SupportOutcome("sceFontFtSupportTrueType", sceFontFtSupportTrueType) ==
                (HasModule("truetype") ? Succeeded : ModuleMissing),
            "supportTrueType did not follow the truetype module the build has");
    Require(sceFontFtSupportSystemFonts() == 0, "supportSystemFonts failed");
}
