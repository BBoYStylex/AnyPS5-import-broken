# libSceShareUtility technical debt

### Unknown function info

- [libSceShareUtility pending exports](../../../core/libs/prx/libSceShareUtility/Export.cpp) - 22 named callable exports have unverified signatures and behaviour; they use the no-parameter unknown-signature convention and throw through `NotImplemented_nid_no_patch`.
