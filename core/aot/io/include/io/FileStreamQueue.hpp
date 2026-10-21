#ifndef CORE_AOT_IO_FILESTREAMQUEUE_HPP
#define CORE_AOT_IO_FILESTREAMQUEUE_HPP

#include <io/DirectStorageStreaming.hpp>
#include <vector>
#include <memory>
#include <mutex>

namespace Aot::Io {

enum class StreamPriority {
    High,
    Normal,
    Low
};

class FileStreamQueue {
public:
    FileStreamQueue();
    ~FileStreamQueue();

    uint64_t QueueAssetLoad(
        const std::string& filePath,
        uint64_t offset,
        size_t compressedSize,
        size_t uncompressedSize,
        void* destinationAddress,
        CompressionType compression = CompressionType::None,
        DestinationType destination = DestinationType::GpuDeviceMemory,
        StreamPriority priority = StreamPriority::Normal
    );

    void WaitForBatch(const std::vector<uint64_t>& requestIds);
    void Flush();

private:
    DirectStorageStreaming& streamingEngine;
    std::vector<uint64_t> activeRequestIds;
    std::mutex queueMutex;
};

} // namespace Aot::Io
#endif
