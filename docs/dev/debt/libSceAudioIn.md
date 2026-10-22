# libSceAudioIn technical debt

### Silent stubs

- [sceAudioInInit](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - returns 0 without library-wide setup, as `sceAudioOutInit` does; each `sceAudioInOpen` opens its own SDL capture device

### Unknown function info

- [sceAudioInIsPadEmbeddedMic](../../../core/libs/prx/libSceAudioIn/Export.cpp) - the PS5 module uses the low handle byte as a port index and returns whether its resolved device type equals 8, propagating negative device errors. Host capture devices have no corresponding controller-microphone classification; the protected throwing body remains unchanged.
- [sceAudioInDeviceIdOpen](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInDeviceIdAsyncOpen](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetUsbGain](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInIsSharedDevice](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInDeviceIdHqOpen](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInDeviceHqOpen](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetConnections](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetPortConnections](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetPortStatuses](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInGetHandleStatusInfo](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInOpenEx](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInHqOpenEx](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInDeviceOpenEx](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInChangeAppModuleState](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetSparkParam](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetSparkSideTone](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetCompressorPreGain](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetNoiseGateThreshold](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetMicLevel](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetDeviceMuteStatus](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetDevConnection](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInCountPorts](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetUserMute](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInVmicCreate](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInVmicDestroy](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInVmicWrite](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInVmicGetStatus](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInVmicSetMute](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetMode](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetMode2](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInExtCtrl](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInInputs](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature

- 6 additional pending imports in [InventoryExports.cpp](../../../core/libs/prx/libSceAudioIn/InventoryExports.cpp) are listed in the local PS5 3.20 export inventory `libSceAudioIn.c`. Their signatures and behavior are unverified; each uses the no-parameter unknown-signature convention and throws through `NotImplemented_nid_no_patch`, for import resolution only.

- `sceAudioInExtClose` — NID `kFKJ3MVcDuo` — [`sceAudioInPending_001_nid_no_patch`](../../../core/libs/prx/libSceAudioIn/InventoryExports.cpp)
- `sceAudioInExtInput` — NID `KpBKoHKVKEc` — [`sceAudioInPending_002_nid_no_patch`](../../../core/libs/prx/libSceAudioIn/InventoryExports.cpp)
- `sceAudioInExtOpen` — NID `YZ+3seW7CyY` — [`sceAudioInPending_003_nid_no_patch`](../../../core/libs/prx/libSceAudioIn/InventoryExports.cpp)
- `sceAudioInExtSetAecMode` — NID `FVGWf8JaHOE` — [`sceAudioInPending_004_nid_no_patch`](../../../core/libs/prx/libSceAudioIn/InventoryExports.cpp)
- `sceAudioInSetConnectionsForUser` — NID `thLNHvkWSeg` — [`sceAudioInPending_005_nid_no_patch`](../../../core/libs/prx/libSceAudioIn/InventoryExports.cpp)
- `sceAudioInSetFocusForUser` — NID `iN3KqF-8R-w` — [`sceAudioInPending_006_nid_no_patch`](../../../core/libs/prx/libSceAudioIn/InventoryExports.cpp)

### Functional

- [sceAudioInHqOpen, sceAudioInAsyncOpen](../../../core/libs/prx/libSceAudioIn/Export.cpp) - PS5 wrapper validation requires 48000 Hz followed by exactly 128 samples for both calls; HqOpen then requires format 2. The host uses the same SDL capture path for both, so native asynchronous scheduling and high-quality processing are not reproduced. Common port validation, device selection and host capture pacing remain incomplete.
- [sceAudioInGetGain, sceAudioInGetRerouteCount](../../../core/libs/prx/libSceAudioIn/Export.cpp) - GetGain writes 1.0f before handle validation and returns 0x80260105 for a null output, as confirmed by the PS5 module. Native handle bit checks and uninitialized/inactive port errors differ from the host port table. Native reroute counters follow device reassignment; the host opens each SDL device once and reports zero.
