# libSceNpFriendListDialog technical debt

### Silent stubs

- [libSceNpFriendListDialog](../../../core/libs/prx/libSceNpFriendListDialog/Export.cpp) (libSceNpFriendListDialog) - the exports other than `sceNpFriendListDialogOpen` and `sceNpFriendListDialogGetResult` show nothing and finish as cancelled by the user, as there is no PSN sign-in; no title is known to import the library. The signatures are the PS4 ones of the [Orbital type database](https://github.com/AlexAltea/orbital/blob/b321fcdbd700cc02da1cb1ad25412781d5ec73c6/tools/ida/db_types.json); the PS5 ABI, the result layout and the check order are unverified. The calls run [OfflineDialog.hpp](../../../core/libs/prx/libSceCommonDialog/OfflineDialog.hpp): `Open` finishes at once, `Close` finishes also before `Open`, `GetResult` writes only the 32-bit result word (1) and the 0x80B8xxxx errors follow libSceVrSetupDialog. Closes when: a title that imports it runs, or the module's checks are read or measured on a PS5

### Unknown function info

- [sceNpFriendListDialogGetResult and sceNpFriendListDialogOpen](../../../core/libs/prx/libSceNpFriendListDialog/Export.cpp) (libSceNpFriendListDialog) - throw: the names and NIDs are in the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceNpFriendListDialog.c), but the [Orbital type database](https://github.com/AlexAltea/orbital/blob/b321fcdbd700cc02da1cb1ad25412781d5ec73c6/tools/ida/db_types.json) lists only the `A` forms, so the parameters are unknown. They are declared without parameters solely to resolve imports
