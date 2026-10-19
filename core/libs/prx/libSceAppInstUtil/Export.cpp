#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceAppInstUtilInitialize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilTerminate(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppInstallAll(void* reserved) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppInstallTitleDir(const char* titleId, const char* titleDir, void* reserved) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppIsInInstalling(int32_t* installing) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppIsInUpdating() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppExists(const char* titleId, int32_t* exists) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppGetSize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppInstallPkg() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppPrepareOverwritePkg() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppRecoverApp() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppUnInstall(const char* titleId) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppUnInstallAddcont() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppUnInstallPat() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppUnInstallTheme() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppUnInstallTypes() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilGetInstallProgressInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilGetInstallProgress() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilGetPrimaryAppSlot() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppAllowDownloadInstall(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppCancelGetAllAppSize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppCancelGetAppOtherSize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppCancelMoveApps(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppCancelUnInstall(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppConvertAppDiscToDownload(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppConvertAppDownloadToDisc(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppDestroyMetadata(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppDestroyPkg(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetAddcontInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetAddcontInfoByContentId(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetAddcontListTotalSize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetAddcontNum(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetAllAppSize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetAppInstallStatus(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetAppInstallStatusByAppInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetAppOtherSize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetCompilationDiscInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetCustomThemeInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetInsertedDiscMasterDataId(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetInsertedDiscTitleId(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetInsertedDiscTitleIdList(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetInstallAddcontFromDiscStatus(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetIrremovableAppList(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetMoveErrorAppList(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetNoRelatedAddcontTitleList(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppGetStoreCompilationDiscInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppInstallCloudGame(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppInstallContentPush(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppInstallGetLinkState(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppInstallMediaPlayer(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppInstallPs4CloudGame(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppInstallPsNowIcon(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppInstallRequestAddcontFromDisc(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppInstallRequestCompilationDiscApp(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppInstallSharePlayLink(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppInstallSharePlayLinkByUser(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppProhibitDownloadInstall(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppRequestMoveApps(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppSetRemasterInstallType(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppSetSavedataUploadStatus(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppStoreCompilationDiscInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppUnInstall2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppUnInstallForMultiInstall(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppUnInstallForRemote(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilAppUpdateStartTime(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilCancelDataDiscCopy(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilCheckAppSystemVer(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetAddcontInfoList2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetAddcontInfoList3(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetAddcontInstalledStatus(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetAddons(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetAppEmptySlot(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetAppMoveProgressInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetAppSlotInfoList(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetConfigList(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetContentSize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetContentSize2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetDataDiscCopyProgress(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetInsertedDiscRoleType(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetInsertedDiscTotal(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetInstallProgressInfoForMultiDisc(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetInstallStatus(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetMainAppTitleId(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetMainAppTitleIdForSP(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetParentalControlLevel(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetPatchInstallStatus(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetRestoreState(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetSubAppTitleId(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilGetSubAppTitleIdForSP(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilInstall(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilInstallByPackage(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilInstallPatch(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilJson(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilMountBdEmu(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilPauseInstall(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilRegisterDownload(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilRequestDataDiscCopy(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilRequestInstallDiscGame(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilRestoreDataSource(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilResumeInstall(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilSetPrimaryAppSlot(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilStartInstall(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilSwitchConfig(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAppInstUtilUnmountBdEmu(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
