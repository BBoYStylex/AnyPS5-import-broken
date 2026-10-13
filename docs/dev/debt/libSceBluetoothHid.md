# libSceBluetoothHid technical debt

### Silent stubs

- [libSceBluetoothHid](../../../core/libs/prx/libSceBluetoothHid/Export.cpp) implements only the no-device path: initialization and callback and device registration succeed, the callback is never called because no device connects, and calls that need a connected device throw

### Unknown function info

- [libSceBluetoothHid](../../../core/libs/prx/libSceBluetoothHid/Export.cpp) (libSceBluetoothHid) - signatures taken from PPSA01317: `sceBluetoothHidInit()` and `sceBluetoothHidUnregisterCallback()` without arguments, `sceBluetoothHidRegisterCallback(callback, 0, param)` with a 16-byte param, and `sceBluetoothHidRegisterDevice` / `sceBluetoothHidUnregisterDevice(vendorId, productId)`; the `sceBluetoothHidGetDeviceName(handle, nameOut)`, `sceBluetoothHidGetInputReport(report, reportId)` and `sceBluetoothHidSetOutputReport(handle, reportId, data, length)` argument shapes follow [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/main/src/SharpProspero/Interop/Bluetooth/SceBluetoothHid.cs), recovered from the module binary (counts, pointer-versus-value and scalar widths). The layouts behind the pointer arguments, the handle and device identity semantics and the console's error codes are unverified; with no device connected the calls throw, and a null output pointer throws before the device check
