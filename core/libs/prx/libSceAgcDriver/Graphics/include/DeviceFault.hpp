#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_DEVICEFAULT_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_DEVICEFAULT_HPP

#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace AgcDriver::Graphics {

class DeviceFault {
public:
    static constexpr std::uint32_t MaxEntries = 256;

    DeviceFault(VkDevice device, PFN_vkGetDeviceFaultInfoEXT query) : device(device), query(query) {}

    void Report(VkResult result, const char* operation, FILE* output = stderr) const noexcept {
        if (result != VK_ERROR_DEVICE_LOST || query == nullptr || reported.exchange(true)) return;
        try {
            write(operation, output);
        } catch (...) {
            std::fprintf(output, "[gpu-fault] device lost at %s; fault information could not be collected\n", operation);
        }
        std::fflush(output);
    }

private:
    void write(const char* operation, FILE* output) const {
        VkDeviceFaultCountsEXT counts{VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT};
        auto result = query(device, &counts, nullptr);
        if (result != VK_SUCCESS) {
            std::fprintf(output, "[gpu-fault] device lost at %s; fault count query failed: Vulkan result %d\n", operation, static_cast<int>(result));
            return;
        }
        const auto availableAddresses = counts.addressInfoCount;
        const auto availableVendors = counts.vendorInfoCount;
        std::vector<VkDeviceFaultAddressInfoEXT> addresses(std::min(availableAddresses, MaxEntries));
        std::vector<VkDeviceFaultVendorInfoEXT> vendors(std::min(availableVendors, MaxEntries));
        counts.addressInfoCount = static_cast<std::uint32_t>(addresses.size());
        counts.vendorInfoCount = static_cast<std::uint32_t>(vendors.size());
        counts.vendorBinarySize = 0;
        VkDeviceFaultInfoEXT info{VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT};
        info.pAddressInfos = addresses.empty() ? nullptr : addresses.data();
        info.pVendorInfos = vendors.empty() ? nullptr : vendors.data();
        result = query(device, &counts, &info);
        if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
            std::fprintf(output, "[gpu-fault] device lost at %s; fault data query failed: Vulkan result %d\n", operation, static_cast<int>(result));
            return;
        }
        const auto addressCount = std::min(counts.addressInfoCount, static_cast<std::uint32_t>(addresses.size()));
        const auto vendorCount = std::min(counts.vendorInfoCount, static_cast<std::uint32_t>(vendors.size()));
        const bool partial = result == VK_INCOMPLETE || addressCount < availableAddresses || vendorCount < availableVendors;
        const bool described = info.description[0] != '\0';
        std::fprintf(output, "[gpu-fault] device lost at %s; %.*s; addresses %u/%u, vendor entries %u/%u%s\n", operation, static_cast<int>(VK_MAX_DESCRIPTION_SIZE), described ? info.description : "no driver description", addressCount, availableAddresses, vendorCount, availableVendors, partial ? " (partial)" : "");
        for (std::uint32_t i = 0; i < addressCount; ++i) {
            const auto& address = addresses[i];
            std::fprintf(output, "[gpu-fault] address %s 0x%llx, precision 0x%llx\n", addressType(address.addressType), static_cast<unsigned long long>(address.reportedAddress), static_cast<unsigned long long>(address.addressPrecision));
        }
        for (std::uint32_t i = 0; i < vendorCount; ++i) {
            const auto& vendor = vendors[i];
            std::fprintf(output, "[gpu-fault] vendor %.*s, code 0x%llx, data 0x%llx\n", static_cast<int>(VK_MAX_DESCRIPTION_SIZE), vendor.description, static_cast<unsigned long long>(vendor.vendorFaultCode), static_cast<unsigned long long>(vendor.vendorFaultData));
        }
    }

    static const char* addressType(VkDeviceFaultAddressTypeEXT type) {
        switch (type) {
        case VK_DEVICE_FAULT_ADDRESS_TYPE_NONE_EXT: return "none";
        case VK_DEVICE_FAULT_ADDRESS_TYPE_READ_INVALID_EXT: return "read-invalid";
        case VK_DEVICE_FAULT_ADDRESS_TYPE_WRITE_INVALID_EXT: return "write-invalid";
        case VK_DEVICE_FAULT_ADDRESS_TYPE_EXECUTE_INVALID_EXT: return "execute-invalid";
        case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_UNKNOWN_EXT: return "instruction-pointer-unknown";
        case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_INVALID_EXT: return "instruction-pointer-invalid";
        case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_FAULT_EXT: return "instruction-pointer-fault";
        default: return "unknown";
        }
    }

    VkDevice device;
    PFN_vkGetDeviceFaultInfoEXT query;
    mutable std::atomic<bool> reported{false};
};

}

#endif
