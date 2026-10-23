# libSceConvertKeycode technical debt

### Functional

- [sceConvertKeycodeGetVirtualKeycode](../../../core/libs/prx/libSceConvertKeycode/Export.cpp) and `sceConvertKeycodeGetCharacter`: layouts 31–39 throw until the guest platform and compiled SDK version are exposed. `GetVirtualKeycode` also throws for version-dependent Y/Z mappings in layouts 2, 3 and 12. Mapping tables and modifier behavior are recovered; no SDK version is guessed.
- [sceConvertKeycodeGetImeKeyboardType](../../../core/libs/prx/libSceConvertKeycode/Export.cpp): the initial user's IME layout is configured directly with `ANYPS5_KEYBOARD_LAYOUT`, defaulting to English (US), layout 4. Layout values follow [SharpProspero's on-device converter](https://github.com/SvenGDK/SharpProspero/blob/5dfdb4b0b166e83dc9ae275001afb695f12c7174/src/SharpProspero/Input/KeycodeConverter.cs). Other nonnegative user ids and SDK-dependent layouts 31–39 still throw. This does not implement the console's UserService language/variant queries or recover their error propagation and SDK compatibility mapping; console equivalence is unverified.
