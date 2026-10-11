### What

Audit of [`docs/dev/TechnicalDebt.md`](../docs/dev/TechnicalDebt.md) for missing C++ PRX function declarations in libScePad / libSceRtc. There are none: every `scePad*` / `sceRtc*` name the debt file documents has an `APS5_VABI` definition in the file it links, so this pull request changes no library code.

Checked (current line numbers in the linked sources):

| TechnicalDebt entry | Function | Definition |
| --- | --- | --- |
| [`scePadOpenExt`](../docs/dev/TechnicalDebt.md#L65) | `scePadOpenExt` | [`libScePad/Export.cpp:143`](../core/libs/prx/libScePad/Export.cpp) |
| [`sceRtcParseDateTime`](../docs/dev/TechnicalDebt.md#L81) | `sceRtcParseDateTime` | [`libSceRtc/Export.cpp:494`](../core/libs/prx/libSceRtc/Export.cpp) |
| [`scePadIsRemoteController`](../docs/dev/TechnicalDebt.md#L180) | `scePadIsRemoteController` | [`libScePad/Export.cpp:263`](../core/libs/prx/libScePad/Export.cpp) |
| [`scePadSetAngularVelocityBiasCorrectionState`](../docs/dev/TechnicalDebt.md#L181) | `scePadSetAngularVelocityBiasCorrectionState` | [`libScePad/Export.cpp:271`](../core/libs/prx/libScePad/Export.cpp) |
| [`sceRtcFormatRFC2822`, `sceRtcFormatRFC2822LocalTime`, `sceRtcFormatRFC3339LocalTime`](../docs/dev/TechnicalDebt.md#L239) | all three | [`libSceRtc/Export.cpp:471`](../core/libs/prx/libSceRtc/Export.cpp), [`:483`](../core/libs/prx/libSceRtc/Export.cpp), [`:465`](../core/libs/prx/libSceRtc/Export.cpp) |
| [`sceRtcGetDosTime`, `sceRtcSetDosTime`](../docs/dev/TechnicalDebt.md#L254) | both | [`libSceRtc/Export.cpp:424`](../core/libs/prx/libSceRtc/Export.cpp), [`:437`](../core/libs/prx/libSceRtc/Export.cpp) |

The names each entry references in prose (`scePadGetControllerInformation`, `scePadSetAngularVelocityDeadbandState`, `scePadSetTiltCorrectionState`, `sceRtcFormatRFC3339`) are defined too. Every `scePad*` / `sceRtc*` symbol referenced under `core/`, including the `extern "C"` blocks of the guest tests, resolves to an `APS5_VABI` definition in one of the two libraries, and no `scePad*` / `sceRtc*` export is duplicated across libraries. The other `scePad*` / `sceRtc*` names in the aerolib NID list that the PRX does not export are absent on purpose: no title in the repository imports them and none is recorded in `TechnicalDebt.md`, so per `CONTRIBUTING.md` ("Missing imports fail at startup") they are not stubbed speculatively.

### Tested
<!-- OS, title or homebrew, what was checked. "Not tested" is a valid answer. -->
Linux (Ubuntu, GCC 13/14), `BUILD_TESTING=ON`:
- `ninja -C build`: default target (all libraries, tools and tests) builds clean, 0 errors
- `ctest --test-dir build -R 'guest_pad|guest_rtc'`: `guest_pad` and `guest_rtc` pass
- `ctest --test-dir build -R 'patched_libraries|nid'`: pass
- `python3 tools/check_conventions.py --base origin/main`: all conventions checks passed

No title run: no code changed.

### Checklist
<!-- Rules behind each item: https://github.com/boykopovar/AnyPS5/blob/main/CONTRIBUTING.md -->
- [x] Based on current `main`; no other open PR implements the same functions
- [x] One topic per PR; follow-ups go in a new PR
- [x] No comments in code except [technical debt](https://github.com/boykopovar/AnyPS5/blob/main/docs/dev/CONVENTIONS.md)
- [x] Unimplemented paths throw (`NotImplemented_nid_no_patch`); silent stubs are listed in [TechnicalDebt](https://github.com/boykopovar/AnyPS5/blob/main/docs/dev/TechnicalDebt.md#silent-stubs)
- [x] No notes, investigation `.md` files or images added to the repository (attach them to this PR)
- [x] New third-party code is a submodule built from source
- [x] Generic behaviour, not specific to one title
- [x] Depends on: none
- [x] AI-assisted: yes
