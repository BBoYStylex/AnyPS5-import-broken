#include "prx/libc/include/general/VabiMacros.hpp"
#include "SceTypes.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
int APS5_VABI sceNpGetNpId(int user_id, NpId* np_id);
int APS5_VABI sceNpSetContentRestriction(const NpContentRestriction* restriction);
void APS5_VABI sceNpRegisterGamePresenceCallback(void* callback, void* userdata);
}

extern "C" {
int APS5_VABI sceNpCreateRequest(void);
int APS5_VABI sceNpCreateAsyncRequest(const NpCreateAsyncRequestParameter* param);
int APS5_VABI sceNpDeleteRequest(int reqId);
int APS5_VABI sceNpSetTimeout(int reqId, std::int32_t resolveRetry, std::uint32_t resolveTimeout,
    std::uint32_t connTimeout, std::uint32_t sendTimeout, std::uint32_t recvTimeout);
}

namespace {

constexpr int InvalidArgument = static_cast<int>(0x80550003u);
constexpr int SignedOut = static_cast<int>(0x80550006u);
constexpr int InvalidSize = static_cast<int>(0x80550011u);
constexpr int RequestNotFound = static_cast<int>(0x80550014u);

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "NpManager: %s\n", message);
        std::abort();
    }
}

void CheckRequestTimeouts() {
    Require(sceNpSetTimeout(0x7fffffff, 2, 0, 0, 0, 0) == RequestNotFound, "never created request");
    Require(sceNpSetTimeout(0x7fffffff, 0, 0, 0, 0, 0) == InvalidArgument, "arguments checked before the request");
    const int requestId = sceNpCreateRequest();
    Require(requestId > 0, "sceNpCreateRequest must return a positive request id");
    Require(sceNpSetTimeout(requestId, 2, 1000000, 10000000, 10000000, 10000000) == 0, "minimum timeouts");
    Require(sceNpSetTimeout(requestId, 0, 0, 0, 0, 30000000) == 0, "only the receive timeout");
    Require(sceNpSetTimeout(requestId, 3, 0, 0, 0, 0) == 0, "only the resolve retries");
    Require(sceNpSetTimeout(requestId, 0, 0, 0, 0, 0) == InvalidArgument, "every value left at its default");
    Require(sceNpSetTimeout(0, 2, 0, 0, 0, 0) == InvalidArgument, "request id 0");
    Require(sceNpSetTimeout(-1, 2, 0, 0, 0, 0) == InvalidArgument, "negative request id");
    Require(sceNpSetTimeout(requestId, -1, 0, 0, 0, 30000000) == InvalidArgument, "negative resolve retries");
    Require(sceNpSetTimeout(requestId, 0, 999999, 0, 0, 0) == InvalidArgument, "resolve timeout below 1 s");
    Require(sceNpSetTimeout(requestId, 0, 0, 9999999, 0, 0) == InvalidArgument, "connect timeout below 10 s");
    Require(sceNpSetTimeout(requestId, 0, 0, 0, 9999999, 0) == InvalidArgument, "send timeout below 10 s");
    Require(sceNpSetTimeout(requestId, 0, 0, 0, 0, 1) == InvalidArgument, "receive timeout below 10 s");
    Require(sceNpDeleteRequest(requestId) == 0, "sceNpDeleteRequest failed");
    Require(sceNpSetTimeout(requestId, 2, 0, 0, 0, 0) == RequestNotFound, "deleted request");
    NpCreateAsyncRequestParameter param{};
    const int asyncId = sceNpCreateAsyncRequest(&param);
    Require(asyncId > 0, "sceNpCreateAsyncRequest must return a positive request id");
    Require(sceNpSetTimeout(asyncId, 2, 0, 0, 0, 0) == 0, "async request");
    Require(sceNpDeleteRequest(asyncId) == 0, "sceNpDeleteRequest failed on the async request");
    Require(sceNpSetTimeout(asyncId, 2, 0, 0, 0, 0) == RequestNotFound, "deleted async request");
}

void CheckContentRestriction() {
    NpAgeRestriction ages[2]{};
    NpContentRestriction restriction{};
    restriction.size = sizeof(restriction);
    restriction.default_age_restriction = 12;
    Require(sceNpSetContentRestriction(&restriction) == 0, "a default age only");
    restriction.age_restriction_count = 2;
    restriction.age_restriction = ages;
    Require(sceNpSetContentRestriction(&restriction) == 0, "per country ages");
    restriction.age_restriction = nullptr;
    Require(sceNpSetContentRestriction(&restriction) == InvalidArgument, "a missing age list");
    restriction.age_restriction_count = 0x101;
    restriction.age_restriction = ages;
    Require(sceNpSetContentRestriction(&restriction) == InvalidArgument, "more than 256 ages");
    restriction.age_restriction_count = -1;
    Require(sceNpSetContentRestriction(&restriction) == InvalidArgument, "a negative count");
    restriction.age_restriction_count = 0;
    restriction.default_age_restriction = -1;
    Require(sceNpSetContentRestriction(&restriction) == InvalidArgument, "a negative default age");
    restriction.default_age_restriction = 0;
    restriction.size = sizeof(restriction) - 1;
    Require(sceNpSetContentRestriction(&restriction) == InvalidSize, "a wrong size");
    Require(sceNpSetContentRestriction(nullptr) == InvalidArgument, "a null structure");
}

}

int main() {
    CheckContentRestriction();
    sceNpRegisterGamePresenceCallback(reinterpret_cast<void*>(1), nullptr);
    sceNpRegisterGamePresenceCallback(nullptr, nullptr);
    CheckRequestTimeouts();
    NpId npId{};
    std::memset(&npId, 0x5a, sizeof(npId));
    NpId untouched{};
    std::memset(&untouched, 0x5a, sizeof(untouched));
    Require(sceNpGetNpId(0x10000, &npId) == SignedOut, "sceNpGetNpId must report the user as signed out");
    Require(std::memcmp(&npId, &untouched, sizeof(npId)) == 0, "sceNpGetNpId must leave the NpId untouched");
    Require(sceNpGetNpId(0x10000, nullptr) == InvalidArgument, "sceNpGetNpId must reject a null NpId");
    return 0;
}
