import os
import re
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import import_audit
import nid_names

LIBRARIES = {
    "libkernel_sys": [
        "__freeze",
        "__getcwd",
        "__sceKernelGetGPI",
        "__sys_debug_init",
        "__sys_dl_get_info",
        "__sys_dl_get_info_2",
        "__sys_dl_get_list",
        "__sys_dl_get_metadata",
        "__sys_dynlib_get_info2",
        "__sys_dynlib_get_info_for_libdbg",
        "__sys_dynlib_get_list2",
        "__sys_dynlib_get_list_for_libdbg",
        "__sys_dynlib_load_prx",
        "__sys_get_proc_type_info",
        "__sys_is_development_mode",
        "__sys_kqueueex",
        "__sys_namedobj_create",
        "__sys_namedobj_delete",
        "__sys_nanosleep",
        "__sys_netabort",
        "__sys_netcontrol",
        "__sys_netgetiflist",
        "__sys_netgetsockinfo",
        "__sys_opmc_disable",
        "__sys_opmc_enable",
        "__sys_opmc_get_ctr",
        "__sys_opmc_get_hw",
        "__sys_opmc_set_ctl",
        "__sys_opmc_set_ctr",
        "__sys_opmc_set_hw",
        "__sys_osem_close",
        "__sys_osem_open",
        "__sys_randomized_path",
        "__sys_rdup",
        "__sys_regmgr_call",
        "__sys_resume_internal_hdd",
        "__sys_resume_process",
        "__sys_set_uevt",
        "__sys_socketclose",
        "__sys_suspend_process",
        "__sys_test_debug_rwmem",
        "__sys_workaround8849",
        "_accept",
        "_bind",
        "_connect",
        "_dup2",
        "_execve",
        "_execvpe",
        "_fpathconf",
        "_getdirentries",
        "_getpeername",
        "_getsockname",
        "_getsockopt",
        "_listen",
        "_openat",
        "_recvfrom",
        "_recvmsg",
        "_sendmsg",
        "_sendto",
        "_setsockopt",
        "_sigaction",
        "_sigsuspend",
        "_wait4",
        "fpathconf",
        "pthread_cond_signalto_np",
        "pthread_condattr_getclock",
        "pthread_mutexattr_getkind_np",
        "pthread_mutexattr_setkind_np",
        "pthread_rwlockattr_gettype_np",
        "sceKernelCallIndirectBuffer3",
        "sceKernelGetAppInfoListForMinisyscore",
        "sceKernelGetCpuCoreClock",
        "sceKernelGetGenuineProsperoSystemSwVersion",
        "sceKernelGetNandInfo",
        "sceKernelGetPrefixVersion",
        "sceKernelGetProsperoCompiledSdkVersion",
        "sceKernelGetProsperoCompiledSdkVersionByPid",
        "sceKernelGetPs4CompiledSdkVersion",
        "sceKernelGetPs4CompiledSdkVersionByPid",
        "sceKernelGetToolMemoryRange",
        "sceKernelGetUpdatemode",
        "sceKernelIccGetBDDriveChuckingState",
        "sceKernelIccGetEmcBtDownload",
        "sceKernelIccGetFloydVersion",
        "sceKernelIccGetThermalAlert",
        "sceKernelIccGetWlanBtGpio",
        "sceKernelIccIsThermalAlert",
        "sceKernelIccSetBootTimestamp",
        "sceKernelIccSetEmcBtDownload",
        "sceKernelIccSetPriorityBuzzer",
        "sceKernelIccSetWlanBtGpio",
        "sceKernelIccSetWlanBtManuTestMode",
        "sceKernelMapperGetUsageStatsData",
        "sceKernelMapperSetPageTablePoolOccupancyNotificationThreshold",
        "scePthreadRegisterThreadDeathHook",
        "setpgid",
    ],
    "libkernel": [
        "__freeze",
        "__sceKernelGetGPI",
        "__sys_debug_init",
        "__sys_dl_get_info",
        "__sys_dl_get_info_2",
        "__sys_dl_get_list",
        "__sys_dl_get_metadata",
        "__sys_dynlib_get_info2",
        "__sys_dynlib_get_info_for_libdbg",
        "__sys_dynlib_get_list2",
        "__sys_dynlib_get_list_for_libdbg",
        "__sys_dynlib_load_prx",
        "__sys_get_proc_type_info",
        "__sys_is_development_mode",
        "__sys_kqueueex",
        "__sys_namedobj_create",
        "__sys_namedobj_delete",
        "__sys_netabort",
        "__sys_netcontrol",
        "__sys_netgetiflist",
        "__sys_netgetsockinfo",
        "__sys_opmc_disable",
        "__sys_opmc_enable",
        "__sys_opmc_get_ctr",
        "__sys_opmc_get_hw",
        "__sys_opmc_set_ctl",
        "__sys_opmc_set_ctr",
        "__sys_opmc_set_hw",
        "__sys_osem_close",
        "__sys_osem_open",
        "__sys_randomized_path",
        "__sys_rdup",
        "__sys_regmgr_call",
        "__sys_set_uevt",
        "__sys_socketclose",
        "__sys_test_debug_rwmem",
        "__sys_workaround8849",
        "_accept",
        "_bind",
        "_connect",
        "_dup2",
        "_execve",
        "_execvpe",
        "_fpathconf",
        "_getdirentries",
        "_getpeername",
        "_getsockname",
        "_getsockopt",
        "_listen",
        "_openat",
        "_recvfrom",
        "_recvmsg",
        "_sendmsg",
        "_sendto",
        "_setsockopt",
        "_sigaction",
        "_sigsuspend",
        "_wait4",
        "pthread_cond_signalto_np",
        "pthread_condattr_getclock",
        "pthread_mutexattr_getkind_np",
        "pthread_mutexattr_setkind_np",
        "pthread_rwlockattr_gettype_np",
        "sceKernelCallIndirectBuffer3",
        "sceKernelGetCpuCoreClock",
        "sceKernelGetGenuineProsperoSystemSwVersion",
        "sceKernelGetPrefixVersion",
        "sceKernelGetProsperoCompiledSdkVersion",
        "sceKernelGetProsperoCompiledSdkVersionByPid",
        "sceKernelGetPs4CompiledSdkVersion",
        "sceKernelGetPs4CompiledSdkVersionByPid",
        "sceKernelGetToolMemoryRange",
        "sceKernelIccGetBDDriveChuckingState",
        "sceKernelIccGetThermalAlert",
        "sceKernelIccSetPriorityBuzzer",
        "sceKernelMapperGetUsageStatsData",
        "sceKernelMapperSetPageTablePoolOccupancyNotificationThreshold",
        "scePthreadRegisterThreadDeathHook",
    ],
    "libSceAgcDriver": [
        "sceAgcDriverDeregisterCaptureInterface",
        "sceAgcDriverIsCoredumpValid",
        "sceAgcDriverIsSuspendPointInFlightDirect",
        "sceAgcDriverRegisterCaptureInterface",
        "sceAgcDriverReleaseRazorACQ",
        "sceAgcDriverResetGc",
        "sceAgcDriverSdmaCopyLinearBlocking",
        "sceAgcDriverSetTFRingDirect",
    ],
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
        """Every file of the library that holds pending exports."""
        directory = self.root / "core" / "libs" / "prx" / library
        sources = []
        for path in sorted(directory.rglob("*.cpp")):
            text = path.read_text(encoding="utf-8", errors="replace")
            if "NotImplemented_nid_no_patch" in text:
                sources.append(text)
        if not sources:
            raise AssertionError(f"{library} holds no pending export")
        return "\n".join(sources)

    def exports(self, library):
        path = self.patched / f"{library}.prx"
        if not path.exists():
            self.skipTest(f"{path} is not built")
        return import_audit.read_exports(path)

    def declared(self, library):
        """The pending exports a library declares, without the NID suffix."""
        return {nid_names.strip_nid_postfix(match.group("name"))
                for match in PENDING.finditer(self.source(library))}

    def present(self, library):
        """The libraries the tree has: one whose names are all missing is not part of it."""
        declared = self.declared(library)
        if not any(name in declared for name in LIBRARIES[library]):
            return False
        return True

    def test_every_pending_export_is_declared_as_a_throwing_function(self):
        for library, names in LIBRARIES.items():
            if not self.present(library):
                continue
            declared = self.declared(library)
            for name in names:
                with self.subTest(library=library, export=name):
                    self.assertIn(name, declared)

    def test_every_pending_export_resolves_by_its_nid(self):
        for library, names in LIBRARIES.items():
            if not self.present(library):
                continue
            exports = self.exports(library)
            for name in names:
                with self.subTest(library=library, export=name):
                    self.assertIn(nid_names.compute_nid(name), exports)

    def test_a_pending_export_does_not_keep_its_plain_name(self):
        for library, names in LIBRARIES.items():
            if not self.present(library):
                continue
            exports = self.exports(library)
            for name in names:
                with self.subTest(library=library, export=name):
                    self.assertNotIn(name, exports)


if __name__ == "__main__":
    unittest.main()
