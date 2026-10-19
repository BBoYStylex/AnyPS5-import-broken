# libSceConvertKeycode technical debt

### Unknown function info

- [sceConvertKeycodeGetCharacter](../../../core/libs/prx/libSceConvertKeycode/Export.cpp) (libSceConvertKeycode) - the signature `(uint16_t keycode, uint32_t state, uint32_t keyboardType, char16_t* character)` is inferred from PPSA12361, which passes a 16-bit keycode, 0, the type from `sceConvertKeycodeGetImeKeyboardType` and a pointer it reads one UTF-16 unit from when the result is 0. PRAGMATA (PPSA02530, #5726) and Onimusha: Way of the Sword (PPSA27836) pass `0x4000000` as a constant: Onimusha calls it for every keycode from 0 to 0x8F to build a table of key names, upper-cases `a` to `z` itself and skips keycodes that fail or return 0, so that bit is ignored and the character is returned without modifier modification. Supported modifier bits are Shift (`0x01`, `0x02`, `0x20`), CapsLock (`0x0200`, `0x10000`), AltGr (`0x40`), and NumLock (`0x0100`, `0x20000`). Any other state bit throws. Supported keyboard types are 0 (US), 1 (Japanese JIS), 2 (UK), 3 (German), 4 (French AZERTY), 5 (Spanish), and 6 (Italian); higher types return `0x80BC0011`.
- [sceConvertKeycodeGetVirtualKeycode](../../../core/libs/prx/libSceConvertKeycode/Export.cpp) (libSceConvertKeycode) - converts USB HID keycodes to Windows/PlayStation virtual keycodes taking keyboard layout into account (types 0..6). Keycodes with no corresponding virtual keycode write 0; higher keyboard types return `0x80BC0011`.
- [sceConvertKeycodeGetCharacterFromKeyboardData](../../../core/libs/prx/libSceConvertKeycode/Export.cpp) (libSceConvertKeycode) - signature unknown; calls `NotImplemented_nid_no_patch`.

### Functional

- [sceConvertKeycodeGetCharacter](../../../core/libs/prx/libSceConvertKeycode/Export.cpp) (libSceConvertKeycode) takes the keycode as a USB HID keyboard usage and returns characters for US, Japanese JIS, UK, German, French AZERTY, Spanish, and Italian layouts with Shift, CapsLock, and AltGr. A keycode without a character returns 0 with character 0; whether the console returns an error instead is not measured.
