#include "SceTypes.hpp"
#include <cstdint>
#include <cstdlib>

extern "C" {
[[noreturn]] void NotImplemented_nid_no_patch(const char*) {
    std::abort();
}
int APS5_VABI sceNpTusCreateNpTitleCtxA(std::uint32_t, int);
int APS5_VABI sceNpTusCreateRequest(int);
int APS5_VABI sceNpTusDeleteNpTitleCtx(int);
int APS5_VABI sceNpTusGetData(int, const void*, std::int32_t, void*, std::uint64_t, void*, std::uint64_t, void*);
int APS5_VABI sceNpTusGetDataVUser(int, const NpOnlineId*, std::int32_t, void*, std::uint64_t, void*, std::uint64_t, void*);
int APS5_VABI sceNpTusGetMultiSlotVariable(int, const void*, const std::int32_t*, void*, std::uint64_t, int, void*);
int APS5_VABI sceNpTusGetMultiSlotVariableVUser(int, const NpOnlineId*, const std::int32_t*, void*, std::uint64_t, int, void*);
int APS5_VABI sceNpTusSetMultiSlotVariable(int, const void*, const std::int32_t*, const std::int64_t*, int, void*);
int APS5_VABI sceNpTusSetMultiSlotVariableVUser(int, const NpOnlineId*, const std::int32_t*, const std::int64_t*, int, void*);
int APS5_VABI sceNpTusSetData(int, const void*, std::int32_t, std::uint64_t, std::uint64_t, const void*, const void*, std::uint64_t, const void*, const void*, void*);
int APS5_VABI sceNpTusSetDataVUser(int, const NpOnlineId*, std::int32_t, std::uint64_t, std::uint64_t, const void*, const void*, std::uint64_t, const void*, const void*, void*);
}

namespace {
constexpr int SignedOut = static_cast<int>(0x80550006u);
constexpr int InvalidArgument = static_cast<int>(0x80550704u);
constexpr int InsufficientArgument = static_cast<int>(0x8055070Cu);

void Require(bool value) {
    if (!value) std::abort();
}
}

int main() {
    const int context = sceNpTusCreateNpTitleCtxA(1, 1);
    Require(context > 0);
    const int request = sceNpTusCreateRequest(context);
    Require(request > 0);
    unsigned char npId[64]{};
    NpOnlineId user{{'u', 's', 'e', 'r'}, {}};
    unsigned char status[128];
    unsigned char data[16];
    for (unsigned char& byte : status) byte = 0x5a;
    for (unsigned char& byte : data) byte = 0x5a;

    Require(sceNpTusGetData(request, npId, 1, nullptr, sizeof(status), data, sizeof(data), nullptr) == InsufficientArgument);
    Require(sceNpTusGetData(request, npId, -1, status, sizeof(status), data, sizeof(data), nullptr) == InvalidArgument);
    Require(sceNpTusGetData(request, npId, 1, status, sizeof(status), data, sizeof(data), nullptr) == SignedOut);
    Require(sceNpTusGetDataVUser(request, nullptr, 1, status, sizeof(status), data, sizeof(data), nullptr) == InsufficientArgument);
    Require(sceNpTusGetDataVUser(request, &user, 1, status, sizeof(status), data, sizeof(data), nullptr) == SignedOut);
    std::int32_t slots[2] = {1, 2};
    std::int64_t values[2] = {3, 4};
    Require(sceNpTusGetMultiSlotVariable(request, npId, nullptr, status, sizeof(status), 2, nullptr) == InsufficientArgument);
    Require(sceNpTusGetMultiSlotVariable(request, npId, slots, status, sizeof(status), 2, nullptr) == SignedOut);
    Require(sceNpTusGetMultiSlotVariableVUser(request, nullptr, slots, status, sizeof(status), 2, nullptr) == InsufficientArgument);
    Require(sceNpTusGetMultiSlotVariableVUser(request, &user, slots, status, sizeof(status), 2, nullptr) == SignedOut);
    Require(sceNpTusSetMultiSlotVariable(request, npId, slots, nullptr, 2, nullptr) == InsufficientArgument);
    Require(sceNpTusSetMultiSlotVariable(request, npId, slots, values, 2, nullptr) == SignedOut);
    Require(sceNpTusSetMultiSlotVariableVUser(request, nullptr, slots, values, 2, nullptr) == InsufficientArgument);
    Require(sceNpTusSetMultiSlotVariableVUser(request, &user, slots, values, 2, nullptr) == SignedOut);
    Require(sceNpTusSetData(request, npId, 1, sizeof(data), 0, data, nullptr, 0, nullptr, nullptr, nullptr) == InsufficientArgument);
    Require(sceNpTusSetData(request, npId, 1, sizeof(data), sizeof(data), data, nullptr, 0, nullptr, nullptr, nullptr) == SignedOut);
    Require(sceNpTusSetDataVUser(request, nullptr, 1, sizeof(data), sizeof(data), data, nullptr, 0, nullptr, nullptr, nullptr) == InsufficientArgument);
    Require(sceNpTusSetDataVUser(request, &user, 1, sizeof(data), sizeof(data), data, nullptr, 0, nullptr, nullptr, nullptr) == SignedOut);
    for (const unsigned char byte : status) Require(byte == 0x5a);
    for (const unsigned char byte : data) Require(byte == 0x5a);
    Require(sceNpTusDeleteNpTitleCtx(context) == 0);
}
