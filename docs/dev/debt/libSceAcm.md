# libSceAcm technical debt

### Silent stubs

- [libSceAcm](../../../core/libs/prx/libSceAcm/Export.cpp) batches execute as silent no-ops, so `sceAcm_ConvReverb_SharedInput`, `sceAcm_FFT` and `sceAcm_Panner` return success without encoding audio commands.
- [sceAcmBatchJobPriority](../../../core/libs/prx/libSceAcm/Export.cpp) returns 0; PPSA02530 imports it with an unknown signature and stops when it throws. No batch-job priority on the host. Covered by `guest_acm`
