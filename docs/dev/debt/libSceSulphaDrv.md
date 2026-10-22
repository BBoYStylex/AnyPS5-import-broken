# libSceSulphaDrv technical debt

### Unknown function info

- [libSceSulphaDrv pending exports](../../../core/libs/prx/libSceSulphaDrv/Export.cpp) (libSceSulphaDrv) - sceSulphaGetClientInfo, sceSulphaGetConnectedClientCount, sceSulphaGetDefaultConfig, sceSulphaGetNeededMemory, sceSulphaInit, sceSulphaMessage, sceSulphaSetBookmark and sceSulphaShutdown are the complete PS5 3.20 export table of this module per the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceSulphaDrv.c). Their signatures and behaviour are unverified; each declaration uses the no-parameter unknown-signature convention and throws through `NotImplemented_nid_no_patch`. The module implements none of them
