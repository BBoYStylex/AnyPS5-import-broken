#include "prx/libSceAgcDriver/Execution/include/VulkanDevice.hpp"
#include "VulkanTestDevice.hpp"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void SetRequest(const char* variable, const std::string& value) {
#ifdef _WIN32
    if (_putenv_s(variable, value.c_str()) != 0) throw std::runtime_error(std::string("cannot set ") + variable);
#else
    if (setenv(variable, value.c_str(), 1) != 0) throw std::runtime_error(std::string("cannot set ") + variable);
#endif
}

void Require(const bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

}

int main() {
    try {
        SetRequest("ANYPS5_GPU", "");
        SetRequest("ANYPS5_VULKAN_DRIVER", "");
        const auto defaultDevice = OpenVulkanTestDevice();
        if (!defaultDevice) return VulkanTestSkipped;
        const std::string name = defaultDevice->DeviceName();
        Require(!name.empty(), "the default device has no name");

        std::string request = name;
        for (auto& character : request) character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
        SetRequest("ANYPS5_GPU", request);
        const AgcDriver::VulkanDevice requested;
        Require(requested.DeviceName() == name, "ANYPS5_GPU=" + request + " selected " + requested.DeviceName() + " instead of " + name);

        SetRequest("ANYPS5_VULKAN_DRIVER", "no such driver 1d3d154f");
        try {
            const AgcDriver::VulkanDevice missing;
            throw std::runtime_error("ANYPS5_VULKAN_DRIVER naming no driver selected " + missing.DeviceName());
        } catch (const std::runtime_error& error) {
            const std::string message = error.what();
            Require(message.find("ANYPS5_VULKAN_DRIVER=\"no such driver 1d3d154f\"") != std::string::npos && message.find("ANYPS5_GPU=\"" + request + "\"") != std::string::npos && message.find(name + " [driver=") != std::string::npos, "unexpected error: " + message);
        }
        SetRequest("ANYPS5_VULKAN_DRIVER", "");
        SetRequest("ANYPS5_GPU", "no such device 1d3d154f");
        try {
            const AgcDriver::VulkanDevice missing;
            throw std::runtime_error("ANYPS5_GPU naming no device selected " + missing.DeviceName());
        } catch (const std::runtime_error& error) {
            const std::string message = error.what();
            Require(message.find("ANYPS5_GPU=\"no such device 1d3d154f\"") != std::string::npos && message.find(name) != std::string::npos, "unexpected error: " + message);
        }
        std::puts("device selection tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
