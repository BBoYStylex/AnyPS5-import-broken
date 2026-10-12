#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <vector>

extern "C" {
int APS5_VABI sceVoiceQoSInit(void*, std::uint32_t, std::int32_t);
int APS5_VABI sceVoiceQoSEnd();
int APS5_VABI sceVoiceQoSCreateLocalEndpoint();
int APS5_VABI sceVoiceQoSDeleteLocalEndpoint();
int APS5_VABI sceVoiceQoSCreateRemoteEndpoint();
int APS5_VABI sceVoiceQoSDeleteRemoteEndpoint();
int APS5_VABI sceVoiceQoSConnect();
int APS5_VABI sceVoiceQoSDisconnect();
int APS5_VABI sceVoiceQoSSetLocalEndpointAttribute();
int APS5_VABI sceVoiceQoSSetRemoteEndpointAttribute();
int APS5_VABI sceVoiceQoSReadPacket();
int APS5_VABI sceVoiceQoSWritePacket();
int APS5_VABI sceVoiceQoSGetLocalEndpointAttribute();
}

static void Require(bool value) { if (!value) std::abort(); }

static void RequireUninitialized(int notInitialized) {
    Require(sceVoiceQoSEnd() == notInitialized);
    Require(sceVoiceQoSCreateLocalEndpoint() == notInitialized);
    Require(sceVoiceQoSDeleteLocalEndpoint() == notInitialized);
    Require(sceVoiceQoSCreateRemoteEndpoint() == notInitialized);
    Require(sceVoiceQoSDeleteRemoteEndpoint() == notInitialized);
    Require(sceVoiceQoSConnect() == notInitialized);
    Require(sceVoiceQoSDisconnect() == notInitialized);
    Require(sceVoiceQoSSetLocalEndpointAttribute() == notInitialized);
    Require(sceVoiceQoSSetRemoteEndpointAttribute() == notInitialized);
    Require(sceVoiceQoSReadPacket() == notInitialized);
    Require(sceVoiceQoSWritePacket() == notInitialized);
    Require(sceVoiceQoSGetLocalEndpointAttribute() == notInitialized);
}

int main(int argc, char** argv) {
    Require(argc == 2);
    constexpr int argumentInvalid = static_cast<int>(0x804E0902);
    constexpr int initialized = static_cast<int>(0x804E0905);
    constexpr int notInitialized = static_cast<int>(0x804E0901);
    const auto appType = static_cast<std::int32_t>(std::strtoul(argv[1], nullptr, 16));
    std::vector<std::uint8_t> memory(0x40000, 0xAA);
    RequireUninitialized(notInitialized);
    Require(sceVoiceQoSInit(nullptr, 0x40000, appType) == argumentInvalid);
    Require(sceVoiceQoSInit(nullptr, 0, appType) == argumentInvalid);
    for (std::uint32_t size : {0u, 1u, 0x100u, 0x3FFFFu, 0x40001u, 0x80000u, 0xFFFFFFFFu}) Require(sceVoiceQoSInit(memory.data(), size, appType) == argumentInvalid);
    for (std::int32_t type : {0, 1, 0x08000000, 0x20000001, 0x30000000, 0x40000000, static_cast<std::int32_t>(0x80000000), -1}) {
        Require(sceVoiceQoSInit(memory.data(), 0x40000, type) == argumentInvalid);
    }
    for (std::uint8_t byte : memory) Require(byte == 0xAA);
    Require(sceVoiceQoSInit(memory.data(), 0x40000, appType) == 0);
    Require(sceVoiceQoSInit(memory.data(), 0x40000, appType) == initialized);
    Require(sceVoiceQoSInit(nullptr, 0, 0) == initialized);
    Require(sceVoiceQoSEnd() == 0);
    RequireUninitialized(notInitialized);
    Require(sceVoiceQoSInit(nullptr, 0x40000, appType) == argumentInvalid);
    Require(sceVoiceQoSInit(memory.data(), 0x40000, appType) == 0);
    Require(sceVoiceQoSInit(memory.data(), 0x40000, appType) == initialized);
    Require(sceVoiceQoSEnd() == 0);
    RequireUninitialized(notInitialized);
}
