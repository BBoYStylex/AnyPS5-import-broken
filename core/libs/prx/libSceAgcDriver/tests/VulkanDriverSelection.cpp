#include "prx/libSceAgcDriver/Execution/include/VulkanDriverSelection.hpp"
#include <array>
#include <cstdio>
#include <stdexcept>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

unsigned queries = 0;

VKAPI_ATTR void VKAPI_CALL QueryProperties(VkPhysicalDevice physical, VkPhysicalDeviceProperties2* properties) {
    Require(physical == VK_NULL_HANDLE, "unexpected physical device");
    Require(properties->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, "incorrect properties type");
    auto* driver = static_cast<VkPhysicalDeviceDriverProperties*>(properties->pNext);
    Require(driver != nullptr && driver->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES && driver->pNext == nullptr, "incorrect driver properties chain");
    ++queries;
    driver->driverID = VK_DRIVER_ID_MOLTENVK;
    std::strcpy(driver->driverName, "MoltenVK");
    std::strcpy(driver->driverInfo, "test version");
}

void CheckQueries() {
    VkExtensionProperties unrelated{};
    std::strcpy(unrelated.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    VkExtensionProperties supported{};
    std::strcpy(supported.extensionName, VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME);
    const std::array extensions{unrelated, supported};
    for (const auto apiVersion : {VK_API_VERSION_1_1, VK_API_VERSION_1_2, VK_API_VERSION_1_4}) {
        for (const bool advertised : {false, true}) {
            const std::span<const VkExtensionProperties> available(extensions.data(), advertised ? 2 : 1);
            const auto previous = queries;
            const auto driver = AgcDriver::QueryVulkanDriverProperties(VK_NULL_HANDLE, apiVersion, available, QueryProperties);
            const bool expected = advertised || apiVersion >= VK_API_VERSION_1_2;
            Require(queries == previous + (expected ? 1 : 0), "driver properties queried without support or omitted after core promotion");
            if (expected) {
                Require(driver.driverID == VK_DRIVER_ID_MOLTENVK && std::strcmp(driver.driverName, "MoltenVK") == 0 && std::strcmp(driver.driverInfo, "test version") == 0, "queried identity was lost");
            } else {
                Require(driver.driverName[0] == '\0' && driver.driverInfo[0] == '\0' && driver.driverID == 0, "unsupported identity was invented");
            }
        }
    }
    const auto driver = AgcDriver::QueryVulkanDriverProperties(VK_NULL_HANDLE, VK_API_VERSION_1_1, {}, nullptr);
    Require(driver.driverName[0] == '\0', "empty extension list invented a driver identity");
}

void CheckSelection() {
    struct Candidate {
        std::string_view device;
        std::string_view driver;
    };
    constexpr std::array candidates{
        Candidate{"Apple M1 Pro", "MoltenVK"},
        Candidate{"Apple M1 Pro", "KosmicKrisp"},
        Candidate{"Another GPU", "MoltenVK"},
        Candidate{"Apple M1 Pro", ""},
    };
    struct Request {
        std::string_view device;
        std::string_view driver;
        unsigned matches;
    };
    constexpr std::array requests{
        Request{"", "", 0b1111},
        Request{"M1 PRO", "", 0b1011},
        Request{"", "mOlTeNvK", 0b0101},
        Request{"apple", "MOLTEN", 0b0001},
        Request{"apple", "kosmic", 0b0010},
        Request{"another", "kosmic", 0},
        Request{"missing", "MoltenVK", 0},
        Request{"", "unavailable", 0},
        Request{"", "missing", 0},
        Request{"", "MoltenVK-extra", 0},
        Request{"", "Molten VK", 0},
    };
    for (const auto& request : requests) {
        unsigned matches = 0;
        for (unsigned index = 0; index < candidates.size(); ++index) {
            const auto& candidate = candidates[index];
            if (AgcDriver::VulkanDeviceMatchesRequest(candidate.device, candidate.driver, request.device, request.driver)) matches |= 1u << index;
        }
        Require(matches == request.matches, "GPU and driver filters selected the wrong candidates");
    }
    Require(AgcDriver::VulkanNameMatches("", ""), "empty request must accept an unavailable identity");
    Require(!AgcDriver::VulkanNameMatches("", "driver"), "explicit request must reject an unavailable identity");
}

}

int main() {
    try {
        CheckQueries();
        CheckSelection();
        std::puts("Vulkan driver selection tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
