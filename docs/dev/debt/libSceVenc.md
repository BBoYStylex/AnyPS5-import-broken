# libSceVenc technical debt

### Unknown function info

- [libSceVenc pending exports](../../../core/libs/prx/libSceVenc/Export.cpp) - 14 named callable exports have unverified signatures and behaviour; they use the no-parameter unknown-signature convention and throw through `NotImplemented_nid_no_patch`.
