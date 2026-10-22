#ifndef CORE_AOT_RUNTIME_MEMORYBRIDGE_HPP
#define CORE_AOT_RUNTIME_MEMORYBRIDGE_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <optional>

namespace Aot::Runtime {

enum class MemoryPoolType {
    HostRam,
    SharedVisibleVram,
    DeviceLocalVram
};

enum class MemoryProtection : uint32_t {
    None = 0,
    Read = 1,
    Write = 2,
    Execute = 4,
    ReadWrite = 3,
    ReadExecute = 5,
    ReadWriteExecute = 7
};

struct MemoryBlock {
    uint64_t GuestVirtualAddress;
    size_t Size;
    MemoryPoolType PoolType;
    MemoryProtection Protection;
    bool Committed;
};

class MemoryBridge {
public:
    static constexpr size_t GuestAddressSpaceSize = 16ULL * 1024ULL * 1024ULL * 1024ULL;
    static constexpr size_t StandardPageSize = 16ULL * 1024ULL;
    static constexpr size_t LargePageSize = 2ULL * 1024ULL * 1024ULL;

    MemoryBridge();
    ~MemoryBridge();

    MemoryBridge(const MemoryBridge&) = delete;
    MemoryBridge& operator=(const MemoryBridge&) = delete;
    MemoryBridge(MemoryBridge&&) noexcept;
    MemoryBridge& operator=(MemoryBridge&&) noexcept;

    bool Initialize(uint64_t preferredBaseAddress = 0x1000000000ULL);
    void Shutdown();

    uint64_t Allocate(size_t size, MemoryPoolType poolType, size_t alignment = StandardPageSize);
    bool Deallocate(uint64_t guestVirtualAddress);
    bool Protect(uint64_t guestVirtualAddress, size_t size, MemoryProtection protection);

    void* TranslateGuestToHost(uint64_t guestVirtualAddress) const;
    [[nodiscard]] uint64_t TranslateHostToGuest(const void* hostPointer) const;
    [[nodiscard]] bool QueryMemoryBlock(uint64_t guestVirtualAddress, MemoryBlock& outBlock) const;
    [[nodiscard]] bool IsValidGuestAddress(uint64_t guestVirtualAddress) const;
    [[nodiscard]] size_t GetTotalCommittedBytes() const;
    [[nodiscard]] size_t GetCommittedBytesByPool(MemoryPoolType poolType) const;

private:
    void* hostBasePointer;
    uint64_t baseGuestAddress;
    mutable std::mutex allocationMutex;
    std::unordered_map<uint64_t, MemoryBlock> activeBlocks;
    uint64_t nextAvailableOffset;
};

} // namespace Aot::Runtime
#endif
