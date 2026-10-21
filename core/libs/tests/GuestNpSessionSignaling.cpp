#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
int APS5_VABI sceNpSessionSignalingInitialize(void* param);
int APS5_VABI sceNpSessionSignalingCreateContext2(const void* param, std::uint32_t* contextId);
int APS5_VABI sceNpSessionSignalingCreateContext(const void* param, std::uint32_t* contextId);
int APS5_VABI sceNpSessionSignalingDeactivate(std::uint32_t contextId);
int APS5_VABI sceNpSessionSignalingDestroyContext(std::uint32_t contextId);
int APS5_VABI sceNpSessionSignalingActivateSession(void);
int APS5_VABI sceNpSessionSignalingActivateUser(void);
int APS5_VABI sceNpSessionSignalingGetConnectionInfo(void);
int APS5_VABI sceNpSessionSignalingGetConnectionFromPeerAddress2(void);
std::int32_t APS5_VABI sceNpSessionSignalingGetLocalNetInfo(std::int32_t contextId, void* info);
int APS5_VABI sceNpSessionSignalingRequestPrepare(std::uint32_t contextId, std::uint32_t* requestId);
std::int32_t APS5_VABI sceNpSessionSignalingGetConnectionStatus(std::int32_t contextId, std::int32_t connectionId, std::int32_t* status, void* peerAddress, std::uint16_t* peerPort);
int APS5_VABI sceNpSessionSignalingTerminate(void);
}

namespace {

constexpr int InvalidArgument = static_cast<int>(0x80553303u);
constexpr int Unavailable = static_cast<int>(0x80552D06u);
constexpr int ConnectionNotFound = static_cast<int>(0x8055330Cu);

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "NpSessionSignaling: %s\n", message);
        std::abort();
    }
}

}

int main() {
    unsigned char initializeParam[32] = {};
    Require(sceNpSessionSignalingInitialize(initializeParam) == 0, "initialize failed");
    unsigned char contextParam[64] = {};
    std::uint32_t contextId = 0;
    Require(sceNpSessionSignalingCreateContext2(contextParam, &contextId) == 0, "context creation failed");

    std::uint32_t requestId = 0xa5a5a5a5u;
    Require(sceNpSessionSignalingRequestPrepare(contextId, nullptr) == InvalidArgument, "null request id");
    Require(sceNpSessionSignalingRequestPrepare(contextId, &requestId) == Unavailable, "prepare without network");
    Require(requestId == 0xa5a5a5a5u, "failed prepare wrote a request id");

    const auto netContextId = static_cast<std::int32_t>(contextId);
    unsigned char info[16];
    std::memset(info, 0xa5, sizeof(info));
    unsigned char untouched[sizeof(info)];
    std::memcpy(untouched, info, sizeof(info));
    Require(sceNpSessionSignalingGetLocalNetInfo(netContextId, nullptr) == InvalidArgument, "null info");
    Require(sceNpSessionSignalingGetLocalNetInfo(netContextId, info) == Unavailable, "local net info without network");
    Require(std::memcmp(info, untouched, sizeof(info)) == 0, "failed query modified the info");

    std::int32_t status = 0x5a5a5a5a;
    std::uint32_t peerAddress = 0xa5a5a5a5u;
    std::uint16_t peerPort = 0xa5a5;
    Require(sceNpSessionSignalingGetConnectionStatus(netContextId, 1, nullptr, &peerAddress, &peerPort) == InvalidArgument, "null connection status");
    Require(sceNpSessionSignalingGetConnectionStatus(netContextId, 1, &status, &peerAddress, &peerPort) == ConnectionNotFound, "connection without network");
    Require(sceNpSessionSignalingGetConnectionStatus(netContextId, 1, &status, nullptr, nullptr) == ConnectionNotFound, "connection without peer outputs");
    Require(status == 0x5a5a5a5a && peerAddress == 0xa5a5a5a5u && peerPort == 0xa5a5, "failed status query wrote an output");

        std::uint32_t legacyContextId = 0;
    Require(sceNpSessionSignalingCreateContext(nullptr, nullptr) == InvalidArgument, "null legacy context id");
    Require(sceNpSessionSignalingCreateContext(nullptr, &legacyContextId) == 0, "legacy context creation");
    Require(legacyContextId > 0, "valid legacy context id");
    Require(sceNpSessionSignalingDeactivate(legacyContextId) == 0, "deactivate legacy context");
    Require(sceNpSessionSignalingDestroyContext(legacyContextId) == 0, "destroy legacy context");

    Require(sceNpSessionSignalingActivateSession() == Unavailable, "activate session without network");
    Require(sceNpSessionSignalingActivateUser() == Unavailable, "activate user without network");
    Require(sceNpSessionSignalingGetConnectionInfo() == Unavailable, "connection info without network");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress2() == Unavailable, "peer address query without network");

    Require(sceNpSessionSignalingTerminate() == 0, "terminate failed");
}
