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
    assert(streaming.RegisterExternalGpuMemory(gpuResource));
    assert(streaming.HasExternalGpuMemory(42));
    uint64_t gpuReqId = streaming.StreamToGpuResource(42, 0, "", 0, 32);
    assert(gpuReqId > 0);
    streaming.WaitForRequest(gpuReqId);
    streaming.UnregisterExternalGpuMemory(42);
    assert(!streaming.HasExternalGpuMemory(42));

    streaming.Shutdown();

    Aot::Io::FileStreamQueue queue;
    queue.Flush();

    return 0;
}
