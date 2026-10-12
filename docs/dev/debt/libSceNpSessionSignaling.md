# libSceNpSessionSignaling technical debt

### Unknown function info

- [sceNpSessionSignalingGetConnectionFromPeerAddress](../../../core/libs/prx/libSceNpSessionSignaling/Export.cpp) (libSceNpSessionSignaling) - imported by PPSA01289 and added without an implementation; unknown signature
- [sceNpSessionSignalingGetMemoryInfo](../../../core/libs/prx/libSceNpSessionSignaling/Export.cpp) (libSceNpSessionSignaling) - unknown signature
- [sceNpSessionSignalingGetConnectionStatistics](../../../core/libs/prx/libSceNpSessionSignaling/Export.cpp) (libSceNpSessionSignaling) - unknown signature
- [sceNpSessionSignalingGetConnectionStatus](../../../core/libs/prx/libSceNpSessionSignaling/Export.cpp) (libSceNpSessionSignaling) - `(context id, connection id, int32 status*, peer address*, uint16 peer port*)` is assumed from PS4's `sceNpSignalingGetConnectionStatus`; ASTRO BOT (PPSA21564) imports it. No session ever activates offline, so every connection id answers `CONN_NOT_FOUND` (0x8055330C, from [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/64d198190fa05d37cbb1fa29c50f967c376fce55/src/core/libraries/np/np_error.h)) without writing the outputs, and a null status answers `INVALID_ARGUMENT`. The context id is not checked, as contexts are not tracked. The console's answer for an unknown context or connection is unverified
