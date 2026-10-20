# libSceFontFt technical debt

### Unknown function info

- [sceFontFtInitAliases, sceFontFtSetAliasFont, sceFontFtSetAliasPath and sceFontFtTermAliases](../../../core/libs/prx/libSceFontFt/Export.cpp) (libSceFontFt) - the names and NIDs are present in the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceFontFt.c), but no credible prototype was found for the alias table they manage. They are declared without parameters solely to resolve known imports; this is not an ABI claim and every call throws
- [sceFontSelectGlyphsFt](../../../core/libs/prx/libSceFontFt/Export.cpp) (libSceFontFt) - the name and NID are present in the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceFontFt.c), but the table it selects is not known, unlike the library and renderer tables that `sceFontSelectLibraryFt` and `sceFontSelectRendererFt` return. It is declared without parameters solely to resolve a known import and every call throws

### Functional

- [sceFontFtSupportFontFormats, sceFontFtSupportSystemFonts and sceFontFtSupportTrueTypeGx](../../../core/libs/prx/libSceFontFt/Export.cpp) (libSceFontFt) - the eleven format queries beside them report a format the build cannot read by throwing that the FreeType module is not built in, and these follow the same rule: the formats are FreeType's `bdf`, `cff`, `pcf`, `pfr`, `t1cid`, `truetype`, `type1`, `type42` and `winfonts` modules, and the GX TrueType format is FreeType's `gxvalid` module, which the bundled FreeType does not build in. The console's own answers are not measured, and a build that has a module reports the format as supported
- [sceFontFtSupportSystemFonts](../../../core/libs/prx/libSceFontFt/Export.cpp) (libSceFontFt) - the console's font library reads the system fonts, and the repository's font library does too, so the query reports that the format is supported
