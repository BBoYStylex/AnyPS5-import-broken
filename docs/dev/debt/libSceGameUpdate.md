# libSceGameUpdate technical debt

### Unknown function info

- [sceGameUpdateGetAddcontLatestVersion](../../../core/libs/prx/libSceGameUpdate/Export.cpp) (libSceGameUpdate) - a null `entitlement_label` returns `SCE_GAME_UPDATE_ERROR_INVALID_ARGUMENT` (0x80412803) per prosper; KytyPS5 accepts it. Bytes after `found` are zeroed as in KytyPS5; prosper leaves them untouched