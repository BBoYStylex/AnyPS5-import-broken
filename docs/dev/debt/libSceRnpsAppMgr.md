# libSceRnpsAppMgr technical debt

### Unknown function info

- [libSceRnpsAppMgr pending exports](../../../core/libs/prx/libSceRnpsAppMgr/Export.cpp) - 22 named callable exports have unverified signatures and behaviour; they use the no-parameter unknown-signature convention and throw through `NotImplemented_nid_no_patch`.
