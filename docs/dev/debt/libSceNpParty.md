# libSceNpParty technical debt

### Silent stubs

- [sceNpPartyGetState](../../../core/libs/prx/libSceNpParty/Export.cpp) (libSceNpParty) reports that the user is in no party (`2`), as no PSN party service runs

### Unknown function info

- [sceNpPartyGetState](../../../core/libs/prx/libSceNpParty/Export.cpp) (libSceNpParty) - the state value `2` (not in party) follows [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/f68923fe807f657348e6719ce658bcbd0a2b1c72/src/core/libraries/np/np_party.cpp)'s `OrbisNpPartyState` (PS4), and the `uint16_t*` signature [Orbital](https://github.com/AlexAltea/orbital/blob/master/tools/ida/db_types.json); neither is measured on a PS5. A null output throws: the console's error for it is not known. The call does not check `sceNpPartyInitialize`, which is not exported, as shadPS4 does not. Closes when: measured on a PS5
