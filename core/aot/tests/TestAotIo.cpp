#include <io/DirectStorageStreaming.hpp>
#include <io/FileStreamQueue.hpp>
#include <cassert>
#include <vector>

int main() {
    auto& streaming = Aot::Io::DirectStorageStreaming::Instance();
    bool initOk = streaming.Initialize(false);
    assert(initOk);

    std::vector<char> destBuffer(128, 0);
    Aot::Io::StreamRequest req{
        .RequestId = 0,
        .SourceFilePath = "",
        .FileOffset = 0,
        .CompressedSize = 0,
        .UncompressedSize = 0,
        .TargetDestination = destBuffer.data(),
        .Compression = Aot::Io::CompressionType::None,
        .Destination = Aot::Io::DestinationType::HostMemory,
        .CompletionCallback = nullptr
    };

    uint64_t reqId = streaming.EnqueueRequest(std::move(req));
    assert(reqId > 0);
    streaming.WaitForRequest(reqId);

    Aot::Io::ExternalGpuMemoryResource gpuResource{
        .ResourceId = 42,
        .SharedHandle = reinterpret_cast<void*>(0x1234),
        .MappedHostAddress = destBuffer.data(),
        .SizeBytes = destBuffer.size(),
        .IsGpuExclusive = false
    };
    bool regOk = streaming.RegisterExternalGpuMemory(gpuResource);
    assert(regOk);
    static_cast<void>(regOk);
    bool hasRes = streaming.HasExternalGpuMemory(42);
    assert(hasRes);
    static_cast<void>(hasRes);

    uint64_t gpuReqId = streaming.StreamToGpuResource(42, 0, "", 0, 32);
    assert(gpuReqId > 0);
    streaming.WaitForRequest(gpuReqId);
    streaming.UnregisterExternalGpuMemory(42);
    bool stillHasRes = streaming.HasExternalGpuMemory(42);
    assert(!stillHasRes);
    static_cast<void>(stillHasRes);

    bool completed = streaming.IsRequestCompleted(reqId);
    assert(completed);

    auto stats = streaming.GetStatistics();
    assert(stats.TotalRequests >= 1);

    Aot::Io::FileStreamQueue queue;
    assert(queue.GetActiveRequestCount() == 0);
    uint64_t queuedId = queue.QueueAssetLoad("", 0, 0, 0, destBuffer.data());
    assert(queuedId > 0);
    queue.Flush();
    assert(queue.GetActiveRequestCount() == 0);

    streaming.Shutdown();

    return 0;
}
