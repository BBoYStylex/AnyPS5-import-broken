# libSceDiscId technical debt

### Silent stubs

- [libSceDiscId](../../../core/libs/prx/libSceDiscId/Export.cpp) has no optical disc drive backend: `sceDiscIdGet` and `sceDiscIdGet2` validate null pointers and flags, returning `SCE_DISC_ID_ERROR_NO_DISC` (0x80D50002).

### Unknown function info

- [sceDiscIdGet](../../../core/libs/prx/libSceDiscId/Export.cpp) takes one output pointer; `sceDiscIdGet2` takes an output pointer and a 32-bit argument that must be zero. Both pass the pointer to a device ioctl without accessing its contents. Output structure sizes and fields require the device-side implementation or a caller and remain unresolved.
