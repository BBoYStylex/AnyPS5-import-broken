# libSceSocialScreenDialog technical debt

### Silent stubs

- [libSceSocialScreenDialog](../../../core/libs/prx/libSceSocialScreenDialog/Export.cpp) (libSceSocialScreenDialog) - the seven exports show nothing and finish as cancelled by the user, as no second screen is supported; no title is known to import the library. The signatures are the PS4 ones of the [Orbital type database](https://github.com/AlexAltea/orbital/blob/b321fcdbd700cc02da1cb1ad25412781d5ec73c6/tools/ida/db_types.json); the PS5 ABI, the result layout and the check order are unverified. The calls run [OfflineDialog.hpp](../../../core/libs/prx/libSceCommonDialog/OfflineDialog.hpp): `Open` finishes at once, `Close` finishes also before `Open`, `GetResult` writes only the 32-bit result word (1) and the 0x80B8xxxx errors follow libSceVrSetupDialog. Closes when: a title that imports it runs, or the module's checks are read or measured on a PS5
