# libSceVideoRecordingP technical debt

### Unknown function info

- [sceVideoRecordingOpen, Start, Stop, Close, GetStatus and QueryMemSize](../../../core/libs/prx/libSceVideoRecordingP/Export.cpp) (libSceVideoRecordingP) - the signatures, the parameter structure (its size, the ring buffer length in seconds and the container format), the `NONE` status 0 and the 4 KiB maximum heap follow [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/5dfdb4b0b166e83dc9ae275001afb695f12c7174/src/SharpProspero/Interop/Video/VideoRecording.cs). The host has no gameplay video recorder: `sceVideoRecordingOpen` checks its path and parameter and throws, so the recorder is never open, `GetStatus` reports `NONE` and `Start`, `Stop` and `Close` throw, as the console's error for a recorder that is not open is unverified. `QueryMemSize` returns the 4 KiB maximum for any non-null parameter rather than the console's size for those settings
- [NID `iQS6DUtLybE`](../../../core/libs/prx/libSceVideoRecordingP/Export.cpp) (libSceVideoRecordingP, exported as `videoRecordingUnknown_iQS6DUtLybE`) - name and signature unknown, so it throws
