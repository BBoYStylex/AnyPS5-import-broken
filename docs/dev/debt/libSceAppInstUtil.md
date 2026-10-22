# libSceAppInstUtil technical debt

### Unknown function info

- [sceAppInstUtilAppConvertAppDiscToDownload, sceAppInstUtilAppGetCustomThemeInfo, sceAppInstUtilAppGetInstallAddcontFromDiscStatus, sceAppInstUtilAppInstallCloudGame, sceAppInstUtilAppInstallMediaPlayer](../../../core/libs/prx/libSceAppInstUtil/Export.cpp) - names and NIDs match the [PS5 3.20 inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceAppInstUtil.c); signatures and behavior are unverified. No-parameter declarations resolve imports and throw through `NotImplemented_nid_no_patch` when called.
- [sceAppInstUtilAppInstallRequestAddcontFromDisc, sceAppInstUtilAppSetSavedataUploadStatus](../../../core/libs/prx/libSceAppInstUtil/Export.cpp) - names and NIDs match the [PS5 3.20 inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceAppInstUtil.c); signatures and behavior are unverified. No-parameter declarations resolve imports and throw through `NotImplemented_nid_no_patch` when called.

- [sceAppInstUtilAppInstallPkg](../../../core/libs/prx/libSceAppInstUtil/Export.cpp) - the package path is followed by an opaque 64-bit value sent to the install service; its meaning and pointer-versus-integer type remain unresolved.
- [sceAppInstUtilGetInstallProgressInfo, sceAppInstUtilGetInstallProgress and sceAppInstUtilGetPrimaryAppSlot](../../../core/libs/prx/libSceAppInstUtil/Export.cpp) - take a title-id string and an output pointer. The IPC replies contain 48, 4 and 4 bytes respectively; callers and reply producers are needed to establish the field types.
- [sceAppInstUtilAppPrepareOverwritePkg, sceAppInstUtilAppRecoverApp and sceAppInstUtilAppUnInstallTheme](../../../core/libs/prx/libSceAppInstUtil/Export.cpp) - absent from the supplied system module and from the supplied decompiled callers; the old export inventory establishes names but not prototypes.
