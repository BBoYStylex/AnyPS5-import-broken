# libSceVdecCore technical debt

### Unknown function info

- [libSceVdecCore pending exports](../../../core/libs/prx/libSceVdecCore/Export.cpp) - 20 named callable exports have unverified signatures and behaviour; they use the no-parameter unknown-signature convention and throw through `NotImplemented_nid_no_patch`.
