#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

static constexpr int SCE_NP_ERROR_INVALID_ARGUMENT = static_cast<int>(0x80550003);
static constexpr int SCE_NP_ERROR_SIGNED_OUT = static_cast<int>(0x80550006);

extern "C" {

int SceNpUtilityModuleLoaded_nid_no_patch = 1;

int APS5_VABI sceNpBandwidthTestAbort(int context_id) {
    (void)context_id;
    return SCE_NP_ERROR_INVALID_ARGUMENT;
}

int APS5_VABI sceNpBandwidthTestGetStatus(int context_id, int* status) {
    (void)context_id;
    if (status) *status = 0;
    return SCE_NP_ERROR_INVALID_ARGUMENT;
}

int APS5_VABI sceNpBandwidthTestInitStartDownload(const void* param) {
    (void)param;
    return SCE_NP_ERROR_SIGNED_OUT;
}

int APS5_VABI sceNpBandwidthTestInitStartUpload(const void* param) {
    (void)param;
    return SCE_NP_ERROR_SIGNED_OUT;
}

int APS5_VABI sceNpBandwidthTestShutdown(int context_id, void* result) {
    (void)context_id;
    (void)result;
    return SCE_NP_ERROR_INVALID_ARGUMENT;
}

int APS5_VABI sceNpAppInfoIntAbortRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppInfoIntCheckAvailability() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppInfoIntCheckAvailabilityA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppInfoIntCheckAvailabilityAll() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppInfoIntCheckAvailabilityAllA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppInfoIntCheckServiceAvailability() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppInfoIntCheckServiceAvailabilityA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppInfoIntCheckServiceAvailabilityAll() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppInfoIntCheckServiceAvailabilityAllA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppInfoIntCreateRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppInfoIntDestroyRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppInfoIntFinalize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppInfoIntInitialize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppLaunchLink2IntAbortRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppLaunchLink2IntCreateRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppLaunchLink2IntDestroyRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppLaunchLink2IntFinalize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppLaunchLink2IntGetCompatibleTitleIdList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppLaunchLink2IntGetCompatibleTitleIdNum() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppLaunchLink2IntInitialize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppLaunchLinkIntAbortRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppLaunchLinkIntCreateRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppLaunchLinkIntDestroyRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppLaunchLinkIntFinalize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppLaunchLinkIntGetCompatibleTitleIdList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppLaunchLinkIntGetCompatibleTitleIdNum() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAppLaunchLinkIntInitialize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpBandwidthTestShutdownWithDetailedInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupAbortRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupCreateAsyncRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupCreateTitleCtxA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetAbortRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetCensorComment() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetConvertJidToNpId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetConvertNpIdToJid() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetCreateRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetCreateTitleCtx() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetDeleteRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetDeleteTitleCtx() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetInit() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetInitWithFunctionPointer() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetInitWithMemoryPool() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetIsInit() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetNpId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetSanitizeComment() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetSetTimeout() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupNetTerm() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupPollAsync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupSetTimeout() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpLookupWaitAsync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceChecker2IntAbortRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceChecker2IntCreateRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceChecker2IntDestroyRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceChecker2IntFinalize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceChecker2IntGetServiceAvailability() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceChecker2IntGetServiceAvailabilityA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceChecker2IntInitialize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceChecker2IntIsSetServiceType() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceCheckerIntAbortRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceCheckerIntCreateRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceCheckerIntDestroyRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceCheckerIntFinalize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceCheckerIntGetAvailability() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceCheckerIntGetAvailabilityList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceCheckerIntInitialize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceCheckerIntIsCached() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpTitleMetadataIntAbortRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpTitleMetadataIntCreateRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpTitleMetadataIntDeleteRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpTitleMetadataIntGetInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpTitleMetadataIntGetNpTitleId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpUtilityInit() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpUtilityTerm() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpWordFilterCreateTitleCtxA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpWordFilterSetTimeout() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpWordFilterWaitAsync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
}
