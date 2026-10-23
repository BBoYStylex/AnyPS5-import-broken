# libSceRegMgr technical debt

### Functional

- [Registry store defaults](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - entries are backed by an in-memory store without filesystem persistence. Callable prototypes for `sceRegMgrGetInt`, `sceRegMgrGetStr`, and `sceRegMgrGetBin`, as well as key constants (version `17825792`, PS4 version `16842752`, update mode `33619968`, language `33685504`, nickname `33882112`, dimmer `33947648`, auto power-down `42075904`, init user `117637120`) and types, derive from [regmgr.h](https://github.com/ps5-payload-dev/regdump/blob/883524cc48c8497267ba7fc94ff0ac58880c4227/regmgr.h#L3713-L3729). The eight initial values (version `0x03200000`, PS4 version `0x09000000`, update mode `0`, language `1` [English US], nickname `"PS5"`, dimmer `15`, power-down `60`, init user `1`) are host emulation fallback assumptions, not measured console defaults.
- [Unverified registry APIs](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - export names and NIDs for setters, reset, and initial-value getters are present in the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/cf6eede3f6588d779664e01874638f09484324b7/libSceRegMgr.c) and [NID database](https://github.com/zecoxao/sce_symbols/blob/9620e03b2964dbf0cfc1cb143ab24a65125091de/aerolib.csv), which establish symbol names only. Callable signatures and runtime behavior for `sceRegMgrGet*InitVal`, `sceRegMgrResetVal`, and `sceRegMgrNonSys*` (including user parameter semantics) remain unverified.
- [Region queries and utility helpers](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - `sceRegMgrSrvGetRegion` returning `1` and `sceRegMgrSrvGetRegionStr` returning `"US"` are host emulation fallback assumptions, not measured console defaults. Two-letter region conversion mappings in `sceRegMgrSrvCnvRegionInt` and `sceRegMgrSrvCnvRegionStr` (JP 0, US 1, EU 2, KR 3, AS 4, RU 5, CN 6; unknown -1) are implementation assumptions; no pinned console source verifies their conversion tables or error behavior. Status and error checks (`sceRegMgrIsInitOK`, `sceRegMgrGetInitError`, `sceRegMgrCheckError`, `sceRegMgrCheckIntVal`, `sceRegMgrNonSysCheckError`) and utility memory helpers (`sceRegMgrStrncpy`, `sceRegMgrMemset`) model unverified behavior. Unimplemented control, driver, backup, and event notification routines continue throwing.

### Unknown function info

- [sceRegMgrIsChange](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrEvtGetCnt](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrEvtGetRegId](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrSetInitLevel](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrSrvGetMachineType](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrSrvGetQAFforReg](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrSrvGetRealMachineType](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrBackupNeedMem](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrBackupPullData](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrBackupPushData](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrBackupPullDataGkp](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrBackupPushDataGkp](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrCntlStart](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrCntlDeleteReg](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrCntlGetFilesCount](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrCntlGetFileName](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrCntSetInitReinst](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrRecoverRegNvs](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrBackupNeedMemForPS4](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrBackupPullDataForPS4](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrBackupPushDataForPS4](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrEvtGetCntForPS4](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrEvtGetRegIdForPS4](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrToolGetUpdateCnt](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrToolGetEntryCnt](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrToolDataCheckGet](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrDrvGetUpdateCnt](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrDrvGetEntCnt](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrDrvDataOpen](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrDrvDataCheckGet](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrDrvDataClose](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrToolGetInfo](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrLogPull](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrCntlStopHdd](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
- [sceRegMgrPrintInfo](../../../core/libs/prx/libSceRegMgr/Export.cpp) (libSceRegMgr) - unknown signature
