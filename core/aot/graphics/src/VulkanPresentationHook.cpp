#include <graphics/VulkanPresentationHook.hpp>
#include <numeric>

namespace Aot::Graphics {

VulkanPresentationHook& VulkanPresentationHook::Instance() {
    static VulkanPresentationHook instance;
    return instance;
}

VulkanPresentationHook::VulkanPresentationHook()
    : lastFrameTimePoint(std::chrono::high_resolution_clock::now()) {}

VulkanPresentationHook::~VulkanPresentationHook() = default;

void VulkanPresentationHook::SetTargetPresentFunction(PfnVkQueuePresentKHR targetFunction) {
    std::lock_guard<std::mutex> lock(hookMutex);
    targetPresentFunction = targetFunction;
}

PfnVkQueuePresentKHR VulkanPresentationHook::GetTargetPresentFunction() const {
    std::lock_guard<std::mutex> lock(hookMutex);
    return targetPresentFunction;
}

VkResult VulkanPresentationHook::Present(VkQueue queue, const VkPresentInfoKHR* presentInfo) {
    VkResult result = VK_SUCCESS;
    PfnVkQueuePresentKHR target = nullptr;

    {
        std::lock_guard<std::mutex> lock(hookMutex);
        target = targetPresentFunction;
    }

    if (target != nullptr) {
        result = target(queue, presentInfo);
    }

    recordFrameMetrics(result, presentInfo);
    return result;
}

void VulkanPresentationHook::RegisterCallback(FramePresentationCallback callback) {
    std::lock_guard<std::mutex> lock(hookMutex);
    callbacks.push_back(std::move(callback));
}

void VulkanPresentationHook::ClearCallbacks() {
    std::lock_guard<std::mutex> lock(hookMutex);
    callbacks.clear();
}

void VulkanPresentationHook::RequestCaptureNextFrame() {
    captureRequested.store(true, std::memory_order_release);
}

bool VulkanPresentationHook::IsCaptureRequested() const {
    return captureRequested.load(std::memory_order_acquire);
}

uint64_t VulkanPresentationHook::GetTotalPresentedFrames() const {
    std::lock_guard<std::mutex> lock(hookMutex);
    return totalPresentedFrames;
}

double VulkanPresentationHook::GetCurrentFps() const {
    std::lock_guard<std::mutex> lock(hookMutex);
    return currentFps;
}

double VulkanPresentationHook::GetAverageFps() const {
    std::lock_guard<std::mutex> lock(hookMutex);
    if (recentFrameDurationsMs.empty()) {
        return 0.0;
    }
    double totalMs = std::accumulate(recentFrameDurationsMs.begin(), recentFrameDurationsMs.end(), 0.0);
    double avgMs = totalMs / static_cast<double>(recentFrameDurationsMs.size());
    return (avgMs > 0.0001) ? (1000.0 / avgMs) : 0.0;
}

double VulkanPresentationHook::GetLastFrameDurationMs() const {
    std::lock_guard<std::mutex> lock(hookMutex);
    return lastFrameDurationMs;
}

void VulkanPresentationHook::ResetMetrics() {
    std::lock_guard<std::mutex> lock(hookMutex);
    totalPresentedFrames = 0;
    lastFrameDurationMs = 0.0;
    currentFps = 0.0;
    recentFrameDurationsMs.clear();
    lastFrameTimePoint = std::chrono::high_resolution_clock::now();
    captureRequested.store(false, std::memory_order_release);
}

void VulkanPresentationHook::recordFrameMetrics(VkResult result, const VkPresentInfoKHR* presentInfo) {
    FramePresentationMetrics metrics{};
    std::vector<FramePresentationCallback> localCallbacks;

    {
        std::lock_guard<std::mutex> lock(hookMutex);
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = now - lastFrameTimePoint;
        lastFrameTimePoint = now;

        lastFrameDurationMs = elapsed.count();
        currentFps = (lastFrameDurationMs > 0.0001) ? (1000.0 / lastFrameDurationMs) : 0.0;

        recentFrameDurationsMs.push_back(lastFrameDurationMs);
        if (recentFrameDurationsMs.size() > MaxRecentFrameHistory) {
            recentFrameDurationsMs.pop_front();
        }

        ++totalPresentedFrames;

        metrics.FrameIndex = totalPresentedFrames;
        metrics.PresentResult = result;
        metrics.FrameDurationMs = lastFrameDurationMs;
        metrics.CurrentFps = currentFps;

        if (recentFrameDurationsMs.empty()) {
            metrics.AverageFps = 0.0;
        } else {
            double totalMs = std::accumulate(recentFrameDurationsMs.begin(), recentFrameDurationsMs.end(), 0.0);
            double avgMs = totalMs / static_cast<double>(recentFrameDurationsMs.size());
            metrics.AverageFps = (avgMs > 0.0001) ? (1000.0 / avgMs) : 0.0;
        }

        if (presentInfo != nullptr) {
            if (presentInfo->swapchainCount > 0 && presentInfo->pSwapchains != nullptr) {
                metrics.Swapchain = presentInfo->pSwapchains[0];
            }
            if (presentInfo->pImageIndices != nullptr) {
                metrics.ImageIndex = presentInfo->pImageIndices[0];
            }
        }

        metrics.CaptureRequested = captureRequested.exchange(false, std::memory_order_acq_rel);
        localCallbacks = callbacks;
    }

    for (const auto& callback : localCallbacks) {
        if (callback) {
            callback(metrics);
        }
    }
}

} // namespace Aot::Graphics
