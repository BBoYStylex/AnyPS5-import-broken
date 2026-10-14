# libSceNpTrophy2 technical debt

### Silent stubs

- [libSceNpTrophy2](../../../core/libs/prx/libSceNpTrophy2/src/GameInfo.cpp): `sceNpTrophy2RegisterUnlockCallback` accepts a callback that is never called, as no trophy is ever unlocked, `sceNpTrophy2UnregisterUnlockCallback` only returns success, and `sceNpTrophy2ShowTrophyList` returns success without showing a list (there is no system trophy UI; seen on PPSA15552).

### Unknown function info

- [sceNpTrophy2GetRewardIcon](../../../core/libs/prx/libSceNpTrophy2/src/Context.cpp) (libSceNpTrophy2) - the context, handle, trophy id, buffer and size pointer follow [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/5dfdb4b0b166e83dc9ae275001afb695f12c7174/src/SharpProspero/Interop/Np/NpTrophy2.cs). No trophy reported by `sceNpTrophy2GetTrophyInfo` has a reward, so, as `sceNpTrophy2GetTrophyIcon` does, it writes a size of 0 when the size pointer is non-null and throws instead of returning `ICON_FILE_NOT_FOUND`, which the console may answer for a trophy without a reward
