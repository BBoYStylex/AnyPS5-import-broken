#ifndef CORE_AOT_IO_DIRECTSTORAGESTREAMING_HPP
#define CORE_AOT_IO_DIRECTSTORAGESTREAMING_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <unordered_map>

namespace Aot::Io {

enum class CompressionType {
    None,
    Zlib,
    Zstd
};

enum class DestinationType {
    HostMemory,
    GpuDeviceMemory
};

struct ExternalGpuMemoryResource {
    uint64_t ResourceId;
    void* SharedHandle;
    void* MappedHostAddress;
    uint64_t SizeBytes;
    bool IsGpuExclusive;
};

struct StreamRequest {
    uint64_t RequestId;
    std::string SourceFilePath;
    uint64_t FileOffset;
    size_t CompressedSize;
    size_t UncompressedSize;
    void* TargetDestination;
    CompressionType Compression;
    DestinationType Destination;
    std::function<void(uint64_t, bool)> CompletionCallback;
};

struct StreamingStatistics {
    uint64_t TotalRequests{0};
    uint64_t CompletedRequests{0};
    uint64_t FailedRequests{0};
    uint64_t TotalBytesStreamed{0};
};

class DirectStorageStreaming {
public:
    static DirectStorageStreaming& Instance();

    DirectStorageStreaming();
    ~DirectStorageStreaming();

    DirectStorageStreaming(const DirectStorageStreaming&) = delete;
    DirectStorageStreaming& operator=(const DirectStorageStreaming&) = delete;

    bool Initialize(bool preferDirectStorage = true);
    void Shutdown();

    uint64_t EnqueueRequest(StreamRequest request);
    void WaitForRequest(uint64_t requestId);
    [[nodiscard]] bool IsRequestCompleted(uint64_t requestId, bool* outSuccess = nullptr) const;
    [[nodiscard]] bool IsHardwareDecompressionSupported() const;
    [[nodiscard]] StreamingStatistics GetStatistics() const;

    bool RegisterExternalGpuMemory(const ExternalGpuMemoryResource& resource);
    void UnregisterExternalGpuMemory(uint64_t resourceId);
    bool HasExternalGpuMemory(uint64_t resourceId) const;
    uint64_t StreamToGpuResource(
        uint64_t resourceId,
        uint64_t resourceOffset,
        std::string_view sourcePath,
        uint64_t fileOffset,
        size_t sizeBytes,
        CompressionType compression = CompressionType::None,
        std::function<void(uint64_t, bool)> callback = nullptr
    );

private:
    void workerThreadLoop();
    bool processRequestCpuFallback(const StreamRequest& request);

    std::atomic<bool> initialized{false};
    std::atomic<bool> hardwareDecompressionAvailable{false};
    std::atomic<bool> workerRunning{false};
    std::atomic<uint64_t> nextRequestId{1};
    std::atomic<uint64_t> statTotalRequests{0};
    std::atomic<uint64_t> statCompletedRequests{0};
    std::atomic<uint64_t> statFailedRequests{0};
    std::atomic<uint64_t> statTotalBytesStreamed{0};

    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::queue<StreamRequest> pendingRequests;

    std::mutex completionMutex;
    std::condition_variable completionCondition;
    std::unordered_map<uint64_t, bool> completedRequests;

    mutable std::mutex resourceMutex;
    std::unordered_map<uint64_t, ExternalGpuMemoryResource> registeredResources;

    std::vector<std::thread> workerThreads;
};

} // namespace Aot::Io
#endif
