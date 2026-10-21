# libSceSysCore technical debt

### Unknown function info

- [sceApplicationGetAppInfoByAppId](../../../core/libs/prx/libSceSysCore/Export.cpp) (libSceSysCore) - the name and NID are present in the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceSysCore.c), but no credible prototype was found. The declaration has no parameters solely to resolve a known import and makes no ABI claim

- [libSceSysCore pending exports](../../../core/libs/prx/libSceSysCore/Export.cpp) - 70 named callable exports are listed in the [PS5 3.20 export table](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceSysCore.c) and were cross-checked against the same module in PS5 firmware 13.60. Their signatures and behaviour are unverified; the declarations use the no-parameter unknown-signature convention and throw through `NotImplemented_nid_no_patch`.
