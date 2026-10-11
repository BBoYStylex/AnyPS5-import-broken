#include "SceTypes.hpp"
#include <cstdint>
#include <cstdlib>

extern "C" {
int APS5_VABI sceNpAuthCreateRequest();
int APS5_VABI sceNpAuthDeleteRequest(int reqId);
int APS5_VABI sceNpAuthGetIdTokenV3(int reqId, const void* param, void* idToken);
int APS5_VABI sceNpAuthPollAsync(int reqId, int* result);
int APS5_VABI sceNpAuthWaitAsync(int reqId, int* result);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

constexpr int kInvalidArgument = static_cast<int>(0x80550003);
constexpr int kSignedOut = static_cast<int>(0x80550006);
constexpr int kPollFinished = 0;

}

int main() {
    const int request = sceNpAuthCreateRequest();
    const int other = sceNpAuthCreateRequest();
    Require(request > 0 && other > 0 && request != other);

    std::uint64_t param = 0;
    std::uint64_t token = 0;
    Require(sceNpAuthGetIdTokenV3(request, nullptr, &token) == kInvalidArgument);
    Require(sceNpAuthGetIdTokenV3(request, &param, nullptr) == kInvalidArgument);
    Require(sceNpAuthGetIdTokenV3(request, &param, &token) == kSignedOut);

    int result = 0x5a5a5a5a;
    Require(sceNpAuthPollAsync(request, nullptr) == kPollFinished);
    Require(sceNpAuthPollAsync(request, &result) == kPollFinished);
    Require(result == kSignedOut);
    result = 0x5a5a5a5a;
    Require(sceNpAuthWaitAsync(request, nullptr) == kPollFinished);
    Require(sceNpAuthWaitAsync(request, &result) == kPollFinished);
    Require(result == kSignedOut);

    Require(sceNpAuthDeleteRequest(request) == 0);
    Require(sceNpAuthDeleteRequest(other) == 0);
    return 0;
}
