# libSceCommonDialog technical debt

### Silent stubs

- [libSceCommonDialog](../../../core/libs/prx/libSceCommonDialog/Export.cpp)
- [sceCommonDialogTerminate](../../../core/libs/prx/libSceCommonDialog/Export.cpp) - sceCommonDialogTerminate (NID `KQNflBU4pN8`) is not documented in pinned aerolib.csv or registered in shadPS4, and no known title imports it directly. The return code `0x80B80001` (`COMMON_DIALOG_ERROR_NOT_SYSTEM_INITIALIZED`) follows shadPS4's `NOT_SYSTEM_INITIALIZED` naming to distinguish from `0x80B80003` (`NOT_INITIALIZED`); both the export and the error code remain unverified.
