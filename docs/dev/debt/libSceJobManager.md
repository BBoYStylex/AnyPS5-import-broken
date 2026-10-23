# libSceJobManager technical debt

### Functional

- [JobManager compute sequences](../../../core/libs/prx/libSceJobManager/Export.cpp) submit a separate label write and interrupt for every dispatch instead of batching and rewinding commands encoded ahead. Commands and labels live on the host heap; labels are written as 32 bits and have no GPU import, so the driver drains recorded work before each write. Required command storage uses fixed 104/148/32-byte estimates instead of measuring the current builders. A compute dispatch on a CPU sequence and a failing `sceAgcDriverAddEqEvent` throw.
- [JobManager scheduling state](../../../core/libs/prx/libSceJobManager/Export.cpp) lives on the host heap instead of the caller's block. Blocks smaller than 0x2d8 bytes throw. Worker spin and persistent-thread settings do not affect scheduling. Factory priorities above one and use after sequence destruction throw. Shutdown throws while a compute sequence has work in flight or a caller is blocked in a sequence wait. Worker creation and attribute failures throw where the original does not check.
- The [manager](../../../core/libs/prx/libSceJobManager/Export.cpp) omits the `sceRazorCpuJobManager*` profiler calls. Sequence and factory vtable order has been checked against original modules, but no title that calls it has been run and PS5 timing equivalence is unmeasured.
