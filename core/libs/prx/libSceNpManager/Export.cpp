#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceUserService/UserService.hpp"
#include <array>
#include <atomic>
#include <mutex>
#include <unordered_set>

// PSN is not emulated: the user is reported as signed out and online queries fail.
static constexpr int SCE_NP_ERROR_INVALID_ARGUMENT = static_cast<int>(0x80550003);
static constexpr int SCE_NP_ERROR_SIGNED_OUT = static_cast<int>(0x80550006);
static constexpr int SCE_NP_ERROR_USER_NOT_FOUND = static_cast<int>(0x80550007);
static constexpr int SCE_NP_ERROR_CALLBACK_ALREADY_REGISTERED = static_cast<int>(0x80550008);
static constexpr int SCE_NP_ERROR_CALLBACK_NOT_REGISTERED = static_cast<int>(0x80550009);
static constexpr int SCE_NP_ERROR_REQUEST_NOT_FOUND = static_cast<int>(0x80550014);
static constexpr int SCE_NP_ERROR_CALLBACK_MAX = static_cast<int>(0x8055001D);
static constexpr uint32_t NP_STATE_SIGNED_OUT = 1;
static constexpr int NP_POLL_ASYNC_FINISHED = 0;
static constexpr uint32_t NP_REACHABILITY_STATE_UNAVAILABLE = 0;

static std::atomic<int> g_nextRequest{1};

namespace {
std::mutex reachabilityMutex;
void* reachabilityCallback = nullptr;
void* reachabilityUserdata = nullptr;
std::mutex requestMutex;
std::unordered_set<int> liveRequests;

int AddRequest() {
    const int reqId = g_nextRequest.fetch_add(1, std::memory_order_relaxed);
    std::lock_guard lock(requestMutex);
    liveRequests.insert(reqId);
    return reqId;
}

using NpStateCallbackA = void (APS5_VABI *)(int userId, uint32_t state, void* userdata);
using NpStateCallback = void (APS5_VABI *)(int userId, uint32_t state, NpId* npId, void* userdata);

constexpr int NP_STATE_CALLBACK_MAX = 8;

struct StateCallbackA {
    NpStateCallbackA callback = nullptr;
    void* userdata = nullptr;
    bool pending = false;
};

struct StateCallback {
    NpStateCallback callback = nullptr;
    void* userdata = nullptr;
    bool pending = false;
};

std::mutex stateMutex;
std::array<StateCallbackA, NP_STATE_CALLBACK_MAX> stateCallbacksA;
StateCallback stateCallback;
}

extern "C" {

int APS5_VABI sceNpAbortRequest(int req_id) {
    (void)req_id;
    return 0;
}

int APS5_VABI sceNpCheckCallback(void) {
    std::array<StateCallbackA, NP_STATE_CALLBACK_MAX> callbacksA;
    StateCallback legacy;
    {
        std::lock_guard lock(stateMutex);
        callbacksA = stateCallbacksA;
        legacy = stateCallback;
        for (auto& entry : stateCallbacksA) entry.pending = false;
        stateCallback.pending = false;
    }
    if (legacy.pending) legacy.callback(USER_SERVICE_INITIAL_USER_ID, NP_STATE_SIGNED_OUT, nullptr, legacy.userdata);
    for (const auto& entry : callbacksA) {
        if (entry.pending) entry.callback(USER_SERVICE_INITIAL_USER_ID, NP_STATE_SIGNED_OUT, entry.userdata);
    }
    return 0;
}

int APS5_VABI sceNpCheckNpAvailability(int req_id, const NpOnlineId* online_id) {
 (void)req_id;
 if (!online_id) return SCE_NP_ERROR_INVALID_ARGUMENT;
 return SCE_NP_ERROR_USER_NOT_FOUND;
}

int APS5_VABI sceNpCheckNpReachability(int req_id, int user_id) {
 (void)req_id;
 (void)user_id;
 return SCE_NP_ERROR_SIGNED_OUT;
}

int APS5_VABI sceNpCheckPremium(int req_id, const NpCheckPremiumParameter* param, NpCheckPremiumResult* result) {
    (void)req_id;
    (void)param;
    (void)result;
    return SCE_NP_ERROR_SIGNED_OUT;
}

int APS5_VABI sceNpCreateAsyncRequest(const NpCreateAsyncRequestParameter* param) {
    (void)param;
    return AddRequest();
}

int APS5_VABI sceNpCreateRequest(void) {
    return AddRequest();
}

int APS5_VABI sceNpDeleteRequest(int req_id) {
    std::lock_guard lock(requestMutex);
    liveRequests.erase(req_id);
    return 0;
}

int APS5_VABI sceNpGetAccountAge(int req_id, int user_id, uint8_t* age) {
 (void)req_id;
 (void)user_id;
 if (!age) return SCE_NP_ERROR_INVALID_ARGUMENT;
 return SCE_NP_ERROR_SIGNED_OUT;
}

int APS5_VABI sceNpGetAccountCountryA(int user_id, void* country_code) {
    (void)user_id;
    if (!country_code) return SCE_NP_ERROR_INVALID_ARGUMENT;
    return SCE_NP_ERROR_SIGNED_OUT;
}

int APS5_VABI sceNpGetAccountIdA(int user_id, uint64_t* account_id) {
    (void)user_id;
    (void)account_id;
    return SCE_NP_ERROR_SIGNED_OUT;
}

int APS5_VABI sceNpGetNpId(int user_id, NpId* np_id) {
    (void)user_id;
    if (!np_id) return SCE_NP_ERROR_INVALID_ARGUMENT;
    return SCE_NP_ERROR_SIGNED_OUT;
}

int APS5_VABI sceNpGetNpReachabilityState(int user_id, uint32_t* state) {
    (void)user_id;
    if (!state) return SCE_NP_ERROR_INVALID_ARGUMENT;
    *state = NP_REACHABILITY_STATE_UNAVAILABLE;
    return 0;
}

int APS5_VABI sceNpGetOnlineId(int user_id, NpOnlineId* online_id) {
    (void)user_id;
    if (!online_id) return SCE_NP_ERROR_INVALID_ARGUMENT;
    return SCE_NP_ERROR_SIGNED_OUT;
}

int APS5_VABI sceNpGetState(int user_id, uint32_t* state) {
    (void)user_id;
    if (!state) return SCE_NP_ERROR_INVALID_ARGUMENT;
    *state = NP_STATE_SIGNED_OUT;
    return 0;
}

int APS5_VABI sceNpHasSignedUp(int user_id, bool* has_signed_up) {
    (void)user_id;
    if (!has_signed_up) return SCE_NP_ERROR_INVALID_ARGUMENT;
    *has_signed_up = false;
    return 0;
}

int APS5_VABI sceNpPollAsync(int req_id, int* result) {
    (void)req_id;
    if (result) *result = SCE_NP_ERROR_SIGNED_OUT;
    return NP_POLL_ASYNC_FINISHED;
}

void APS5_VABI sceNpRegisterGamePresenceCallback(void* callback, void* userdata) {
 (void)callback;
 (void)userdata;
 NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceNpRegisterNpReachabilityStateCallback(void* callback, void* userdata) {
    if (!callback) return SCE_NP_ERROR_INVALID_ARGUMENT;
    std::lock_guard lock(reachabilityMutex);
    if (reachabilityCallback) return SCE_NP_ERROR_CALLBACK_ALREADY_REGISTERED;
    reachabilityCallback = callback;
    reachabilityUserdata = userdata;
    return 0;
}

int APS5_VABI sceNpUnregisterNpReachabilityStateCallback(void) {
    std::lock_guard lock(reachabilityMutex);
    if (!reachabilityCallback) return SCE_NP_ERROR_CALLBACK_NOT_REGISTERED;
    reachabilityCallback = nullptr;
    reachabilityUserdata = nullptr;
    return 0;
}

int APS5_VABI sceNpRegisterPlusEventCallback(void* callback, void* userdata) {
    (void)userdata;
    if (!callback) return SCE_NP_ERROR_INVALID_ARGUMENT;
    return 0;
}

int APS5_VABI sceNpRegisterPremiumEventCallback(void* callback, void* userdata) {
    (void)userdata;
    if (!callback) return SCE_NP_ERROR_INVALID_ARGUMENT;
    return 0;
}

int APS5_VABI sceNpRegisterStateCallback(NpStateCallback callback, void* userdata) {
    if (!callback) return SCE_NP_ERROR_INVALID_ARGUMENT;
    std::lock_guard lock(stateMutex);
    if (stateCallback.callback) return SCE_NP_ERROR_CALLBACK_ALREADY_REGISTERED;
    stateCallback = {callback, userdata, true};
    return 0;
}

int APS5_VABI sceNpSetContentRestriction(const NpContentRestriction* restriction) {
 (void)restriction;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceNpSetNpTitleId(const NpTitleId* title_id, const NpTitleSecret* title_secret) {
    (void)title_id;
    (void)title_secret;
    return 0;
}

int APS5_VABI sceNpSetTimeout(int reqId, int32_t resolveRetry, uint32_t resolveTimeout, uint32_t connTimeout,
    uint32_t sendTimeout, uint32_t recvTimeout) {
    constexpr uint32_t minResolveTimeout = 1000000;
    constexpr uint32_t minTransferTimeout = 10000000;
    auto belowMinimum = [](uint32_t timeout, uint32_t minimum) { return timeout != 0 && timeout < minimum; };
    if (reqId <= 0 || resolveRetry < 0) return SCE_NP_ERROR_INVALID_ARGUMENT;
    if (resolveRetry == 0 && resolveTimeout == 0 && connTimeout == 0 && sendTimeout == 0 && recvTimeout == 0) {
        return SCE_NP_ERROR_INVALID_ARGUMENT;
    }
    if (belowMinimum(resolveTimeout, minResolveTimeout) || belowMinimum(connTimeout, minTransferTimeout) ||
        belowMinimum(sendTimeout, minTransferTimeout) || belowMinimum(recvTimeout, minTransferTimeout)) {
        return SCE_NP_ERROR_INVALID_ARGUMENT;
    }
    std::lock_guard lock(requestMutex);
    return liveRequests.count(reqId) ? 0 : SCE_NP_ERROR_REQUEST_NOT_FOUND;
}

int APS5_VABI sceNpUnregisterStateCallback(void) {
    std::lock_guard lock(stateMutex);
    if (!stateCallback.callback) return SCE_NP_ERROR_CALLBACK_NOT_REGISTERED;
    stateCallback = {};
    return 0;
}

int APS5_VABI sceNpGetAccountLanguage2(int req_id, int user_id, void* language) {
    (void)req_id;
    (void)user_id;
    (void)language;
    return SCE_NP_ERROR_SIGNED_OUT;
}

int APS5_VABI sceNpNotifyPremiumFeature(const void* param) {
    (void)param;
    return 0;
}

int APS5_VABI sceNpRegisterStateCallbackA(NpStateCallbackA callback, void* userdata) {
    if (!callback) return SCE_NP_ERROR_INVALID_ARGUMENT;
    std::lock_guard lock(stateMutex);
    for (const auto& entry : stateCallbacksA) {
        if (entry.callback == callback) return SCE_NP_ERROR_CALLBACK_ALREADY_REGISTERED;
    }
    for (int index = 0; index < NP_STATE_CALLBACK_MAX; ++index) {
        if (stateCallbacksA[index].callback) continue;
        stateCallbacksA[index] = {callback, userdata, true};
        return index + 1;
    }
    return SCE_NP_ERROR_CALLBACK_MAX;
}

int APS5_VABI sceNpUnregisterStateCallbackA(int callbackId) {
    if (callbackId < 1 || callbackId > NP_STATE_CALLBACK_MAX) return SCE_NP_ERROR_INVALID_ARGUMENT;
    std::lock_guard lock(stateMutex);
    auto& entry = stateCallbacksA[callbackId - 1];
    if (!entry.callback) return SCE_NP_ERROR_CALLBACK_NOT_REGISTERED;
    entry = {};
    return 0;
}

int APS5_VABI sceNpUnregisterPremiumEventCallback(void) {
    return 0;
}

int APS5_VABI sceNpGetUserIdByAccountId(uint64_t account_id, int* user_id) {
    if (account_id == 0 || !user_id) return SCE_NP_ERROR_INVALID_ARGUMENT;
    return SCE_NP_ERROR_SIGNED_OUT;
}

int APS5_VABI sceNpGetSandboxInfo(void* sandbox_info) {
    (void)sandbox_info;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

APS5_EXPORT("8weiQyIdI3A", sceNpUnknown_8weiQyIdI3A);
int APS5_VABI sceNpUnknown_8weiQyIdI3A(int userId, void* buffer) {
    (void)userId;
    (void)buffer;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

APS5_EXPORT("WxLcN76q19o", sceNpUnknown_WxLcN76q19o);
int APS5_VABI sceNpUnknown_WxLcN76q19o(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceNpInGameMessageCreateHandle() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpInGameMessageDeleteHandle() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpInGameMessageGetMemoryPoolStatistics() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpInGameMessageInitialize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpInGameMessagePrepare() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpInGameMessagePrepareA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpInGameMessageSendData() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpInGameMessageSendDataA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpInGameMessageTerminate() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpIntGetAppType() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpIntGetGamePresenceStatus() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpIntGetNpTitleId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpIntGetNpTitleIdSecret() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpIntRegisterCheckCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpIntRegisterGamePresenceCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpIsPlusMember() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntAbortRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntAddActiveSigninStateCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntAddOnlineIdChangeCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntAddPlusMemberTypeCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntAddSigninStateCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntAuthGetAuthorizationCode() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntAuthGetIdToken() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntCheckNpAvailability() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntCheckNpAvailabilityByPid() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntCheckNpState() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntCheckNpStateA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntClearGameAccessToken() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntClearOnlineIdChangeFlag() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntClearTicket() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntClearUsedFlag() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntClearVshAccessToken() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntCreateAuthenticationTicket() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntCreateLoginContext() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntCreateLoginRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntCreateRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntDeleteLoginContext() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntDeleteRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetAccessTokenViaImplicitFlow() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetAccountAge() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetAccountCountry() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetAccountCountryA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetAccountCountrySdk() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetAccountDateOfBirthSdk() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetAccountId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetAccountIdSdk() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetAccountLanguage() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetAccountLanguageA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetAccountNpEnv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetAccountType() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetActiveSigninState() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetAuthServerErrorFlag() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetAuthorizationCode() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetAuthorizationCodeWithPsnoUri() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetClientCredentialAccessToken() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetCommunicationRestrictionStatus() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetGameAccessToken() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetIssuerId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetNpEnv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetNpId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetNpIdByOnlineId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetNpIdSdk() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetOfflineAccountId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetOnlineId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetOnlineIdByAccountId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetOnlineIdChangeFlag() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetOnlineIdSdk() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetParentalControlFlag() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetParentalControlInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetPlusMemberType() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetPlusMemberTypeNB() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetServerError() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetSigninState() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetSigninTelemetryInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetTicket() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetTicketA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetTitleToken() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetTitleTokenWithCheck() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetUserIdByAccountId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetUserIdByNpId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetUserIdByOnlineId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetUserIdByOnlineIdSdk() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetUserList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetUserNum() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetVshAccessToken() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI _sceNpIpcCreateMemoryFromKernel() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI _sceNpIpcCreateMemoryFromPool() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI _sceNpIpcDestroyMemory() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI _sceNpIpcFreeImpl() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI _sceNpIpcGetNpMemAllocator() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI _sceNpIpcMallocImpl() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI _sceNpIpcReallocImpl() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI _sceNpManagerCreateMemoryFromKernel() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI _sceNpManagerCreateMemoryFromPool() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI _sceNpManagerDestroyMemory() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI _sceNpManagerFreeImpl() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI _sceNpManagerGetNpMemAllocator() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI _sceNpManagerMallocImpl() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI _sceNpManagerReallocImpl() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmAbort() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientAbortRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientClearNpTitleToken() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientClearNpTitleTokenA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientCreateRequest2() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientCreateResourceContext() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientCreateResourceContext2() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientDeleteRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientDeleteResourceContext() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientDeleteResourceContext2() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetAppId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetCacheControlMaxAge() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetGameNpTitleInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetGameNpTitleToken() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetGameTitleBanInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetNpComInfo2() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetNpComInfo2A() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetNpComInfo3() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetNpComInfo4() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetNpTitleId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetNpTitleToken() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetNpTitleToken2() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetNpTitleTokenA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetRelatedGameNpTitleIds() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetRelatedGameNpTitleIdsA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetRelatedGameNpTitleIdsResult() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetServiceBaseUrl() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetServiceBaseUrlA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetServiceBaseUrlWithNpTitleId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetServiceBaseUrlWithNpTitleIdA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetServiceIdInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientGetServiceIdInfoA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientInitialize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientSetNpTitleId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmClientTerminate() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmCreateConnection() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmCreateRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmDeleteConnection() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmDeleteRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmGenerateNpTitleToken() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmGenerateNpTitleToken2() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmGetNpCommInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmGetNpCommInfo2() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmGetRelatedGameNpTitleIds() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmGetServiceBaseUrl() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmGetServiceIdInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmInitialize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpAsmTerminate() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpCheckCallbackForLib() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpCheckNpAvailabilityA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpCheckPlus() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpGetAccountCountry() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpGetAccountDateOfBirth() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpGetAccountDateOfBirthA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpGetAccountId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpGetAccountLanguage() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpGetAccountLanguageA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpGetGamePresenceStatus() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpGetGamePresenceStatusA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpGetParentalControlInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpGetParentalControlInfoA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpGetUserIdByOnlineId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpIdMapperAbortRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpIdMapperAccountIdToNpId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpIdMapperAccountIdToOnlineId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpIdMapperCreateRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpIdMapperDeleteRequest() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpIdMapperNpIdToAccountId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpIdMapperOnlineIdToAccountId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpInGameMessageAbortHandle() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpInGameMessageCheckCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetVshAccessTokenWithCheck() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntGetVshClientId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntIsSubAccount() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntIsTemporarySignout() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntIsUnregisteredClientError() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginAuthenticate() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginBind() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginCreateAuthenticationTicket() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginGetAccessToken() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginGetAccessTokenViaImplicitFlow() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginGetAccountId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginGetAuthenticateResponse() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginGetDeviceCodeInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginGetOnlineId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginGetUserId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginResetSsoToken() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginRevalidatePassword() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginSetUserId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginSignin() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginValidateKratosAuthCode() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntLoginVerifyDeviceCode() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntPfAuth() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntRemoveActiveSigninStateCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntRemoveOnlineIdChangeCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntRemovePlusMemberTypeCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntRemoveSigninStateCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntRevalidatePassword() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntSetPlusMemberTypeNB() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntSetRequestHeader() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntSetTimeout() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntSignout() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntSubmitUserCode() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntTemporarySignout() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntUpdateBcAccessToken() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerIntUpdateVshAccessToken() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerPrxStartVsh() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpManagerPrxStopVsh() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpNotifyPlusFeature(void* param) {
    (void)param;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2CreateUserContext() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2DeleteUserContext() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2Init() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2IsInit() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2OptionalCheckCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2RegisterDataType() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2RegisterExtendedDataFilter() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2RegisterNotificationExCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2SendPushStatisticsDataSystemTelemetry() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2SetGlobalMutex() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2SetNpCommunicationId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2Term() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2TriggerEmptyUserEvent() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2UnregisterDataType() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2UnregisterExtendedDataFilter() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2UnregisterNotificationExCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2UnsetNpCommunicationId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPush2WaitCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushCheckCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushInit() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushIntBeginInactive() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushIntEndInactive() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushIntGetBindUserState() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushIntGetConnectionState() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushIntRegisterNotificationPacketCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushIntUnregisterNotificationPacketCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushRegisterExtendedDataFilter() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushRegisterNotificationExCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushStartNotification() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushStartNotificationA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushStopNotification() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushStopNotificationA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushTerm() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushUnregisterExtendedDataFilter() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpPushUnregisterNotificationCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpRegisterGamePresenceCallbackA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpRegisterStateCallbackForToolkit() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceClientInit() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpServiceClientTerm() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpSetAdditionalScope() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpSetGamePresenceOnline(int req_id, const NpOnlineId* online_id, uint32_t presence) {
    (void)req_id;
    (void)online_id;
    (void)presence;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpSetGamePresenceOnlineA(int req_id, int user_id, uint32_t presence) {
    (void)req_id;
    (void)user_id;
    (void)presence;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpSetNpTitleIdVsh() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpUnregisterGamePresenceCallbackA(int callback_id) {
    (void)callback_id;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpUnregisterPlusEventCallback() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpUnregisterStateCallbackForToolkit() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceNpWaitAsync(int req_id, int* result) {
    (void)req_id;
    (void)result;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
}
