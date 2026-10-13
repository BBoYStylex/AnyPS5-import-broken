#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdlib>

extern "C" {
int APS5_VABI sceFontFtSupportBdf();
int APS5_VABI sceFontFtSupportCid();
int APS5_VABI sceFontFtSupportFontFormats();
int APS5_VABI sceFontFtSupportOpenType();
int APS5_VABI sceFontFtSupportOpenTypeOtf();
int APS5_VABI sceFontFtSupportOpenTypeTtf();
int APS5_VABI sceFontFtSupportPcf();
int APS5_VABI sceFontFtSupportPfr();
int APS5_VABI sceFontFtSupportSystemFonts();
int APS5_VABI sceFontFtSupportTrueType();
int APS5_VABI sceFontFtSupportTrueTypeGx();
int APS5_VABI sceFontFtSupportType1();
int APS5_VABI sceFontFtSupportType42();
int APS5_VABI sceFontFtSupportWinFonts();
}

static void Require(bool value) {
    if (!value) std::abort();
}

int main() {
    Require(sceFontFtSupportBdf() == 0);
    Require(sceFontFtSupportCid() == 0);
    Require(sceFontFtSupportFontFormats() == 0);
    Require(sceFontFtSupportOpenType() == 0);
    Require(sceFontFtSupportOpenTypeOtf() == 0);
    Require(sceFontFtSupportOpenTypeTtf() == 0);
    Require(sceFontFtSupportPcf() == 0);
    Require(sceFontFtSupportPfr() == 0);
    Require(sceFontFtSupportSystemFonts() == 0);
    Require(sceFontFtSupportTrueType() == 0);
    Require(sceFontFtSupportTrueTypeGx() == 0);
    Require(sceFontFtSupportType1() == 0);
    Require(sceFontFtSupportType42() == 0);
    Require(sceFontFtSupportWinFonts() == 0);
    return 0;
}
