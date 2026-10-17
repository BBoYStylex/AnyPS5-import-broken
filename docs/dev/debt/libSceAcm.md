# libSceAcm technical debt

### Silent stubs

- [libSceAcm](../../../core/libs/prx/libSceAcm/Export.cpp) batches execute as silent no-ops, so `sceAcm_ConvReverb_SharedInput`, `sceAcm_FFT`, `sceAcm_Panner` and `sceAcmBatchJobPriority` return success without encoding audio commands or scheduling batch jobs.

### Unknown function info

- PPSA02530 imports declared without parameters, signatures unknown: [sceAcmBatchJobPriority](../../../core/libs/prx/libSceAcm/Export.cpp) (libSceAcm) and [scePadVrControllerGetTriggerEffectStates](../../../core/libs/prx/libScePad/Export.cpp) (libScePad).

