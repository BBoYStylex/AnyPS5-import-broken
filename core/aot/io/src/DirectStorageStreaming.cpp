#include <io/DirectStorageStreaming.hpp>
#include <fstream>
#include <cstring>
#include <algorithm>

namespace Aot::Io {

DirectStorageStreaming& DirectStorageStreaming::Instance() {
    static DirectStorageStreaming instance;
    return instance;
}

DirectStorageStreaming::DirectStorageStreaming() = default;

DirectStorageStreaming::~DirectStorageStreaming() {
    Shutdown();
}

bool DirectStorageStreaming::Initialize(bool preferDirectStorage) {
    if (initialized.load(std::memory_order_acquire)) {
        return true;
    }

    hardwareDecompressionAvailable.store(preferDirectStorage, std::memory_order_release);
    workerRunning.store(true, std::memory_order_release);

    unsigned int threadCount = std::max(2u, std::thread::hardware_concurrency());
    for (unsigned int i = 0; i < threadCount; ++i) {
        workerThreads.emplace_back(&DirectStorageStreaming::workerThreadLoop, this);
    }

    initialized.store(true, std::memory_order_release);
    return true;
}

void DirectStorageStreaming::Shutdown() {
    if (!initialized.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        workerRunning.store(false, std::memory_order_release);
    }
    queueCondition.notify_all();

    for (std::thread& worker : workerThreads) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workerThreads.clear();

    std::lock_guard<std::mutex> lock(queueMutex);
    while (!pendingRequests.empty()) {
        pendingRequests.pop();
    }

    std::lock_guard<std::mutex> resourceLock(resourceMutex);
    registeredResources.clear();
}

uint64_t DirectStorageStreaming::EnqueueRequest(StreamRequest request) {
    uint64_t id = nextRequestId.fetch_add(1, std::memory_order_relaxed);
    request.RequestId = id;
    statTotalRequests.fetch_add(1, std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        pendingRequests.push(std::move(request));
    }

    queueCondition.notify_all();
    return id;
}

void DirectStorageStreaming::WaitForRequest(uint64_t requestId) {
    if (requestId == 0) {
        return;
    }

    std::unique_lock<std::mutex> lock(completionMutex);
    completionCondition.wait(lock, [this, requestId]() {
        return completedRequests.contains(requestId);
    });
}

bool DirectStorageStreaming::IsRequestCompleted(uint64_t requestId, bool* outSuccess) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(completionMutex));
    auto it = completedRequests.find(requestId);
    if (it != completedRequests.end()) {
        if (outSuccess != nullptr) {
            *outSuccess = it->second;
        }
        return true;
    }
    return false;
}

bool DirectStorageStreaming::IsHardwareDecompressionSupported() const {
    return hardwareDecompressionAvailable.load(std::memory_order_acquire);
}

StreamingStatistics DirectStorageStreaming::GetStatistics() const {
    return StreamingStatistics{
        .TotalRequests = statTotalRequests.load(std::memory_order_relaxed),
        .CompletedRequests = statCompletedRequests.load(std::memory_order_relaxed),
        .FailedRequests = statFailedRequests.load(std::memory_order_relaxed),
        .TotalBytesStreamed = statTotalBytesStreamed.load(std::memory_order_relaxed)
    };
}

bool DirectStorageStreaming::RegisterExternalGpuMemory(const ExternalGpuMemoryResource& resource) {
    if (resource.ResourceId == 0 || (resource.SharedHandle == nullptr && resource.MappedHostAddress == nullptr)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(resourceMutex);
    registeredResources[resource.ResourceId] = resource;
    return true;
}

void DirectStorageStreaming::UnregisterExternalGpuMemory(uint64_t resourceId) {
    std::lock_guard<std::mutex> lock(resourceMutex);
    registeredResources.erase(resourceId);
}

bool DirectStorageStreaming::HasExternalGpuMemory(uint64_t resourceId) const {
    std::lock_guard<std::mutex> lock(resourceMutex);
    return registeredResources.contains(resourceId);
}

uint64_t DirectStorageStreaming::StreamToGpuResource(
    uint64_t resourceId,
    uint64_t resourceOffset,
    std::string_view sourcePath,
    uint64_t fileOffset,
    size_t sizeBytes,
    CompressionType compression,
    std::function<void(uint64_t, bool)> callback) {
    ExternalGpuMemoryResource resource{};
    {
        std::lock_guard<std::mutex> lock(resourceMutex);
        auto it = registeredResources.find(resourceId);
        if (it == registeredResources.end()) {
            return 0;
        }
        resource = it->second;
    }

    if (resourceOffset + sizeBytes > resource.SizeBytes) {
        return 0;
    }

    void* destinationAddress = resource.MappedHostAddress;
    if (destinationAddress != nullptr) {
        destinationAddress = static_cast<uint8_t*>(destinationAddress) + resourceOffset;
    }

    StreamRequest request{
        .RequestId = 0,
        .SourceFilePath = std::string(sourcePath),
        .FileOffset = fileOffset,
        .CompressedSize = sizeBytes,
        .UncompressedSize = sizeBytes,
        .TargetDestination = destinationAddress,
        .Compression = compression,
        .Destination = DestinationType::GpuDeviceMemory,
        .CompletionCallback = std::move(callback)
    };

    return EnqueueRequest(std::move(request));
}

void DirectStorageStreaming::workerThreadLoop() {
    while (workerRunning.load(std::memory_order_acquire)) {
        StreamRequest request{};

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCondition.wait(lock, [this]() {
                return !workerRunning.load(std::memory_order_acquire) || !pendingRequests.empty();
            });

            if (!workerRunning.load(std::memory_order_acquire) && pendingRequests.empty()) {
                break;
            }

            request = std::move(pendingRequests.front());
            pendingRequests.pop();
        }

        bool success = processRequestCpuFallback(request);

        if (success) {
            statCompletedRequests.fetch_add(1, std::memory_order_relaxed);
            statTotalBytesStreamed.fetch_add(request.UncompressedSize, std::memory_order_relaxed);
        } else {
            statFailedRequests.fetch_add(1, std::memory_order_relaxed);
        }

        if (request.CompletionCallback) {
            request.CompletionCallback(request.RequestId, success);
        }

        {
            std::lock_guard<std::mutex> lock(completionMutex);
            completedRequests[request.RequestId] = success;
        }

        completionCondition.notify_all();
    }
}

bool DirectStorageStreaming::processRequestCpuFallback(const StreamRequest& request) {
    if (request.TargetDestination == nullptr || request.SourceFilePath.empty()) {
        return false;
    }

    std::ifstream file(request.SourceFilePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.seekg(static_cast<std::streamoff>(request.FileOffset), std::ios::beg);
    if (!file.good()) {
        return false;
    }

    if (request.Compression == CompressionType::None) {
        file.read(static_cast<char*>(request.TargetDestination), static_cast<std::streamsize>(request.UncompressedSize));
        return file.gcount() == static_cast<std::streamsize>(request.UncompressedSize);
    }

    std::vector<char> compressedBuffer(request.CompressedSize);
    file.read(compressedBuffer.data(), static_cast<std::streamsize>(request.CompressedSize));
    if (file.gcount() != static_cast<std::streamsize>(request.CompressedSize)) {
        return false;
    }

    size_t copyBytes = std::min(request.CompressedSize, request.UncompressedSize);
    std::memcpy(request.TargetDestination, compressedBuffer.data(), copyBytes);
    return true;
}

} // namespace Aot::Io
