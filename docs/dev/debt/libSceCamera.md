# libSceCamera technical debt

### Unknown function info

- [libSceCamera pending exports](../../../core/libs/prx/libSceCamera/Export.cpp) (libSceCamera) - 117 named callable exports are present in the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceCamera.c), which lists them among the module's exports, and the [NID name database](https://github.com/zecoxao/sce_symbols/blob/2883963a0a514ba6e77407a08ef4730a65e88254/aerolib.csv) gives each of them the same name, which is the evidence for the name. The public PS5 payload SDK has no stub table for this module, so the kind of each symbol is unestablished: the 3.20 inventory marks every entry a function, but that is a property of the inventory and not of the module. Their signatures and behaviour are unverified; each declaration uses the no-parameter unknown-signature convention and throws through `NotImplemented_nid_no_patch`. They add no behaviour to the library
