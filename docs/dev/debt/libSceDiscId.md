# libSceDiscId technical debt

### Silent stubs

- [libSceDiscId](../../../core/libs/prx/libSceDiscId/Export.cpp) has no optical disc drive backend: `sceDiscIdGet` and `sceDiscIdGet2` report `SCE_DISC_ID_ERROR_NO_DISC` (0x80D50002) for every pointer and flag value, without reading or writing the output. Neither SDK nor console evidence establishes that these exports reject a null output or an unsupported flag value, or which error would take precedence, so no argument is treated as invalid. The no-disc code itself is assumed.

### Unknown function info

- [sceDiscIdGet](../../../core/libs/prx/libSceDiscId/Export.cpp) takes one output pointer; `sceDiscIdGet2` takes an output pointer and a 32-bit argument whose accepted values remain unresolved (it may be a bitfield). Both pass the pointer to a device ioctl without accessing its contents. Output structure sizes and fields require the device-side implementation or a caller and remain unresolved.
