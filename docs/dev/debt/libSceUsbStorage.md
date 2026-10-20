# libSceUsbStorage technical debt

### Unknown function info

- [sceUsbStorageInit, sceUsbStorageTerm, sceUsbStorageGetDeviceList, sceUsbStorageGetMountPointOfShellCore and sceUsbStorageIsExist](../../../core/libs/prx/libSceUsbStorage/Export.cpp) (libSceUsbStorage) - PS5 error precedence and repeated initialization have not been measured. The lifecycle, list and mount-path signatures, eight-device limit, 128-byte output and `INVALID_ARGUMENT` / `CLIENT_NOT_VALID` codes follow [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/5dfdb4b0b166e83dc9ae275001afb695f12c7174/src/SharpProspero/Platform/UsbStorage.cs). The presence-query signature and missing-device code follow [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/a808e7626e3c7de8d0f0fda6a91f620b42f6c762/src/core/libraries/usbd/usbstorage.cpp). Non-null initialization attributes and repeated initialization throw; calls without an initialized client return `CLIENT_NOT_VALID`.

### Functional

- [libSceUsbStorage](../../../core/libs/prx/libSceUsbStorage/Export.cpp) (libSceUsbStorage) - physical USB devices and hotplug callbacks are not exposed, and host filesystem failures other than missing or non-directory paths throw. Directory-backed guest mounts `/mnt/usb0` through `/mnt/usb7`, resolved by the existing guest path mapping, are reported as storage devices; regular files are not devices. Queries rescan the paths, but mapping, device-info, callback and directory-handle exports still throw. Termination invalidates the client without removing its folders or path aliases.
