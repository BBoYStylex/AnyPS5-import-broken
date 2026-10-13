# libSceVoiceChat technical debt

### Unknown function info

- [sceVoiceChatRequestCreateVoiceChatGroup, sceVoiceChatRequestJoinVoiceChatGroup, sceVoiceChatRequestLeaveVoiceChatGroup, sceVoiceChatRequestDeleteVoiceChatGroup](../../../core/libs/prx/libSceVoiceChat/Export.cpp) (libSceVoiceChat) - assumed to take a request id and two pointers like the other `sceVoiceChatRequest*` calls; offline, every argument is ignored and `SIGNED_OUT` is returned, as for those
- Declared without parameters, signatures unknown, no title known to import them yet: `sceVoiceChatRegisterMicEventHandler2`, `sceVoiceChatUnregisterMicEventHandler2` in [libSceVoiceChat](../../../core/libs/prx/libSceVoiceChat/Export.cpp) (libSceVoiceChat). Names from the zecoxao/sce_symbols NID list, each verified against its NID
