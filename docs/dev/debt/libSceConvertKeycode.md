# libSceConvertKeycode technical debt

### Unknown function info

- [sceConvertKeycodeGetCharacter](../../../core/libs/prx/libSceConvertKeycode/Export.cpp) (libSceConvertKeycode) - keyboard layout types in 1..37 not yet implemented throw `std::runtime_error`; types above 37 return `0x80BC0011`. Unrecognized state bits outside modifier, LED and ignored flags throw `std::runtime_error`. Bit `0x4000000` is passed as a constant by PPSA02530 (#5726) and PPSA27836 and is ignored. Keyboard layout types (0..37) match SharpProspero and shadPS4 `OrbisImeKeyboardType`. State bit packing follows `(modifiers & 0xFF) << 8 | (leds & 7) << 16` (Shift `0x0200`/`0x2000`, AltGr `0x4000`, NumLock `0x10000`, CapsLock `0x20000`, plus raw modifier bytes). Supported layouts: US (0, 4, 11), German (2, 3), UK (5), Spanish (6, 7), French (9, 10, 12), Italian (13), and Japanese (22, 23).
- [sceConvertKeycodeGetVirtualKeycode](../../../core/libs/prx/libSceConvertKeycode/Export.cpp) (libSceConvertKeycode) - converts USB HID keycodes to Windows/PlayStation virtual keycodes taking keyboard layout into account; unmapped keycodes return 0; keyboard types above 37 return `0x80BC0011`. Layout definitions and virtual keycodes follow fpPS4 (`src/ps4_libsceconvertkeycode.pas`) and SharpProspero.
- [sceConvertKeycodeGetCharacterFromKeyboardData](../../../core/libs/prx/libSceConvertKeycode/Export.cpp) (libSceConvertKeycode) - signature unknown; calls `NotImplemented_nid_no_patch`.

### Functional

- [sceConvertKeycodeGetCharacter](../../../core/libs/prx/libSceConvertKeycode/Export.cpp) (libSceConvertKeycode) takes a USB HID keyboard usage and returns characters for US, UK, German, French, Spanish, Italian, and Japanese layouts with Shift, CapsLock, and AltGr. Unprintable control keycodes (Enter, Backspace, Tab) and unassigned keycodes write 0; whether the console returns an error code instead is not measured.
