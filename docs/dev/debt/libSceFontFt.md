# libSceFontFt technical debt

### Silent stubs

- [sceFontFtSupportBdf, sceFontFtSupportCid, sceFontFtSupportFontFormats, sceFontFtSupportOpenType, sceFontFtSupportOpenTypeOtf, sceFontFtSupportOpenTypeTtf, sceFontFtSupportPcf, sceFontFtSupportPfr, sceFontFtSupportSystemFonts, sceFontFtSupportTrueType, sceFontFtSupportTrueTypeGx, sceFontFtSupportType1, sceFontFtSupportType42, sceFontFtSupportWinFonts](../../../core/libs/prx/libSceFontFt/Export.cpp) (libSceFontFt) return 0 and do not load extra FreeType modules. The calls are parameterless, as in [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/main/src/core/libraries/font/fontft.cpp). TrueType and OpenType already work through the existing driver; [LibrarySupport](../../../core/libs/prx/libSceFontFt/src/Driver.cpp) ignores the format flags. Other format flags are accepted and unused.
