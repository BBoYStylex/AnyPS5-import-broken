#include "prx/libSceAgcDriver/Graphics/include/Context.hpp"
#include "prx/libSceAgcDriver/Graphics/include/DeviceFault.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

enum class Mode { Normal, Truncated, Empty, Undescribed, CountFailure, DataFailure, Exception };

Mode mode = Mode::Normal;
std::uint32_t queries = 0;
const auto faultDevice = reinterpret_cast<VkDevice>(std::uintptr_t{0x1234});

VKAPI_ATTR VkResult VKAPI_CALL mockFaultInfo(VkDevice device, VkDeviceFaultCountsEXT* counts, VkDeviceFaultInfoEXT* info) {
    ++queries;
    check(device == faultDevice && counts->sType == VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT && counts->pNext == nullptr, "the fault query keeps the device and initializes the counts");
    if (mode == Mode::Exception) throw std::runtime_error("mock fault query failure");
    if (info == nullptr) {
        check(queries == 1, "the counts are queried first and once");
        if (mode == Mode::CountFailure) return VK_ERROR_UNKNOWN;
        counts->addressInfoCount = counts->vendorInfoCount = mode == Mode::Truncated ? 400 : mode == Mode::Empty ? 0 : 2;
        counts->vendorBinarySize = 999;
        return VK_SUCCESS;
    }
    check(queries == 2 && info->sType == VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT && info->pNext == nullptr, "the data query follows the counts query");
    check(counts->vendorBinarySize == 0 && info->pVendorBinaryData == nullptr, "no vendor binary is requested");
    const auto expected = mode == Mode::Truncated ? AgcDriver::Graphics::DeviceFault::MaxEntries : mode == Mode::Empty ? 0u : 2u;
    check(counts->addressInfoCount == expected && counts->vendorInfoCount == expected, "the arrays are sized from the bounded counts");
    check((info->pAddressInfos != nullptr) == (expected != 0) && (info->pVendorInfos != nullptr) == (expected != 0), "empty arrays are passed as null pointers");
    if (mode == Mode::DataFailure) return VK_ERROR_OUT_OF_HOST_MEMORY;
    if (mode != Mode::Undescribed) std::snprintf(info->description, sizeof(info->description), "mock device fault");
    if (expected != 0) {
        info->pAddressInfos[0] = {VK_DEVICE_FAULT_ADDRESS_TYPE_WRITE_INVALID_EXT, 0x7f3660000, 0x1000};
        auto& vendor = info->pVendorInfos[0];
        std::snprintf(vendor.description, sizeof(vendor.description), "mock vendor fault");
        vendor.vendorFaultCode = 0xab;
        vendor.vendorFaultData = 0xcd;
    }
    counts->addressInfoCount = counts->vendorInfoCount = expected != 0 ? 1 : 0;
    return mode == Mode::Truncated ? VK_INCOMPLETE : VK_SUCCESS;
}

std::string capture(const AgcDriver::Graphics::DeviceFault& fault, VkResult result, const char* operation) {
    const auto output = std::unique_ptr<FILE, decltype(&std::fclose)>(std::tmpfile(), std::fclose);
    check(output != nullptr, "temporary report file");
    fault.Report(result, operation, output.get());
    std::rewind(output.get());
    std::array<char, 8192> bytes{};
    return std::string(bytes.data(), std::fread(bytes.data(), 1, bytes.size(), output.get()));
}

void testReports() {
    for (const auto current : {Mode::Normal, Mode::Truncated, Mode::Empty, Mode::Undescribed, Mode::CountFailure, Mode::DataFailure, Mode::Exception}) {
        mode = current;
        queries = 0;
        const AgcDriver::Graphics::DeviceFault fault(faultDevice, mockFaultInfo);
        check(capture(fault, VK_SUCCESS, "success").empty() && capture(fault, VK_TIMEOUT, "timeout").empty() && queries == 0, "only a lost device is queried");
        const auto text = capture(fault, VK_ERROR_DEVICE_LOST, "vkQueueSubmit recorder");
        check(capture(fault, VK_ERROR_DEVICE_LOST, "vkWaitForFences recorder").empty(), "a later loss is not reported again");
        const auto expectedQueries = current == Mode::CountFailure || current == Mode::Exception ? 1u : 2u;
        check(queries == expectedQueries, "one report makes at most two queries");
        check(text.find("[gpu-fault] device lost at vkQueueSubmit recorder;") != std::string::npos, "the report names the failed operation");
        if (current == Mode::CountFailure) {
            check(text.find("fault count query failed: Vulkan result -13") != std::string::npos, "a failed count query is reported");
        } else if (current == Mode::DataFailure) {
            check(text.find("fault data query failed: Vulkan result -1") != std::string::npos, "a failed data query is reported");
        } else if (current == Mode::Exception) {
            check(text.find("fault information could not be collected") != std::string::npos, "an exception from the query is contained");
        } else if (current == Mode::Undescribed) {
            check(text.find("vkQueueSubmit recorder; no driver description; addresses 1/2") != std::string::npos, "an empty driver description is named");
        } else if (current == Mode::Empty) {
            check(text.find("mock device fault; addresses 0/0, vendor entries 0/0\n") != std::string::npos && text.find("[gpu-fault] address") == std::string::npos, "an empty report lists no entries");
        } else {
            check(text.find("[gpu-fault] address write-invalid 0x7f3660000, precision 0x1000") != std::string::npos, "the address entry is reported");
            check(text.find("[gpu-fault] vendor mock vendor fault, code 0xab, data 0xcd") != std::string::npos, "the vendor entry is reported");
            check(text.find(current == Mode::Truncated ? "addresses 1/400, vendor entries 1/400 (partial)" : "addresses 1/2, vendor entries 1/2 (partial)") != std::string::npos, "returned and available counts are both reported");
            check(text.find("address none") == std::string::npos && text.find("code 0x0,") == std::string::npos, "only the returned entries are printed");
        }
    }
}

void testCheckResult() {
    mode = Mode::Normal;
    queries = 0;
    const AgcDriver::Graphics::DeviceFault fault(faultDevice, mockFaultInfo);
    AgcDriver::Graphics::Context context{};
    context.deviceFault = &fault;
    context.CheckResult(VK_SUCCESS, "success");
    check(queries == 0, "a successful result is not reported");
    bool threw = false;
    try {
        context.CheckResult(VK_ERROR_DEVICE_LOST, "vkWaitForFences graphics");
    } catch (const std::runtime_error& error) {
        threw = std::string_view(error.what()).find("vkWaitForFences graphics: Vulkan result -4") != std::string_view::npos;
    }
    check(threw && queries == 2, "the device loss is reported and still thrown");
    context.deviceFault = nullptr;
    threw = false;
    try {
        context.CheckResult(VK_ERROR_DEVICE_LOST, "vkQueueSubmit graphics");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw && queries == 2, "without the extension a device loss is thrown unchanged");
}

}

int main() {
    try {
        testReports();
        testCheckResult();
        std::puts("Device fault reports passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
