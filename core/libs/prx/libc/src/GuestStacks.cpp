#include "prx/libc/include/GuestStacks.hpp"

#ifdef _WIN32
#include "prx/libc/include/GuestArena.hpp"
#include <iterator>
#include <map>
#include <mutex>
#include <windows.h>
#endif

namespace GuestStacks {
namespace {

#ifdef _WIN32
// PS5 ABI constant: the job-context pointer lives at FS:-0x78 (0x78 bytes before the thread pointer).
// In AnyPS5 terms: *(TlsSlot + threadPointerOffset - 0x78).
constexpr std::ptrdiff_t kPs5JobCtxTpDisp = -0x78;
// Offset within .gtls where WindowsTlsBuilder stored threadPointerOffset (just after the 4-byte index slot).
constexpr std::uintptr_t kGtlsTpoOffset = 0x2c;
// Offset within a PS5 1MB stack frame that holds the current job descriptor.
constexpr std::uintptr_t kStackJobSlot = 0x48;

struct GuestTlsCache {
    bool valid = false;
    DWORD tlsIndex = 0;
    std::uint32_t threadPointerOffset = 0;
};

std::once_flag gGtlsOnce;
GuestTlsCache gGtlsCache;

static void InitGuestTlsCache() {
    const auto* base = static_cast<const std::uint8_t*>(static_cast<const void*>(GetModuleHandleW(nullptr)));
    if (!base) return;
    const auto lfanew = *reinterpret_cast<const std::uint32_t*>(base + 0x3c);
    const auto* ntHdr = base + lfanew;
    // PE32+ optional header: DataDirectory[9]=TLS at OptHdr+0x70+9*8.  OptHdr starts at ntHdr+24.
    const auto tlsDirRva = *reinterpret_cast<const std::uint32_t*>(ntHdr + 24 + 0x70 + 9 * 8);
    const auto tlsDirSize = *reinterpret_cast<const std::uint32_t*>(ntHdr + 24 + 0x70 + 9 * 8 + 4);
    if (tlsDirRva == 0 || tlsDirSize < 40) return;
    // IMAGE_TLS_DIRECTORY64 field +0x10 = AddressOfIndex (absolute VA).
    const auto addrOfIndex = *reinterpret_cast<const std::uint64_t*>(base + tlsDirRva + 0x10);
    if (addrOfIndex == 0) return;
    const auto* indexPtr = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(addrOfIndex));
    // threadPointerOffset is stored at addrOfIndex+4 (kGtlsTpoOffset - 0x28).
    const auto tpo = *reinterpret_cast<const std::uint32_t*>(indexPtr + kGtlsTpoOffset - 0x28);
    if (static_cast<std::ptrdiff_t>(tpo) < -kPs5JobCtxTpDisp) return;  // tpo < 0x78: invalid
    gGtlsCache.tlsIndex = *reinterpret_cast<const DWORD*>(indexPtr);
    gGtlsCache.threadPointerOffset = tpo;
    gGtlsCache.valid = true;
}

static std::uintptr_t CurrentGuestJobContext() {
    std::call_once(gGtlsOnce, InitGuestTlsCache);
    if (!gGtlsCache.valid) return 0;
    const auto* tlsSlot = static_cast<const std::uint8_t*>(TlsGetValue(gGtlsCache.tlsIndex));
    if (!tlsSlot) return 0;
    const auto fieldOff = static_cast<std::ptrdiff_t>(gGtlsCache.threadPointerOffset) + kPs5JobCtxTpDisp;
    return *reinterpret_cast<const std::uintptr_t*>(tlsSlot + fieldOff);
}

struct StackBounds {
    std::uintptr_t base;
    std::uintptr_t limit;
    std::uintptr_t deallocation;
};

constexpr std::size_t DeallocationStackOffset = 0x1478;

std::mutex rangesLock;
std::map<std::uintptr_t, std::uintptr_t> ranges;

StackBounds CurrentBounds() {
    auto* tib = reinterpret_cast<NT_TIB*>(NtCurrentTeb());
    auto* teb = reinterpret_cast<std::uint8_t*>(tib);
    return {reinterpret_cast<std::uintptr_t>(tib->StackBase), reinterpret_cast<std::uintptr_t>(tib->StackLimit),
            *reinterpret_cast<std::uintptr_t*>(teb + DeallocationStackOffset)};
}

void ApplyBounds(const StackBounds& bounds) {
    auto* tib = reinterpret_cast<NT_TIB*>(NtCurrentTeb());
    auto* teb = reinterpret_cast<std::uint8_t*>(tib);
    tib->StackBase = reinterpret_cast<void*>(bounds.base);
    tib->StackLimit = reinterpret_cast<void*>(bounds.limit);
    *reinterpret_cast<std::uintptr_t*>(teb + DeallocationStackOffset) = bounds.deallocation;
}

bool RegisteredBounds(std::uintptr_t stackPointer, StackBounds& bounds) {
    std::lock_guard lock(rangesLock);
    const auto next = ranges.upper_bound(stackPointer);
    if (next == ranges.begin()) return false;
    const auto& [low, high] = *std::prev(next);
    if (stackPointer >= high) return false;
    bounds = {high, low, low};
    return true;
}

bool HostBounds(std::uintptr_t stackPointer, StackBounds& bounds) {
    MEMORY_BASIC_INFORMATION memory{};
    if (VirtualQuery(reinterpret_cast<const void*>(stackPointer), &memory, sizeof(memory)) != sizeof(memory) || memory.State != MEM_COMMIT) return false;
    const auto base = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
    bounds = {base + memory.RegionSize, base, reinterpret_cast<std::uintptr_t>(memory.AllocationBase)};
    return true;
}
#endif

}

void GuestStackRegister_nid_no_patch(const void* pointer, std::size_t bytes) {
#ifdef _WIN32
    const auto low = reinterpret_cast<std::uintptr_t>(pointer);
    const auto high = low + bytes;
    std::lock_guard lock(rangesLock);
    const auto same = ranges.find(low);
    if (same != ranges.end() && same->second == high) return;
    auto overlap = ranges.upper_bound(low);
    if (overlap != ranges.begin() && std::prev(overlap)->second > low) --overlap;
    while (overlap != ranges.end() && overlap->first < high) {
        GuestArena::GuestArenaUnpinWritable_nid_postfix(reinterpret_cast<const void*>(overlap->first), overlap->second - overlap->first);
        overlap = ranges.erase(overlap);
    }
    GuestArena::GuestArenaPinWritable_nid_postfix(pointer, bytes);
    ranges.emplace(low, high);
#else
    (void)pointer;
    (void)bytes;
#endif
}

void APS5_VABI GuestStackSwitch_nid_no_patch(std::uintptr_t stackPointer) {
#ifdef _WIN32
    const auto current = CurrentBounds();
    if (stackPointer >= current.deallocation && stackPointer < current.base) return;
    StackBounds target{};
    const bool registered = RegisteredBounds(stackPointer, target);
    if (registered || HostBounds(stackPointer, target)) {
        ApplyBounds(target);
        // PS5 scheduler fast path reads stack_base+0x48 as the current job descriptor.
        // The kernel pre-fills this for each fiber stack; mirror the current job context here
        // so the dispatch trampoline does not crash on a null dereference.
        if (registered) {
            const auto newBase = stackPointer & ~static_cast<std::uintptr_t>(0xFFFFF);
            auto& slot = *reinterpret_cast<std::uintptr_t*>(newBase + kStackJobSlot);
            if (slot == 0) {
                // tls[0x130] is the scheduler context; dereference once more to get the
                // job descriptor that the dispatch-trampoline fast path expects here.
                if (const auto schedCtx = CurrentGuestJobContext())
                    if (const auto jobDesc = *reinterpret_cast<const std::uintptr_t*>(schedCtx))
                        slot = jobDesc;
            }
        }
    }
#else
    (void)stackPointer;
#endif
}

}
