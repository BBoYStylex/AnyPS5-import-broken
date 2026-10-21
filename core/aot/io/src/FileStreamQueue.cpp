#include <io/FileStreamQueue.hpp>

namespace Aot::Io {

FileStreamQueue::FileStreamQueue()
    : streamingEngine(DirectStorageStreaming::Instance()) {
    streamingEngine.Initialize(true);
}

FileStreamQueue::~FileStreamQueue() {
    Flush();
}

uint64_t FileStreamQueue::QueueAssetLoad(
    const std::string& filePath,
    uint64_t offset,
    size_t compressedSize,
    size_t uncompressedSize,
    void* destinationAddress,
    CompressionType compression,
    DestinationType destination,
    StreamPriority priority) {
    static_cast<void>(priority);

    StreamRequest request{
        .RequestId = 0,
        .SourceFilePath = filePath,
        .FileOffset = offset,
        .CompressedSize = compressedSize,
        .UncompressedSize = uncompressedSize,
        .TargetDestination = destinationAddress,
        .Compression = compression,
        .Destination = destination,
        .CompletionCallback = nullptr
    };

    uint64_t requestId = streamingEngine.EnqueueRequest(std::move(request));

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        activeRequestIds.push_back(requestId);
    }

    return requestId;
}

void FileStreamQueue::WaitForBatch(const std::vector<uint64_t>& requestIds) {
    for (uint64_t requestId : requestIds) {
        streamingEngine.WaitForRequest(requestId);
    }
}

void FileStreamQueue::Flush() {
    std::vector<uint64_t> pendingWait;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        pendingWait = std::move(activeRequestIds);
        activeRequestIds.clear();
    }

    for (uint64_t requestId : pendingWait) {
        streamingEngine.WaitForRequest(requestId);
    }
}

} // namespace Aot::Io
