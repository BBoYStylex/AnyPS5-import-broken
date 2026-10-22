#!/usr/bin/env python3
import re
import sys
import unittest
from pathlib import Path

SOURCE_EXPORTS = (
    "sceCamera2Close",
    "sceCamera2Finalize",
    "sceCamera2GetAttribute",
    "sceCamera2GetAutoExposureGain",
    "sceCamera2GetAutoWhiteBalance",
    "sceCamera2GetCalibrationData",
    "sceCamera2GetConfig",
    "sceCamera2GetContrast",
    "sceCamera2GetExposureGain",
    "sceCamera2GetFieldOfView",
    "sceCamera2GetFrameData",
    "sceCamera2GetGamma",
    "sceCamera2GetHue",
    "sceCamera2GetSaturation",
    "sceCamera2GetSharpness",
    "sceCamera2GetWhiteBalance",
    "sceCamera2Initialize",
    "sceCamera2IsAttached",
    "sceCamera2IsValidFrameData",
    "sceCamera2Open",
    "sceCamera2SetAttribute",
    "sceCamera2SetAutoExposureGain",
    "sceCamera2SetAutoWhiteBalance",
    "sceCamera2SetConfig",
    "sceCamera2SetContrast",
    "sceCamera2SetExposureGain",
    "sceCamera2SetGamma",
    "sceCamera2SetHue",
    "sceCamera2SetSaturation",
    "sceCamera2SetSharpness",
    "sceCamera2SetVideoSync",
    "sceCamera2SetWhiteBalance",
    "sceCamera2Start",
    "sceCamera2Stop",
    "sceCameraAccGetData",
    "sceCameraAudioClose",
    "sceCameraAudioGetData",
    "sceCameraAudioGetData2",
    "sceCameraAudioOpen",
    "sceCameraAudioReset",
    "sceCameraChangeAppModuleState",
    "sceCameraClose",
    "sceCameraCloseByHandle",
    "sceCameraDeviceOpen",
    "sceCameraGetAttribute",
    "sceCameraGetAutoExposureGain",
    "sceCameraGetAutoWhiteBalance",
    "sceCameraGetCalibData",
    "sceCameraGetCalibDataForEve",
    "sceCameraGetCalibDataFromDevice",
    "sceCameraGetCalibDataFromDeviceForEve",
    "sceCameraGetCalibrationData",
    "sceCameraGetConfig",
    "sceCameraGetConnectedDeviceAndDriverMode",
    "sceCameraGetContrast",
    "sceCameraGetDefectivePixelCancellation",
    "sceCameraGetDeviceConfig",
    "sceCameraGetDeviceConfigWithoutHandle",
    "sceCameraGetDeviceID",
    "sceCameraGetDeviceIDWithoutOpen",
    "sceCameraGetDeviceInfo",
    "sceCameraGetExposureGain",
    "sceCameraGetFrameData",
    "sceCameraGetGamma",
    "sceCameraGetHue",
    "sceCameraGetLensCorrection",
    "sceCameraGetMmapConnectedCount",
    "sceCameraGetProductInfo",
    "sceCameraGetRegister",
    "sceCameraGetRegistryInfo",
    "sceCameraGetSaturation",
    "sceCameraGetSharpness",
    "sceCameraGetVrCaptureInfo",
    "sceCameraGetWhiteBalance",
    "sceCameraInitializeRegistryCalibData",
    "sceCameraIsAttached",
    "sceCameraIsConfigChangeDone",
    "sceCameraIsConnectedForSystem",
    "sceCameraIsValidFrameData",
    "sceCameraOpen",
    "sceCameraOpenByModuleId",
    "sceCameraRemoveAppModuleFocus",
    "sceCameraSetAppModuleFocus",
    "sceCameraSetAttribute",
    "sceCameraSetAttributeInternal",
    "sceCameraSetAutoExposureGain",
    "sceCameraSetAutoWhiteBalance",
    "sceCameraSetCalibData",
    "sceCameraSetCalibDataForEve",
    "sceCameraSetConfig",
    "sceCameraSetConfigInternal",
    "sceCameraSetContrast",
    "sceCameraSetDebugStop",
    "sceCameraSetDefectivePixelCancellation",
    "sceCameraSetDefectivePixelCancellationInternal",
    "sceCameraSetExposureGain",
    "sceCameraSetForceActivate",
    "sceCameraSetGamma",
    "sceCameraSetHue",
    "sceCameraSetLensCorrection",
    "sceCameraSetLensCorrectionInternal",
    "sceCameraSetProcessFocus",
    "sceCameraSetProcessFocusByHandle",
    "sceCameraSetRegister",
    "sceCameraSetSaturation",
    "sceCameraSetSharpness",
    "sceCameraSetTrackerMode",
    "sceCameraSetUacModeInternal",
    "sceCameraSetVideoSync",
    "sceCameraSetVideoSyncInternal",
    "sceCameraSetWhiteBalance",
    "sceCameraStart",
    "sceCameraStartByHandle",
    "sceCameraStop",
    "sceCameraStopByHandle",
    "sceCameraWaitForEveReady",
    "sceCameraWaitForLukeReady",
)
PENDING = re.compile(
    r"^int APS5_VABI (?P<name>\w+)\(void\) \{\n"
    r"    NotImplemented_nid_no_patch\(__func__\);\n"
    r"    return 0;\n"
    r"\}$", re.MULTILINE)
EXPORT = re.compile(r"\bAPS5_VABI\s+(\w+)\s*\([^;{]*\)\s*\{")


def tools_directory(library):
    for parent in [library, *library.parents]:
        if (parent / "tools" / "import_audit.py").is_file():
            return parent / "tools"
    raise SystemExit("the tools directory is not reachable from " + str(library))


if len(sys.argv) != 3:
    raise SystemExit("usage: test_export_nids.py <library source directory> <patched library>")
LIBRARY = Path(sys.argv[1]).resolve()
PATCHED = Path(sys.argv[2]).resolve()
sys.path.insert(0, str(tools_directory(LIBRARY)))
import import_audit
import nid_names


class PendingExportNidTests(unittest.TestCase):
    def source(self):
        return (LIBRARY / "Export.cpp").read_text(encoding="utf-8", errors="replace")

    def declared(self):
        return {match.group("name") for match in PENDING.finditer(self.source())}

    def exports(self):
        if not PATCHED.is_file():
            self.skipTest(str(PATCHED) + " is not built")
        return import_audit.read_exports(PATCHED)

    def test_every_source_export_is_declared_as_a_pending_export(self):
        declared = self.declared()
        for name in SOURCE_EXPORTS:
            with self.subTest(export=name):
                self.assertIn(name, declared)

    def test_every_declared_export_resolves_by_its_nid(self):
        exports = self.exports()
        for name in EXPORT.findall(self.source()):
            with self.subTest(export=name):
                self.assertIn(nid_names.compute_nid(name), exports)

    def test_a_source_export_resolves_by_its_nid(self):
        exports = self.exports()
        for name in SOURCE_EXPORTS:
            with self.subTest(export=name):
                self.assertIn(nid_names.compute_nid(name), exports)

    def test_a_pending_export_does_not_keep_its_plain_name(self):
        exports = self.exports()
        for name in self.declared():
            with self.subTest(export=name):
                self.assertNotIn(name, exports)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
