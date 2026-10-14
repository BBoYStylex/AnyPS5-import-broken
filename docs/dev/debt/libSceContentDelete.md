# libSceContentDelete technical debt

### Unknown function info

- [sceContentDeleteById and sceContentDeleteByPath](../../../core/libs/prx/libSceContentDelete/Export.cpp) (libSceContentDelete) - the 64-bit content id and the UTF-8 path follow [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/5dfdb4b0b166e83dc9ae275001afb695f12c7174/src/SharpProspero/Interop/Content/ContentDelete.cs). PPSA06465 imports `sceContentDeleteById` and fails to load without it. The host never exports or captures content, so no id or path names any, and both throw once the library is checked to be initialized and the path is non-null; the console's error for missing content, and its answer before `sceContentDeleteInitialize`, are unverified
