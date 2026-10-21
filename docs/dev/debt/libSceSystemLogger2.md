# libSceSystemLogger2 technical debt

### Unknown function info

- [libSceSystemLogger2 pending exports](../../../core/libs/prx/libSceSystemLogger2/Export.cpp) - 1 named callable exports have unverified signatures and behaviour; they use the no-parameter unknown-signature convention and throw through `NotImplemented_nid_no_patch`.
