#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

extern "C" {
int APS5_VABI sceAudioOutExClose();
int APS5_VABI sceAudioOutExConfigureOutput(int, const void*, std::uint32_t);
int APS5_VABI sceAudioOutExGetMonitorInfo(int, void*, std::uint32_t);
int APS5_VABI sceAudioOutExOpen(int, int, int, std::uint32_t, std::uint32_t, std::uint32_t);
}

using ExCloseFunction = int (APS5_VABI *)();
using ExConfigFunction = int (APS5_VABI *)(int, const void*, std::uint32_t);
using ExMonitorFunction = int (APS5_VABI *)(int, void*, std::uint32_t);
using ExOpenFunction = int (APS5_VABI *)(int, int, int, std::uint32_t, std::uint32_t, std::uint32_t);

static bool ReportsNotImplementedClose(ExCloseFunction function, const char* name) {
    try {
        function();
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()) == std::string(name) + " not implemented") return true;
        std::fprintf(stderr, "%s threw: %s\n", name, error.what());
        return false;
    }
    std::fprintf(stderr, "%s returned instead of reporting that it is not implemented\n", name);
    return false;
}

static bool ReportsNotImplementedConfig(ExConfigFunction function, const char* name) {
    try {
        function(0, nullptr, 0);
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()) == std::string(name) + " not implemented") return true;
        std::fprintf(stderr, "%s threw: %s\n", name, error.what());
        return false;
    }
    std::fprintf(stderr, "%s returned instead of reporting that it is not implemented\n", name);
    return false;
}

static bool ReportsNotImplementedMonitor(ExMonitorFunction function, const char* name) {
    try {
        std::uint8_t buffer[256] = {};
        function(0, buffer, sizeof(buffer));
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()) == std::string(name) + " not implemented") return true;
        std::fprintf(stderr, "%s threw: %s\n", name, error.what());
        return false;
    }
    std::fprintf(stderr, "%s returned instead of reporting that it is not implemented\n", name);
    return false;
}

static bool ReportsNotImplementedOpen(ExOpenFunction function, const char* name) {
    try {
        function(0, 0, 0, 0, 0, 0);
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()) == std::string(name) + " not implemented") return true;
        std::fprintf(stderr, "%s threw: %s\n", name, error.what());
        return false;
    }
    std::fprintf(stderr, "%s returned instead of reporting that it is not implemented\n", name);
    return false;
}

int main() {
    bool passed = ReportsNotImplementedClose(sceAudioOutExClose, "sceAudioOutExClose");
    passed = ReportsNotImplementedConfig(sceAudioOutExConfigureOutput, "sceAudioOutExConfigureOutput") && passed;
    passed = ReportsNotImplementedMonitor(sceAudioOutExGetMonitorInfo, "sceAudioOutExGetMonitorInfo") && passed;
    passed = ReportsNotImplementedOpen(sceAudioOutExOpen, "sceAudioOutExOpen") && passed;
    return passed ? 0 : 1;
}
