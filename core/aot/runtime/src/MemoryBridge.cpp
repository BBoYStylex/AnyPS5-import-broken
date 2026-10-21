#include <runtime/MemoryBridge.hpp>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

#include <algorithm>

namespace Aot::Runtime {

MemoryBridge::MemoryBridge()
    : hostBasePointer(nullptr),
      baseGuestAddress(0),
      nextAvailableOffset(0) {}

MemoryBridge::~MemoryBridge() {
    Shutdown();
}

MemoryBridge::MemoryBridge(MemoryBridge&& other) noexcept
    : hostBasePointer(other.hostBasePointer),
      baseGuestAddress(other.baseGuestAddress),
      activeBlocks(std::move(other.activeBlocks)),
      nextAvailableOffset(other.nextAvailableOffset) {
    other.hostBasePointer = nullptr;
    other.baseGuestAddress = 0;
    other.nextAvailableOffset = 0;
}

MemoryBridge& MemoryBridge::operator=(MemoryBridge&& other) noexcept {
    if (this != &other) {
        Shutdown();
        hostBasePointer = other.hostBasePointer;
        baseGuestAddress = other.baseGuestAddress;
        activeBlocks = std::move(other.activeBlocks);
        nextAvailableOffset = other.nextAvailableOffset;
        other.hostBasePointer = nullptr;
        other.baseGuestAddress = 0;
        other.nextAvailableOffset = 0;
    }
    return *this;
}

bool MemoryBridge::Initialize(uint64_t preferredBaseAddress) {
    std::lock_guard<std::mutex> lock(allocationMutex);
    if (hostBasePointer != nullptr) {
        return true;
    }

    baseGuestAddress = preferredBaseAddress;

#if defined(_WIN32)
    hostBasePointer = VirtualAlloc(
        reinterpret_cast<LPVOID>(preferredBaseAddress),
        GuestAddressSpaceSize,
        MEM_RESERVE,
        PAGE_NOACCESS
    );
    if (hostBasePointer == nullptr) {
        hostBasePointer = VirtualAlloc(
            nullptr,
            GuestAddressSpaceSize,
            MEM_RESERVE,
            PAGE_NOACCESS
        );
    }
#else
    hostBasePointer = mmap(
        reinterpret_cast<void*>(preferredBaseAddress),
        GuestAddressSpaceSize,
        PROT_NONE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    if (hostBasePointer == MAP_FAILED) {
        hostBasePointer = nullptr;
    }
#endif

    nextAvailableOffset = 0;
    return hostBasePointer != nullptr;
}

void MemoryBridge::Shutdown() {
    std::lock_guard<std::mutex> lock(allocationMutex);
    if (hostBasePointer == nullptr) {
        return;
    }

#if defined(_WIN32)
    VirtualFree(hostBasePointer, 0, MEM_RELEASE);
#else
    munmap(hostBasePointer, GuestAddressSpaceSize);
#endif

    hostBasePointer = nullptr;
    baseGuestAddress = 0;
    nextAvailableOffset = 0;
    activeBlocks.clear();
}

uint64_t MemoryBridge::Allocate(size_t size, MemoryPoolType poolType, size_t alignment) {
    std::lock_guard<std::mutex> lock(allocationMutex);
    if (hostBasePointer == nullptr || size == 0) {
        return 0;
    }

    size_t actualAlignment = std::max(alignment, StandardPageSize);
    size_t alignedSize = (size + StandardPageSize - 1) & ~(StandardPageSize - 1);

    uint64_t currentOffset = (nextAvailableOffset + actualAlignment - 1) & ~(actualAlignment - 1);
    if (currentOffset + alignedSize > GuestAddressSpaceSize) {
        return 0;
    }

    uint8_t* targetHostAddress = static_cast<uint8_t*>(hostBasePointer) + currentOffset;

#if defined(_WIN32)
    void* committedPtr = VirtualAlloc(
        targetHostAddress,
        alignedSize,
        MEM_COMMIT,
        PAGE_READWRITE
    );
    if (committedPtr == nullptr) {
        return 0;
    }
#else
    if (mprotect(targetHostAddress, alignedSize, PROT_READ | PROT_WRITE) != 0) {
        return 0;
    }
#endif

    uint64_t allocatedGuestAddress = baseGuestAddress + currentOffset;
    activeBlocks[allocatedGuestAddress] = MemoryBlock{
        .GuestVirtualAddress = allocatedGuestAddress,
        .Size = alignedSize,
        .PoolType = poolType,
        .Committed = true
    };

    nextAvailableOffset = currentOffset + alignedSize;
    return allocatedGuestAddress;
}

bool MemoryBridge::Deallocate(uint64_t guestVirtualAddress) {
    std::lock_guard<std::mutex> lock(allocationMutex);
    auto iterator = activeBlocks.find(guestVirtualAddress);
    if (iterator == activeBlocks.end()) {
        return false;
    }

    const MemoryBlock& block = iterator->second;
    uint64_t offset = block.GuestVirtualAddress - baseGuestAddress;
    uint8_t* targetHostAddress = static_cast<uint8_t*>(hostBasePointer) + offset;

#if defined(_WIN32)
    VirtualFree(targetHostAddress, block.Size, MEM_DECOMMIT);
#else
    mprotect(targetHostAddress, block.Size, PROT_NONE);
#endif

    activeBlocks.erase(iterator);
    return true;
}

void* MemoryBridge::TranslateGuestToHost(uint64_t guestVirtualAddress) const {
    if (!IsValidGuestAddress(guestVirtualAddress)) {
        return nullptr;
    }
    uint64_t offset = guestVirtualAddress - baseGuestAddress;
    return static_cast<uint8_t*>(hostBasePointer) + offset;
}

bool MemoryBridge::IsValidGuestAddress(uint64_t guestVirtualAddress) const {
    if (hostBasePointer == nullptr) {
        return false;
    }
    return guestVirtualAddress >= baseGuestAddress &&
           guestVirtualAddress < (baseGuestAddress + GuestAddressSpaceSize);
}

uint64_t MemoryBridge::TranslateHostToGuest(const void* hostPointer) const {
    if (hostBasePointer == nullptr || hostPointer == nullptr) {
        return 0;
    }
    const auto* hostBytes = static_cast<const uint8_t*>(hostPointer);
    const auto* baseBytes = static_cast<const uint8_t*>(hostBasePointer);

    if (hostBytes < baseBytes || hostBytes >= (baseBytes + GuestAddressSpaceSize)) {
        return 0;
    }
    return baseGuestAddress + (hostBytes - baseBytes);
}

bool MemoryBridge::QueryMemoryBlock(uint64_t guestVirtualAddress, MemoryBlock& outBlock) const {
    std::lock_guard<std::mutex> lock(allocationMutex);
    for (const auto& [baseAddr, block] : activeBlocks) {
        if (guestVirtualAddress >= block.GuestVirtualAddress &&
            guestVirtualAddress < (block.GuestVirtualAddress + block.Size)) {
            outBlock = block;
            return true;
        }
    }
    return false;
}

size_t MemoryBridge::GetTotalCommittedBytes() const {
    std::lock_guard<std::mutex> lock(allocationMutex);
    size_t total = 0;
    for (const auto& [baseAddr, block] : activeBlocks) {
        if (block.Committed) {
            total += block.Size;
        }
    }
    return total;
}

size_t MemoryBridge::GetCommittedBytesByPool(MemoryPoolType poolType) const {
    std::lock_guard<std::mutex> lock(allocationMutex);
    size_t total = 0;
    for (const auto& [baseAddr, block] : activeBlocks) {
        if (block.Committed && block.PoolType == poolType) {
            total += block.Size;
        }
    }
    return total;
}

} // namespace Aot::Runtime
