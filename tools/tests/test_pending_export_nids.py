import os
import re
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import import_audit
import nid_names

LIBRARIES = {
    "libSceVideoOut": [
        "sceVideoOutAddBuffer",
        "sceVideoOutAddBuffer4k2kPrivilege",
        "sceVideoOutAddBufferAnisoPrivilege",
        "sceVideoOutAddBufferHdrPrivilege",
        "sceVideoOutAddBufferYccPrivilege",
        "sceVideoOutAddDisplayPositionEvent",
        "sceVideoOutAddDriver",
        "sceVideoOutAddS3dHmdPrivilege",
        "sceVideoOutAddSetModeEvent",
        "sceVideoOutAddStereoBuffer",
        "sceVideoOutAdjustColor2_",
        "sceVideoOutConfigureLayout",
        "sceVideoOutConfigureOptionsInitialize_",
        "sceVideoOutConfigureOutputEx",
        "sceVideoOutConfigureOutputModeExSubmitDoneUnsafe_",
        "sceVideoOutConfigureOutputModeEx_",
        "sceVideoOutConfigureOutputMode_",
        "sceVideoOutControlHdcpEncryption",
        "sceVideoOutCursorIsUpdatePending",
        "sceVideoOutCursorSet2xMagnify",
        "sceVideoOutCursorSetPositionStereo",
        "sceVideoOutCurveSamplePointsCalc_",
        "sceVideoOutDeleteDisplayPositionEvent",
        "sceVideoOutDeleteDriver",
        "sceVideoOutDeleteSetModeEvent",
        "sceVideoOutDriverAddEvent",
        "sceVideoOutDriverDecrementBufferLabel",
        "sceVideoOutDriverDeleteEvent",
        "sceVideoOutDriverGetBufferLabelAddress",
        "sceVideoOutDriverGetEopHandle",
        "sceVideoOutDriverGetFinishedEopHandle",
        "sceVideoOutDriverGetLatestEopHandle",
        "sceVideoOutDriverIncrementBufferLabel",
        "sceVideoOutDriverResetBufferLabel",
        "sceVideoOutDriverTriggerEvent",
        "sceVideoOutGetBufferInformation",
        "sceVideoOutGetCoredumpOutputMode",
        "sceVideoOutGetCurrentOutputMode_",
        "sceVideoOutGetDeviceCapabilityInfo_",
        "sceVideoOutGetDeviceInfoExOts_",
        "sceVideoOutGetDeviceInfoEx_",
        "sceVideoOutGetHdmiKsvList_",
        "sceVideoOutGetHdmiMonitorInfoNoMask_",
        "sceVideoOutGetHdmiMonitorInfo_",
        "sceVideoOutGetHdmiRawEdid_",
        "sceVideoOutGetMonitorInfo",
        "sceVideoOutGetPipelineStatus",
        "sceVideoOutGetPortStatusInfo",
        "sceVideoOutGetPortStatusInfoByBusSpecifier_",
        "sceVideoOutGetPortStatusInfo_",
        "sceVideoOutGetStatusForWebcore",
        "sceVideoOutGetVideoOutModeByBusSpecifier_",
        "sceVideoOutHdmiMonitorInfoIsSupportedHdcpVersion",
        "sceVideoOutHdmiMonitorInfoIsSupportedVideoOutMode_",
        "sceVideoOutLockBuffer",
        "sceVideoOutModeSetAny_",
        "sceVideoOutRazorAddCallback",
        "sceVideoOutRazorRecallCallbacks",
        "sceVideoOutRazorRemoveCallback",
        "sceVideoOutRegisterBufferAttribute",
        "sceVideoOutRegisterStereoBuffers",
        "sceVideoOutRemoveBuffer",
        "sceVideoOutSetAlphaControl",
        "sceVideoOutSetBlankAfterResume",
        "sceVideoOutSetContentType",
        "sceVideoOutSetDisplayEventPosition",
        "sceVideoOutSetGamutMetadata_",
        "sceVideoOutSetToneMap_",
        "sceVideoOutSubmitChangeBufferAttribute",
        "sceVideoOutSysAddSetModeEvent",
        "sceVideoOutSysAddSetModeEvent2",
        "sceVideoOutSysAddVblankEvent",
        "sceVideoOutSysAddVblankEvent2",
        "sceVideoOutSysClose",
        "sceVideoOutSysConfigureLayout",
        "sceVideoOutSysConfigureOutput",
        "sceVideoOutSysConfigureOutputMode",
        "sceVideoOutSysCursorDisable",
        "sceVideoOutSysCursorEnable",
        "sceVideoOutSysCursorIsUpdatePending",
        "sceVideoOutSysCursorObtain",
        "sceVideoOutSysCursorRelease",
        "sceVideoOutSysCursorSetPosition",
        "sceVideoOutSysDeleteVblankEvent",
        "sceVideoOutSysDeleteVblankEvent2",
        "sceVideoOutSysGetBus",
        "sceVideoOutSysGetColorConversionStatus",
        "sceVideoOutSysGetCurrentOutputMode",
        "sceVideoOutSysGetDeviceInfo",
        "sceVideoOutSysGetFlipStatus",
        "sceVideoOutSysGetMonitorInfo_",
        "sceVideoOutSysGetPipelineStatus",
        "sceVideoOutSysGetResolutionStatus",
        "sceVideoOutSysGetResolutionStatus2",
        "sceVideoOutSysGetVblankStatus",
        "sceVideoOutSysGetVblankStatus2",
        "sceVideoOutSysGetVideoOutModeByBusSpecifier",
        "sceVideoOutSysHdmiMonitorInfoIsMorpheus",
        "sceVideoOutSysIsModeS3d",
        "sceVideoOutSysIsSupportedByMonitorInfo_",
        "sceVideoOutSysIsUserStatusSystemDefault",
        "sceVideoOutSysIsUserStatusVr",
        "sceVideoOutSysModeIsSystemDefault",
        "sceVideoOutSysModeIsVr",
        "sceVideoOutSysOpenInternal",
        "sceVideoOutSysReadCrc",
        "sceVideoOutSysReadCrc64",
        "sceVideoOutSysResetAtGpuReset",
        "sceVideoOutSysSetColorMode",
        "sceVideoOutSysSetDcePipeMode",
        "sceVideoOutSysSetDimmer",
        "sceVideoOutSysSetDimmer2",
        "sceVideoOutSysSetHdcpEncryption",
        "sceVideoOutSysSetHdrMetadata",
        "sceVideoOutSysSetOverscanRatio",
        "sceVideoOutSysUpdateDisplayParameter",
        "sceVideoOutSysUpdatePrivilege",
        "sceVideoOutSysUpdateRenderingMode",
        "sceVideoOutUnlockBuffer",
        "sceVideoOutUnregisterBufferAttribute",
    ],
}
PENDING = re.compile(
    r"^int APS5_VABI (?P<name>\w+)\(void\) \{\n"
    r"    NotImplemented_nid_no_patch\(__func__\);\n"
    r"    return 0;\n"
    r"\}$", re.MULTILINE)


class PendingExportNidTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(os.environ.get("ANYPS5_SOURCE_ROOT", Path.cwd())).resolve()
        self.patched = self.root / "build" / "core" / "libs" / "libs"

    def source(self, library):
        directory = self.root / "core" / "libs" / "prx" / library
        for path in sorted(directory.rglob("*.cpp")):
            text = path.read_text(encoding="utf-8", errors="replace")
            if "NotImplemented_nid_no_patch" in text:
                return text
        raise AssertionError(f"{library} holds no pending export")

    def exports(self, library):
        path = self.patched / f"{library}.prx"
        if not path.exists():
            self.skipTest(f"{path} is not built")
        return import_audit.read_exports(path)

    def test_every_pending_export_is_declared_as_a_throwing_function(self):
        for library, names in LIBRARIES.items():
            declared = {match.group("name") for match in PENDING.finditer(self.source(library))}
            for name in names:
                with self.subTest(library=library, export=name):
                    self.assertIn(name, declared)

    def test_every_pending_export_resolves_by_its_nid(self):
        for library, names in LIBRARIES.items():
            exports = self.exports(library)
            for name in names:
                with self.subTest(library=library, export=name):
                    self.assertIn(nid_names.compute_nid(name), exports)

    def test_a_pending_export_does_not_keep_its_plain_name(self):
        for library, names in LIBRARIES.items():
            exports = self.exports(library)
            for name in names:
                with self.subTest(library=library, export=name):
                    self.assertNotIn(name, exports)


if __name__ == "__main__":
    unittest.main()
