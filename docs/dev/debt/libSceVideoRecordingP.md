# libSceVideoRecordingP technical debt

### Functional

- [sceVideoRecordingQueryMemSize, sceVideoRecordingOpen, sceVideoRecordingStart, sceVideoRecordingStop, sceVideoRecordingClose, sceVideoRecordingGetStatus](../../../core/libs/prx/libSceVideoRecordingP/Export.cpp) (libSceVideoRecordingP) run an open/start/stop/close state machine and never capture frames or write the path: `QueryMemSize` returns a fixed 1 MiB heap requirement, `Open` accepts a heap at least that large, and `GetStatus` reports closed (0), open (1) or recording (2). Console status codes, the `param` layout, whether `Close`'s discard flag deletes a file, and the real heap size are unknown. PPSA34015 imports them

### Unknown function info

- [videoRecordingUnknown_iQS6DUtLybE](../../../core/libs/prx/libSceVideoRecordingP/Export.cpp) (libSceVideoRecordingP) - NID-only export with unknown signature; throws
