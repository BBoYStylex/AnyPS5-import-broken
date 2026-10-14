# libSceSystemGesture technical debt

### Silent stubs

- [System gesture recognizers](../../../core/libs/prx/libSceSystemGesture/Export.cpp) (libSceSystemGesture) - `sceSystemGestureGetPrimitiveTouchEvents`, `GetPrimitiveTouchEventsCount`, `GetTouchEvents`, `GetPrimitiveTouchEventByIndex` and the recognizer append, remove, reset and update calls report that no touch input exists, so a title never receives a gesture event. A recognizer keeps only the gesture type and rectangle that `sceSystemGestureCreateTouchRecognizer` and `UpdateTouchRecognizerRectangle` write into it. Handle and pointer arguments are still checked. Error codes follow the 0x80D1 values main already uses in this file; KytyPS5 uses 0x8089 (INVALID_ARGUMENT 0x80890001, INVALID_HANDLE 0x80890006), and which one the console returns is not verified

### Unknown function info

- [sceSystemGestureGetTouchRecognizerInformation](../../../core/libs/prx/libSceSystemGesture/Export.cpp) (libSceSystemGesture) reports the gesture type and rectangle kept in the recognizer and an `updated_time` of 0, since no input ever updates a recognizer; whether the console reports 0 there is unverified. A recognizer created with a null rectangle throws until `UpdateTouchRecognizerRectangle` gives it one, as the rectangle the console reports for it is unknown. A recognizer that `CreateTouchRecognizer` did not write throws `std::invalid_argument` there, in `GetTouchEventByEventID` and in `UpdateTouchRecognizerRectangle`
- [sceSystemGestureGetPrimitiveTouchEventByPrimitiveID and sceSystemGestureGetTouchEventByEventID](../../../core/libs/prx/libSceSystemGesture/Export.cpp) (libSceSystemGesture) throw after the handle and pointer checks: no event is ever reported, so every id is unknown, and the console's error for an unknown id is unverified
