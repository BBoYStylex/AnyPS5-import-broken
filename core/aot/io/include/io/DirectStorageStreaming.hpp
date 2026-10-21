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
    bool IsHardwareDecompressionSupported() const;

private:
    void workerThreadLoop();
    bool processRequestCpuFallback(const StreamRequest& request);

    std::atomic<bool> initialized{false};
    std::atomic<bool> hardwareDecompressionAvailable{false};
    std::atomic<bool> workerRunning{false};
    std::atomic<uint64_t> nextRequestId{1};

    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::queue<StreamRequest> pendingRequests;

    std::mutex completionMutex;
    std::condition_variable completionCondition;
    std::unordered_map<uint64_t, bool> completedRequests;

    std::vector<std::thread> workerThreads;
};

} // namespace Aot::Io
#endif
