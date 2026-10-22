#ifndef CORE_AOT_GRAPHICS_VULKANPRESENTATIONHOOK_HPP
#define CORE_AOT_GRAPHICS_VULKANPRESENTATIONHOOK_HPP

#include <vulkan/vulkan.h>
#include <cstdint>
#include <chrono>
#include <functional>
#include <mutex>
#include <atomic>
#include <vector>
#include <deque>

namespace Aot::Graphics {

struct FramePresentationMetrics {
    uint64_t FrameIndex;
    uint32_t ImageIndex;
    VkSwapchainKHR Swapchain;
    VkResult PresentResult;
    double FrameDurationMs;
    double CurrentFps;
    double AverageFps;
    bool CaptureRequested;
};

using FramePresentationCallback = std::function<void(const FramePresentationMetrics&)>;
using PfnVkQueuePresentKHR = VkResult (*)(VkQueue, const VkPresentInfoKHR*);

class VulkanPresentationHook {
public:
    static VulkanPresentationHook& Instance();

    VulkanPresentationHook();
    ~VulkanPresentationHook();

    VulkanPresentationHook(const VulkanPresentationHook&) = delete;
    VulkanPresentationHook& operator=(const VulkanPresentationHook&) = delete;

    void SetTargetPresentFunction(PfnVkQueuePresentKHR targetFunction);
    [[nodiscard]] PfnVkQueuePresentKHR GetTargetPresentFunction() const;

    VkResult Present(VkQueue queue, const VkPresentInfoKHR* presentInfo);

    void RegisterCallback(FramePresentationCallback callback);
    void ClearCallbacks();

    void RequestCaptureNextFrame();
    [[nodiscard]] bool IsCaptureRequested() const;

    [[nodiscard]] uint64_t GetTotalPresentedFrames() const;
    [[nodiscard]] double GetCurrentFps() const;
    [[nodiscard]] double GetAverageFps() const;
    [[nodiscard]] double GetLastFrameDurationMs() const;

    void ResetMetrics();

private:
    void recordFrameMetrics(VkResult result, const VkPresentInfoKHR* presentInfo);

    mutable std::mutex hookMutex;
    PfnVkQueuePresentKHR targetPresentFunction = nullptr;
    std::vector<FramePresentationCallback> callbacks;
    std::atomic<bool> captureRequested{false};

    uint64_t totalPresentedFrames = 0;
    std::chrono::high_resolution_clock::time_point lastFrameTimePoint;
    double lastFrameDurationMs = 0.0;
    double currentFps = 0.0;
    std::deque<double> recentFrameDurationsMs;
    static constexpr size_t MaxRecentFrameHistory = 60;
};

} // namespace Aot::Graphics
#endif
