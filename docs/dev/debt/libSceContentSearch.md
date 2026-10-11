# libSceContentSearch technical debt

### Unknown function info

- [sceContentSearchGetContentLastUpdateId, sceContentSearchOpenMetadataByContentId, sceContentSearchGetTotalContentSize, sceContentSearchGetNumOfContent](../../../core/libs/prx/libSceContentSearch/Export.cpp) (libSceContentSearch) - imported by PPSA06465, which fails to load without them; added without an implementation because their signatures are unknown
