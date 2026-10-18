# libSceLoginService technical debt

### Silent stubs

- [sceLoginServiceInitialize, sceLoginServiceTerminate, sceLoginServiceRequestDevices](../../../core/libs/prx/libSceLoginService/Export.cpp) (libSceLoginService) - return success and keep no login-service state: AnyPS5 has no PSN login service. EA SPORTS FC 27 (PPSA34015) loads the module with `sceSysmoduleLoadModule(0xe3)` and calls `sceLoginServiceInitialize` with no arguments; a missing `libSceLoginService.prx` stops Windows start-up with `Failed to load module` / `GetLastError: 126`

### Unknown function info

