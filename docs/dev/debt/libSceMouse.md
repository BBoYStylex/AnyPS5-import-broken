# libSceMouse technical debt

### Silent stubs

- [sceMouseResetAbsolutePosition](../../../core/libs/prx/libSceMouse/Export.cpp) (libSceMouse) - returns success without moving anything: the library keeps relative motion deltas only and has no absolute cursor position to reset (`sceMouseRead` reports per-event relative motion)
