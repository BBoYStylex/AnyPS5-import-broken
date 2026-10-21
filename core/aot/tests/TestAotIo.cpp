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

    streaming.Shutdown();

    Aot::Io::FileStreamQueue queue;
    queue.Flush();

    return 0;
}
