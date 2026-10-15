# libSceVrTracker2 technical debt

### Silent stubs

- [libSceVrTracker2](../../../core/libs/prx/libSceVrTracker2/Export.cpp) has no PS VR2 tracker runtime: every export throws, except `sceVrTracker2Finalize`
- [sceVrTracker2Finalize](../../../core/libs/prx/libSceVrTracker2/Export.cpp) returns 0; PPSA29866 imports it with an unknown signature and stops when it throws (#3646). No tracker state to tear down on the host
