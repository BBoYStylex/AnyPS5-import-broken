# libScePlayerInvitationDialog technical debt

### Functional

- [libScePlayerInvitationDialog](../../../core/libs/prx/libScePlayerInvitationDialog/libScePlayerInvitationDialog.cpp) simulates dialog completion without displaying UI or sending invitations, so `scePlayerInvitationDialogGetResult` reports `USER_CANCELED`; its parameter ABI remains unverified. The 40-byte result layout (error code, result, 32 reserved bytes) is taken from [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/main/src/SharpProspero/Interop/Dialog/PlayerInvitationDialog.cs) and the error codes from the other common dialogs, not confirmed on a title.

### Unknown function info

- [scePlayerInvitationDialogParamInit, scePlayerInvitationDialogParamInitialize](../../../core/libs/prx/libScePlayerInvitationDialog/libScePlayerInvitationDialog.cpp) (libScePlayerInvitationDialog) - their names and NIDs are in the [verified NID name database](https://github.com/zecoxao/sce_symbols/blob/2883963a0a514ba6e77407a08ef4730a65e88254/aerolib.csv), which the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libScePlayerInvitationDialog.c) does not list, so a title importing one fails to load. No PS5 source declares their prototype, and no source marks whether they are functions or objects, so they are declared without parameters to resolve the import; this is not an ABI claim
