# libSceAvSetting technical debt

### Unknown function info

- [libSceAvSetting pending exports](../../../core/libs/prx/libSceAvSetting/Export.cpp) - 97 named exports are listed in the public [PS5 3.20 export list](https://github.com/DNNDHH/PS5-3.20_Libs/tree/b2a77c09cd71c01f34c790cfbe98c751a13262ad) and the local SPRX export inventory. Their signatures and behaviour are unverified; declarations use the no-parameter unknown-signature convention and throw through `NotImplemented_nid_no_patch`.
