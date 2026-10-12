#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <xmmintrin.h>
#ifdef _WIN32
#include <windows.h>
#include "prx/libc/include/GuestArena.hpp"
#else
#include <fstream>
#include <sstream>
#include <string>
#endif

using Entry = void (APS5_VABI*)(std::uint64_t, std::uint64_t);

extern "C" {
std::int32_t APS5_VABI _sceFiberInitializeImpl_nid_postfix(FiberObject*, const char*, Entry, std::uint64_t, void*, std::uint64_t, const void*, std::uint32_t);
std::int32_t APS5_VABI sceFiberFinalize(FiberObject*);
std::int32_t APS5_VABI sceFiberRun_nid_postfix(FiberObject*, std::uint64_t, std::uint64_t*);
std::int32_t APS5_VABI sceFiberSwitch(FiberObject*, std::uint64_t, std::uint64_t*);
std::int32_t APS5_VABI sceFiberReturnToThread(std::uint64_t, std::uint64_t*);
std::int32_t APS5_VABI sceFiberGetSelf(FiberObject**);
std::int32_t APS5_VABI sceFiberGetInfo(FiberObject*, FiberInfo*);
std::int32_t APS5_VABI sceFiberGetThreadFramePointerAddress(std::uint64_t*);
std::int32_t APS5_VABI _sceFiberGetThreadFramePointerAddress(std::uint64_t*);
std::int32_t APS5_VABI sceFiberStartContextSizeCheck(std::uint32_t);
std::int32_t APS5_VABI sceFiberStopContextSizeCheck(void);
int APS5_VABI sceKernelMapFlexibleMemory(void**, std::size_t, int, int);
int APS5_VABI sceKernelAllocateDirectMemory(std::int64_t, std::int64_t, std::size_t, std::size_t, int, std::int64_t*);
int APS5_VABI sceKernelMapDirectMemory(void**, std::size_t, int, int, std::int64_t, std::size_t);
int APS5_VABI sceKernelReleaseDirectMemory(std::int64_t, std::size_t);
int APS5_VABI sceKernelMprotect(const void*, std::size_t, int);
int APS5_VABI sceKernelMunmap(void*, std::size_t);
}

static void Require(bool value, const char* what) {
    if (!value) { std::fprintf(stderr, "Fiber check failed: %s\n", what); std::abort(); }
}

namespace {

constexpr std::int32_t FiberErrorNull = static_cast<std::int32_t>(0x80590001);
constexpr std::int32_t FiberErrorInvalid = static_cast<std::int32_t>(0x80590004);
constexpr std::int32_t FiberErrorPermission = static_cast<std::int32_t>(0x80590005);
constexpr std::int32_t FiberErrorState = static_cast<std::int32_t>(0x80590006);

alignas(16) std::array<unsigned char, 64 * 1024> g_firstContext;
alignas(16) std::array<unsigned char, 64 * 1024> g_secondContext;
struct alignas(8) FiberStorage {
    unsigned char bytes[0x100];
};

FiberStorage g_firstStorage;
FiberStorage g_checkedStorage;
alignas(16) std::array<unsigned char, 16 * 1024> g_checkedContext;
std::uint64_t g_threadFramePointer = 0;
FiberStorage g_secondStorage;
auto* const g_first = reinterpret_cast<FiberObject*>(&g_firstStorage);
auto* const g_second = reinterpret_cast<FiberObject*>(&g_secondStorage);

void APS5_VABI FirstEntry(std::uint64_t argOnInitialize, std::uint64_t argOnRun) {
    Require(argOnInitialize == 11 && argOnRun == 100, "first fiber arguments");
    FiberObject* self = nullptr;
    Require(sceFiberGetSelf(&self) == 0 && self == g_first, "first fiber self");
    std::uint64_t framePointer = 0;
    Require(sceFiberGetThreadFramePointerAddress(nullptr) == FiberErrorNull, "frame pointer null");
    Require(sceFiberGetThreadFramePointerAddress(&framePointer) == 0 && framePointer == g_threadFramePointer, "thread frame pointer");
    framePointer = 0;
    Require(_sceFiberGetThreadFramePointerAddress(nullptr) == FiberErrorNull, "underscore frame pointer null");
    Require(_sceFiberGetThreadFramePointerAddress(&framePointer) == 0 && framePointer == g_threadFramePointer, "underscore thread frame pointer");
    volatile double carried = 1.5;
    std::uint64_t received = 0;
    Require(sceFiberSwitch(g_second, 200, &received) == 0, "switch to second fiber");
    Require(received == 300 && carried == 1.5, "switch back to first fiber");
    for (;;) {
        Require(sceFiberReturnToThread(received + 1, &received) == 0, "first fiber return");
    }
}

void APS5_VABI SecondEntry(std::uint64_t argOnInitialize, std::uint64_t argOnRun) {
    Require(argOnInitialize == 22 && argOnRun == 200, "second fiber arguments");
    Require(sceFiberSwitch(g_second, 0, nullptr) == FiberErrorState, "switch to self");
    _mm_setcsr(_mm_getcsr() | 0x8000u);
    std::uint64_t received = 0;
    Require(sceFiberReturnToThread(250, &received) == 0 && received == 260, "second fiber resumed on another thread");
    Require((_mm_getcsr() & 0x8000u) != 0, "second fiber keeps its MXCSR");
    Require(sceFiberSwitch(g_first, 300, nullptr) == 0, "switch to first fiber");
    Require(false, "second fiber resumed after its last switch");
}

void APS5_VABI CheckedEntry(std::uint64_t, std::uint64_t) {
    volatile unsigned char used[4096];
    for (auto& byte : used) byte = 1;
    std::uint64_t received = 0;
    for (;;) sceFiberReturnToThread(0, &received);
}

constexpr std::size_t SmallContextSize = 2048;
constexpr unsigned char SmallGuardFill = 0xa5;
alignas(16) std::array<unsigned char, 64 * 1024> g_smallArea;
FiberStorage g_smallStorage;

void APS5_VABI SmallEntry(std::uint64_t, std::uint64_t) {
    std::uint64_t received = 0;
    for (;;) sceFiberReturnToThread(0, &received);
}

void RunSmallContextFirst() {
    auto* small = reinterpret_cast<FiberObject*>(&g_smallStorage);
    auto* context = g_smallArea.data() + g_smallArea.size() - SmallContextSize;
    g_smallArea.fill(SmallGuardFill);
    Require(sceFiberReturnToThread(0, nullptr) == FiberErrorPermission, "return to thread outside a fiber");
    Require(_sceFiberInitializeImpl_nid_postfix(small, "small", SmallEntry, 0, context, SmallContextSize, nullptr, 0) == 0, "initialize small");
    Require(sceFiberRun_nid_postfix(small, 0, nullptr) == 0, "first fiber run of the process on a 2 KiB context");
    Require(sceFiberRun_nid_postfix(small, 0, nullptr) == 0, "resume on a 2 KiB context");
    Require(sceFiberFinalize(small) == 0, "finalize small");
    for (auto* byte = g_smallArea.data(); byte != context; ++byte) {
        Require(*byte == SmallGuardFill, "nothing is written below a 2 KiB context");
    }
}

constexpr std::size_t GuardBytes = 0x1000;
constexpr std::size_t SharedPageBytes = 0x4000;
alignas(GuardBytes) std::array<unsigned char, 64 * 1024> g_guardArea;
FiberStorage g_guardStorage;
FiberStorage g_otherGuardStorage;
auto* const g_guarded = reinterpret_cast<FiberObject*>(&g_guardStorage);
auto* const g_otherGuarded = reinterpret_cast<FiberObject*>(&g_otherGuardStorage);

#ifdef _WIN32
DWORD Protection(const void* address) {
    MEMORY_BASIC_INFORMATION info{};
    Require(VirtualQuery(address, &info, sizeof(info)) == sizeof(info), "query page protection");
    return info.Protect;
}

bool Readable(const void* address) {
    const auto protection = Protection(address);
    return protection == PAGE_READONLY || protection == PAGE_READWRITE;
}

bool Writable(const void* address) {
    return Protection(address) == PAGE_READWRITE;
}
#else
std::string Permissions(const void* address) {
    const auto wanted = reinterpret_cast<std::uintptr_t>(address);
    std::ifstream maps("/proc/self/maps");
    std::string line;
    while (std::getline(maps, line)) {
        std::istringstream fields(line);
        std::uintptr_t begin = 0, end = 0;
        char dash = 0;
        std::string permissions;
        if ((fields >> std::hex >> begin >> dash >> end >> permissions) && wanted >= begin && wanted < end) return permissions;
    }
    Require(false, "page is mapped");
    return {};
}

bool Writable(const void* address) {
    const auto permissions = Permissions(address);
    return permissions[0] == 'r' && permissions[1] == 'w';
}
#endif

void APS5_VABI GuardedEntry(std::uint64_t argOnInitialize, std::uint64_t argOnRun) {
    volatile unsigned char used[8192];
    for (auto& byte : used) byte = static_cast<unsigned char>(argOnInitialize);
    std::uint64_t received = argOnRun;
    for (;;) sceFiberReturnToThread(received + used[0], &received);
}

void RunGuarded(std::uint64_t argOnRun, const char* what, FiberObject* fiber = g_guarded) {
    std::uint64_t returned = 0;
    Require(sceFiberRun_nid_postfix(fiber, argOnRun, &returned) == 0 && returned == argOnRun + 1, what);
}

void CheckNoGuardByDefault() {
    g_guardArea.fill(0);
    auto* context = g_guardArea.data();
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "unguarded", GuardedEntry, 1, context, g_guardArea.size(), nullptr, 0) == 0, "initialize without APS5_FIBER_GUARD");
    Require(Writable(context), "no guard page without APS5_FIBER_GUARD");
    RunGuarded(10, "run without APS5_FIBER_GUARD");
    Require(sceFiberFinalize(g_guarded) == 0, "finalize without APS5_FIBER_GUARD");
}

#ifdef _WIN32
void CheckGuardedHostContext() {
    g_guardArea.fill(0);
    auto* context = g_guardArea.data();
    Require(Writable(context), "lowest page writable before initialize");
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "guarded", GuardedEntry, 1, context, g_guardArea.size(), nullptr, 0) == 0, "initialize guarded");
    Require(!Readable(context) && !Readable(context + GuardBytes - 1), "lowest page unreadable after initialize");
    Require(Writable(context + GuardBytes), "the page above the guard stays writable");
    RunGuarded(10, "guarded fiber runs");
    RunGuarded(20, "guarded fiber resumes");
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "reinitialized", GuardedEntry, 1, context, g_guardArea.size(), nullptr, 0) == 0, "reinitialize guarded");
    Require(!Readable(context), "lowest page unreadable after reinitialize");
    RunGuarded(30, "reinitialized fiber runs");
    Require(sceFiberFinalize(g_guarded) == 0, "finalize guarded");
    Require(Writable(context), "lowest page writable after finalize");
    context[0] = 1;
    context[GuardBytes - 1] = 2;
    Require(context[0] == 1 && context[GuardBytes - 1] == 2, "lowest page usable after finalize");
}

void CheckUnguardableContexts() {
    auto* unaligned = g_guardArea.data() + 16;
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "unaligned", GuardedEntry, 1, unaligned, g_guardArea.size() - GuardBytes, nullptr, 0) == 0, "initialize unaligned");
    Require(Writable(unaligned), "a context that does not start on a page gets no guard");
    RunGuarded(40, "unaligned fiber runs");
    Require(sceFiberFinalize(g_guarded) == 0, "finalize unaligned");
    auto* small = g_guardArea.data();
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "small", SmallEntry, 0, small, 2 * GuardBytes - 16, nullptr, 0) == 0, "initialize small");
    Require(Writable(small), "a context smaller than two pages gets no guard");
    Require(sceFiberRun_nid_postfix(g_guarded, 0, nullptr) == 0, "small fiber runs");
    Require(sceFiberFinalize(g_guarded) == 0, "finalize small");
}

void CheckGuardedSizeCheck() {
    auto* context = g_guardArea.data();
    FiberInfo info{};
    info.size = sizeof(info);
    Require(sceFiberStartContextSizeCheck(0) == 0, "start size check");
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "checked", CheckedEntry, 0, context, g_guardArea.size(), nullptr, 0) == 0, "initialize checked");
    Require(!Readable(context), "checked context is guarded");
    Require(sceFiberGetInfo(g_guarded, &info) == 0 && info.size_context_margin == g_guardArea.size(), "untouched guarded context is all margin");
    Require(sceFiberRun_nid_postfix(g_guarded, 0, nullptr) == 0, "run checked");
    Require(sceFiberGetInfo(g_guarded, &info) == 0, "guarded checked info");
    Require(info.size_context_margin > GuardBytes && info.size_context_margin < g_guardArea.size() - 4096, "margin under the used stack includes the guard page");
    Require(sceFiberFinalize(g_guarded) == 0, "finalize checked");
    Require(sceFiberStopContextSizeCheck() == 0, "stop size check");
}

std::size_t Collect(const void* address, std::size_t bytes, void** written = nullptr) {
    std::array<void*, 64> pages{};
    std::size_t count = pages.size();
    Require(GuestArena::GuestArenaCollectWrites_nid_postfix(reinterpret_cast<std::uintptr_t>(address), bytes, pages.data(), &count, true), "collect guest writes");
    if (written) std::memcpy(written, pages.data(), count * sizeof(void*));
    return count;
}

void CheckGuardedFlexibleContext() {
    constexpr std::size_t bytes = 0x10000;
    void* mapped = nullptr;
    Require(sceKernelMapFlexibleMemory(&mapped, bytes, 3, 0) == 0, "map flexible memory");
    auto* context = static_cast<unsigned char*>(mapped);
    Collect(context, bytes);
    Require(Collect(context, bytes) == 0, "flexible memory collected clean");
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "flexible", GuardedEntry, 1, context, bytes, nullptr, 0) == 0, "initialize on flexible memory");
    Require(!Readable(context), "flexible context is guarded");
    Require(Collect(context, bytes) == 0, "the guard page is not collected as a write");
    RunGuarded(50, "fiber on flexible memory runs");
    std::array<void*, 64> written{};
    const auto count = Collect(context, bytes, written.data());
    Require(count != 0, "the fiber stack writes are collected");
    for (std::size_t i = 0; i < count; ++i) Require(written[i] >= context + GuardBytes, "no write collected in the guard page");
    Require(sceKernelMprotect(context, bytes, 1) == 0, "guest read-only protection under the guard");
    Require(!Readable(context) && Readable(context + GuardBytes) && !Writable(context + GuardBytes), "the guard survives a guest protection change");
    Require(sceKernelMprotect(context, bytes, 3) == 0, "guest read-write protection under the guard");
    Require(!Readable(context) && Writable(context + GuardBytes), "the guard survives a second guest protection change");
    RunGuarded(60, "fiber on flexible memory resumes");
    Require(sceKernelMprotect(context, bytes, 1) == 0, "guest read-only protection before finalize");
    Require(sceFiberFinalize(g_guarded) == 0, "finalize on flexible memory");
    Require(Readable(context) && !Writable(context), "finalize restores the guest protection set under the guard");
    Require(sceKernelMprotect(context, bytes, 3) == 0, "guest read-write protection after finalize");
    Collect(context, bytes);
    Require(Collect(context, bytes) == 0, "removing the guard is not a write");
    context[8] = 1;
    Require(Collect(context, bytes) == 1, "a write to the former guard page is collected");
    Require(sceKernelMunmap(context, bytes) == 0, "unmap flexible memory");
}

void CheckGuardedSharedContext() {
    constexpr std::size_t bytes = SharedPageBytes * 4;
    std::int64_t phys = 0;
    Require(sceKernelAllocateDirectMemory(0, 0x7fffffffffll, bytes, 0, 0, &phys) == 0, "allocate direct memory");
    void* mapped = nullptr;
    void* aliased = nullptr;
    Require(sceKernelMapDirectMemory(&mapped, bytes, 3, 0, phys, 0) == 0, "map direct memory");
    Require(sceKernelMapDirectMemory(&aliased, bytes, 3, 0, phys, 0) == 0, "map a second view of direct memory");
    auto* context = static_cast<volatile unsigned char*>(mapped);
    auto* alias = static_cast<volatile unsigned char*>(aliased);
    auto* guard = static_cast<unsigned char*>(mapped);
    Collect(mapped, bytes);
    Collect(aliased, bytes);
    Require(Collect(mapped, bytes) == 0 && Collect(aliased, bytes) == 0, "shared views collected clean");
    Require(Protection(guard) == PAGE_READONLY, "shared view armed before initialize");
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "shared", GuardedEntry, 1, guard, bytes, nullptr, 0) == 0, "initialize on a shared view");
    Require(Protection(guard) == PAGE_NOACCESS, "shared context is guarded");
    Require(Protection(guard + GuardBytes) == PAGE_READWRITE && Protection(guard + SharedPageBytes) == PAGE_READWRITE, "the rest of the pinned view is writable");
    Require(!GuestArena::GuestArenaHandleWrite_nid_postfix(reinterpret_cast<std::uintptr_t>(guard) + 8), "a write fault in the guard page is not resumed");
    Require(Protection(guard) == PAGE_NOACCESS, "the write tracker keeps the guard after a fault in it");
    std::array<void*, 64> pinned{};
    const auto pinnedCount = Collect(mapped, bytes, pinned.data());
    Require(pinnedCount == 15, "the pinned shared context but its guard page is collected as written");
    for (std::size_t i = 0; i < pinnedCount; ++i) Require(pinned[i] >= guard + GuardBytes, "the guard page of a pinned shared view is not collected as a write");
    Require(Protection(guard) == PAGE_NOACCESS, "collecting keeps the guard");
    GuestArena::GuestArenaPinWritable_nid_postfix(guard, SharedPageBytes);
    GuestArena::GuestArenaUnpinWritable_nid_postfix(guard, SharedPageBytes);
    Require(Protection(guard) == PAGE_NOACCESS, "another pin keeps the guard");
    Require(sceKernelMprotect(guard, bytes, 3) == 0, "guest protection change of the shared context");
    Require(Protection(guard) == PAGE_NOACCESS && Protection(guard + GuardBytes) == PAGE_READWRITE, "the guard survives a guest protection change of a shared view");
    alias[8] = 7;
    RunGuarded(70, "fiber on a shared view runs");
    Require(sceFiberFinalize(g_guarded) == 0, "finalize on a shared view");
    Require(Protection(guard) == PAGE_READWRITE, "finalize restores the shared view");
    Require(context[8] == 7, "the former guard page shows the write through the other view");
    Collect(mapped, bytes);
    Collect(aliased, bytes);
    Require(Collect(mapped, bytes) == 0 && Collect(aliased, bytes) == 0, "unpinned shared views collected clean");
    Require(Protection(guard) == PAGE_READONLY, "the former guard page is tracked again");
    context[16] = 9;
    Require(Collect(mapped, bytes) == 1 && Collect(aliased, bytes) == 1, "a write to the former guard page is collected in both views");
    Require(alias[16] == 9, "the write reaches the other view");
    Require(sceKernelMunmap(mapped, bytes) == 0 && sceKernelMunmap(aliased, bytes) == 0, "unmap direct memory");
    Require(sceKernelReleaseDirectMemory(phys, bytes) == 0, "release direct memory");
}

void CheckGuardOwnedAcrossRemap() {
    constexpr std::size_t bytes = 0x10000;
    constexpr int MapFixed = 0x10;
    void* mapped = nullptr;
    Require(sceKernelMapFlexibleMemory(&mapped, bytes, 3, 0) == 0, "map flexible memory for a remapped context");
    auto* context = static_cast<unsigned char*>(mapped);
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "unmapped", GuardedEntry, 1, context, bytes, nullptr, 0) == 0, "initialize before unmap");
    Require(!Readable(context), "context guarded before unmap");
    Require(sceKernelMunmap(context, bytes) == 0, "unmap a guarded context");
    void* remapped = context;
    Require(sceKernelMapFlexibleMemory(&remapped, bytes, 3, MapFixed) == 0 && remapped == context, "map flexible memory again at the same address");
    Require(Writable(context), "a new mapping replaces the guard");
    Require(_sceFiberInitializeImpl_nid_postfix(g_otherGuarded, "remapped", GuardedEntry, 1, context, bytes, nullptr, 0) == 0, "initialize after remap");
    Require(!Readable(context), "context guarded after remap");
    Require(sceFiberFinalize(g_guarded) == 0, "finalize the fiber of the unmapped context");
    Require(!Readable(context), "finalizing the fiber of the old mapping keeps the guard of the new one");
    RunGuarded(80, "fiber on the new mapping runs", g_otherGuarded);
    Require(sceFiberFinalize(g_otherGuarded) == 0, "finalize the fiber of the new mapping");
    Require(Writable(context), "finalize restores the new mapping");
    Require(sceKernelMunmap(context, bytes) == 0, "unmap the remapped context");
}

void CheckSharedCheckedContext() {
    auto* context = g_guardArea.data();
    Require(sceFiberStartContextSizeCheck(0) == 0, "start size check for a shared context");
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "shared first", CheckedEntry, 0, context, g_guardArea.size(), nullptr, 0) == 0, "initialize the first fiber of a shared context");
    Require(_sceFiberInitializeImpl_nid_postfix(g_otherGuarded, "shared second", CheckedEntry, 0, context, g_guardArea.size(), nullptr, 0) == 0, "initialize the second fiber of a shared context");
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "shared again", CheckedEntry, 0, context, g_guardArea.size(), nullptr, 0) == 0, "reinitialize a fiber of a guarded shared context");
    Require(!Readable(context), "the shared context stays guarded");
    FiberInfo info{};
    info.size = sizeof(info);
    Require(sceFiberGetInfo(g_guarded, &info) == 0 && info.size_context_margin == g_guardArea.size(), "the reinitialized shared context is all margin");
    Require(sceFiberFinalize(g_guarded) == 0, "finalize the first fiber of a shared context");
    Require(!Readable(context), "the other fiber keeps the guard");
    Require(sceFiberFinalize(g_otherGuarded) == 0, "finalize the second fiber of a shared context");
    Require(Writable(context), "the last finalize removes the guard of a shared context");
    Require(sceFiberStopContextSizeCheck() == 0, "stop size check for a shared context");
}

void CheckGuardTickets() {
    constexpr std::size_t bytes = 0x10000;
    void* mapped = nullptr;
    Require(sceKernelMapFlexibleMemory(&mapped, bytes, 3, 0) == 0, "map flexible memory for guard tickets");
    const auto unguardThrows = [&](std::uint64_t ticket) {
        try {
            GuestArena::GuestArenaUnguard_nid_no_patch(mapped, GuardBytes, ticket);
        } catch (const std::logic_error&) {
            return true;
        }
        return false;
    };
    const auto first = GuestArena::GuestArenaGuard_nid_no_patch(mapped, GuardBytes);
    const auto second = GuestArena::GuestArenaGuard_nid_no_patch(mapped, GuardBytes);
    Require(first != 0 && second != 0 && first != second, "each guard of a page has its own ticket");
    Require(!Readable(mapped), "a page guarded twice is unreadable");
    Require(unguardThrows(second + 1), "removing a guard with an unknown ticket throws");
    Require(!Readable(mapped), "a refused removal keeps the guard");
    GuestArena::GuestArenaUnguard_nid_no_patch(mapped, GuardBytes, first);
    Require(!Readable(mapped), "the second owner keeps the guard");
    Require(unguardThrows(first), "removing a guard twice with the same ticket throws");
    Require(sceKernelMunmap(mapped, bytes) == 0, "unmap a guarded page");
    GuestArena::GuestArenaUnguard_nid_no_patch(mapped, GuardBytes, second);
    Require(unguardThrows(second), "a guard dropped by an unmap is released once");
}

bool ThrowsRuntimeError(void (*call)()) {
    try {
        call();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

void CheckGuardChangedOutsideRegistry() {
    auto* context = g_guardArea.data();
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "changed", GuardedEntry, 1, context, g_guardArea.size(), nullptr, 0) == 0, "initialize before a change outside the guard registry");
    DWORD previous = 0;
    Require(VirtualProtect(context, GuardBytes, PAGE_READWRITE, &previous) && previous == PAGE_NOACCESS, "change the guard page outside the guard registry");
    Require(ThrowsRuntimeError([] { _sceFiberInitializeImpl_nid_postfix(g_otherGuarded, "joining", GuardedEntry, 1, g_guardArea.data(), g_guardArea.size(), nullptr, 0); }), "sharing a guard changed outside the guard registry throws");
    Require(sceFiberFinalize(g_otherGuarded) == FiberErrorInvalid, "a fiber whose guard was refused is not initialized");
    Require(ThrowsRuntimeError([] { sceFiberFinalize(g_guarded); }), "the last finalize of a guard changed outside the guard registry throws");
    Require(Writable(context), "a refused finalize leaves the changed page as it is");
    Require(VirtualProtect(context, GuardBytes, PAGE_NOACCESS, &previous), "put the guard page back");
    Require(sceFiberFinalize(g_guarded) == 0, "a refused finalize keeps the fiber initialized");
    Require(Writable(context), "finalize restores the page once the guard is back");
}

void CheckGuardProtectedOutsideArena() {
    constexpr std::size_t bytes = 2 * SharedPageBytes;
    const auto start = (reinterpret_cast<std::uintptr_t>(g_guardArea.data()) + SharedPageBytes - 1) & ~(SharedPageBytes - 1);
    auto* context = reinterpret_cast<unsigned char*>(start);
    std::fill(context, context + bytes, 0);
    Require(!GuestArena::GuestArenaContains_nid_postfix(context, bytes), "the host context is outside the guest arena");
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "image", GuardedEntry, 1, context, bytes, nullptr, 0) == 0, "initialize outside the guest arena");
    Require(Protection(context) == PAGE_NOACCESS, "the host context is guarded");
    Require(sceKernelMprotect(context, SharedPageBytes, 0) == 0, "guest PROT_NONE outside the guest arena under the guard");
    Require(Protection(context) == PAGE_NOACCESS && Protection(context + GuardBytes) == PAGE_NOACCESS, "the guest PROT_NONE covers the guarded page");
    Require(sceFiberFinalize(g_guarded) == 0, "finalize under a guest PROT_NONE outside the guest arena");
    Require(Protection(context) == PAGE_NOACCESS, "finalize keeps the guest PROT_NONE of the former guard page");
    Require(sceKernelMprotect(context, SharedPageBytes, 3) == 0, "guest read-write protection outside the guest arena after finalize");
    Require(Writable(context) && Writable(context + GuardBytes), "the former guard page is writable again");
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "image read-only", GuardedEntry, 1, context, bytes, nullptr, 0) == 0, "initialize outside the guest arena again");
    Require(sceKernelMprotect(context, SharedPageBytes, 1) == 0, "guest read-only protection outside the guest arena under the guard");
    Require(Protection(context) == PAGE_NOACCESS && Protection(context + GuardBytes) == PAGE_READONLY, "the guard survives a guest protection change outside the guest arena");
    Require(sceFiberFinalize(g_guarded) == 0, "finalize under a guest read-only protection outside the guest arena");
    Require(Protection(context) == PAGE_READONLY, "finalize restores the guest read-only protection of the former guard page");
    Require(sceKernelMprotect(context, SharedPageBytes, 3) == 0, "guest read-write protection outside the guest arena at the end");
    Require(Writable(context), "the former guard page is writable at the end");
}

void CheckGuardUnmappedOutsideArena() {
    constexpr std::size_t bytes = 0x10000;
    constexpr int MapFixed = 0x10;
    void* range = VirtualAlloc(nullptr, 2 * bytes, MEM_RESERVE, PAGE_NOACCESS);
    Require(range != nullptr && VirtualFree(range, 0, MEM_RELEASE), "find a free range outside the guest arena");
    Require(!GuestArena::GuestArenaContains_nid_postfix(range, 2 * bytes), "the free range is outside the guest arena");
    void* mapped = range;
    Require(sceKernelMapFlexibleMemory(&mapped, 2 * bytes, 3, MapFixed) == 0 && mapped == range, "map flexible memory outside the guest arena");
    auto* low = static_cast<unsigned char*>(mapped);
    auto* high = low + bytes;
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "decommitted", GuardedEntry, 1, high, bytes, nullptr, 0) == 0, "initialize on the upper half outside the guest arena");
    Require(_sceFiberInitializeImpl_nid_postfix(g_otherGuarded, "released", GuardedEntry, 1, low, bytes, nullptr, 0) == 0, "initialize on the lower half outside the guest arena");
    Require(Protection(low) == PAGE_NOACCESS && Protection(high) == PAGE_NOACCESS, "both halves outside the guest arena are guarded");
    Require(sceKernelMunmap(high, bytes) == 0, "unmap the upper half outside the guest arena");
    Require(!ThrowsRuntimeError([] { Require(sceFiberFinalize(g_guarded) == 0, "finalize after a decommit outside the guest arena"); }), "a guard dropped by a decommit outside the guest arena is released once");
    Require(sceKernelMunmap(low, bytes) == 0, "unmap the lower half outside the guest arena");
    Require(!ThrowsRuntimeError([] { Require(sceFiberFinalize(g_otherGuarded) == 0, "finalize after a release outside the guest arena"); }), "a guard dropped by a release outside the guest arena is released once");
    mapped = range;
    Require(sceKernelMapFlexibleMemory(&mapped, bytes, 0, MapFixed) == 0 && mapped == range, "map PROT_NONE memory again at the released address");
    Require(_sceFiberInitializeImpl_nid_postfix(g_guarded, "remapped outside", GuardedEntry, 1, low, bytes, nullptr, 0) == 0, "initialize on the new PROT_NONE mapping");
    Require(sceKernelMprotect(low, bytes, 3) == 0, "guest read-write protection of the new mapping");
    Require(Protection(low) == PAGE_NOACCESS && Writable(low + GuardBytes), "the new mapping is guarded");
    Require(sceFiberFinalize(g_guarded) == 0, "finalize on the new mapping outside the guest arena");
    Require(Writable(low), "finalize restores the protection of the new mapping");
    Require(sceKernelMunmap(low, bytes) == 0, "unmap the new mapping outside the guest arena");
}

void RunGuardTests() {
    CheckGuardedHostContext();
    CheckUnguardableContexts();
    CheckGuardedSizeCheck();
    CheckGuardedFlexibleContext();
    CheckGuardedSharedContext();
    CheckGuardOwnedAcrossRemap();
    CheckSharedCheckedContext();
    CheckGuardTickets();
    CheckGuardChangedOutsideRegistry();
    CheckGuardProtectedOutsideArena();
    CheckGuardUnmappedOutsideArena();
}
#endif

[[gnu::noinline]] std::int32_t RunFirst(std::uint64_t* returned) {
    g_threadFramePointer = reinterpret_cast<std::uint64_t>(__builtin_frame_address(0));
    const std::int32_t result = sceFiberRun_nid_postfix(g_first, 100, returned);
    asm volatile("" ::: "memory");
    return result;
}

}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
#ifdef _WIN32
    if (argc > 1 && std::strcmp(argv[1], "guard") == 0) {
        RunGuardTests();
        return 0;
    }
#endif
    RunSmallContextFirst();
    CheckNoGuardByDefault();
    FiberObject* self = nullptr;
    Require(sceFiberGetSelf(&self) == FiberErrorPermission, "no fiber on the thread");
    Require(_sceFiberInitializeImpl_nid_postfix(g_first, "first", FirstEntry, 11, g_firstContext.data(), g_firstContext.size(), nullptr, 0) == 0, "initialize first");
    Require(_sceFiberInitializeImpl_nid_postfix(g_second, "second", SecondEntry, 22, g_secondContext.data(), g_secondContext.size(), nullptr, 0) == 0, "initialize second");
    const auto threadCsr = _mm_getcsr();
    std::uint64_t returned = 0;
    std::uint64_t framePointer = 0;
    Require(sceFiberGetThreadFramePointerAddress(&framePointer) == FiberErrorPermission, "frame pointer outside a fiber");
    Require(_sceFiberGetThreadFramePointerAddress(&framePointer) == FiberErrorPermission, "underscore frame pointer outside a fiber");
    Require(RunFirst(&returned) == 0 && returned == 250, "run first until second returns");
    Require(_mm_getcsr() == threadCsr, "thread MXCSR restored after the fibers");
    std::thread([&] {
        Require(sceFiberRun_nid_postfix(g_second, 260, &returned) == 0 && returned == 301, "resume second on another thread");
    }).join();
    Require(sceFiberFinalize(g_first) == 0, "finalize first");
    Require(sceFiberFinalize(g_second) == 0, "finalize second");

    auto* checked = reinterpret_cast<FiberObject*>(&g_checkedStorage);
    FiberInfo info{};
    info.size = sizeof(info);
    Require(sceFiberStopContextSizeCheck() == FiberErrorState, "stop before start");
    Require(sceFiberStartContextSizeCheck(1) == FiberErrorInvalid, "start with flags");
    Require(_sceFiberInitializeImpl_nid_postfix(checked, "unchecked", CheckedEntry, 0, g_checkedContext.data(), g_checkedContext.size(), nullptr, 0) == 0, "initialize unchecked");
    Require(sceFiberGetInfo(checked, &info) == 0 && info.size_context_margin == ~0ull, "no margin without size check");
    Require(sceFiberFinalize(checked) == 0, "finalize unchecked");
    Require(sceFiberStartContextSizeCheck(0) == 0, "start size check");
    Require(sceFiberStartContextSizeCheck(0) == FiberErrorState, "start twice");
    Require(_sceFiberInitializeImpl_nid_postfix(checked, "checked", CheckedEntry, 0, g_checkedContext.data(), g_checkedContext.size(), nullptr, 0) == 0, "initialize checked");
    Require(sceFiberGetInfo(checked, &info) == 0 && info.size_context_margin == g_checkedContext.size(), "untouched context is all margin");
    Require(sceFiberRun_nid_postfix(checked, 0, nullptr) == 0, "run checked");
    Require(sceFiberGetInfo(checked, &info) == 0, "checked info");
    Require(info.size_context_margin > 0 && info.size_context_margin < g_checkedContext.size() - 4096, "margin below the used stack");
    Require(info.size_context_margin % 8 == 0, "margin in whole words");
    Require(sceFiberStopContextSizeCheck() == 0, "stop size check");
}
