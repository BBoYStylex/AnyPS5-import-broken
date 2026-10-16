#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_VULKANDRIVERSELECTION_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_VULKANDRIVERSELECTION_HPP

#include <vulkan/vulkan.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

namespace AgcDriver {

inline bool VulkanNameMatches(std::string_view name, std::string_view request) {
    return request.empty() || std::search(name.begin(), name.end(), request.begin(), request.end(), [](unsigned char left, unsigned char right) {
        return std::tolower(left) == std::tolower(right);
    }) != name.end();
}

inline bool VulkanDeviceMatchesRequest(std::string_view deviceName, std::string_view driverName, std::string_view requestedDevice, std::string_view requestedDriver) {
    return VulkanNameMatches(deviceName, requestedDevice) && VulkanNameMatches(driverName, requestedDriver);
}

inline VkPhysicalDeviceDriverProperties QueryVulkanDriverProperties(VkPhysicalDevice physical, std::uint32_t apiVersion, std::span<const VkExtensionProperties> extensions, PFN_vkGetPhysicalDeviceProperties2 query) {
    VkPhysicalDeviceDriverProperties driver{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES};
    if (apiVersion >= VK_API_VERSION_1_2 || std::any_of(extensions.begin(), extensions.end(), [](const auto& extension) {
        return std::strcmp(extension.extensionName, VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME) == 0;
    })) {
        VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &driver};
        query(physical, &properties);
    }
    return driver;
}

}

#endif
