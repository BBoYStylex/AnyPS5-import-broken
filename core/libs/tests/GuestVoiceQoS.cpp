#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
int APS5_VABI sceVoiceQoSInit(void*, std::uint32_t, std::int32_t);
int APS5_VABI sceVoiceQoSEnd();
int APS5_VABI sceVoiceQoSCreateLocalEndpoint(std::int32_t*, std::int32_t, std::int32_t, std::int32_t);
int APS5_VABI sceVoiceQoSDeleteLocalEndpoint(std::int32_t);
int APS5_VABI sceVoiceQoSCreateRemoteEndpoint(std::int32_t*);
int APS5_VABI sceVoiceQoSDeleteRemoteEndpoint(std::int32_t);
int APS5_VABI sceVoiceQoSConnect(std::int32_t*, std::int32_t, std::int32_t);
int APS5_VABI sceVoiceQoSDisconnect(std::int32_t);
int APS5_VABI sceVoiceQoSSetLocalEndpointAttribute(std::int32_t, std::int32_t, const void*, std::int32_t);
int APS5_VABI sceVoiceQoSSetRemoteEndpointAttribute(std::int32_t, std::int32_t, const void*, std::int32_t);
int APS5_VABI sceVoiceQoSSetConnectionAttribute(std::int32_t, std::int32_t, const void*, std::int32_t);
int APS5_VABI sceVoiceQoSGetLocalEndpointAttribute(std::int32_t, std::int32_t, void*, std::int32_t);
int APS5_VABI sceVoiceQoSGetRemoteEndpointAttribute(std::int32_t, std::int32_t, void*, std::int32_t);
int APS5_VABI sceVoiceQoSGetConnectionAttribute(std::int32_t, std::int32_t, void*, std::int32_t);
int APS5_VABI sceVoiceQoSGetLocalEndpoint(std::int32_t, std::int32_t*);
int APS5_VABI sceVoiceQoSGetRemoteEndpoint(std::int32_t, std::int32_t*);
int APS5_VABI sceVoiceQoSEnableChat();
int APS5_VABI sceVoiceQoSDisableChat();
int APS5_VABI sceVoiceQoSGetStatus(std::int32_t, std::int32_t, void*, std::int32_t);
int APS5_VABI sceVoiceQoSDebugGetStatus();
int APS5_VABI sceVoiceQoSInitHQ();
int APS5_VABI sceVoiceQoSSetMode();
int APS5_VABI sceVoiceQoSTerminate();
int APS5_VABI sceVoiceQoSReadPacket(std::int32_t, void*, std::uint32_t*);
int APS5_VABI sceVoiceQoSWritePacket(std::int32_t, const void*, std::uint32_t*);
}

static void Require(bool value) { if (!value) std::abort(); }

template <typename TCall>
static bool NotImplemented(TCall call) {
    try {
        call();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

template <typename TCall>
static bool ReportsNotImplemented(const char* name, TCall call) {
    try {
        call();
    } catch (const std::runtime_error& error) {
        return std::string(error.what()) == std::string(name) + " not implemented";
    }
    return false;
}

int main(int argc, char** argv) {
    Require(argc == 2);
    constexpr int argumentInvalid = static_cast<int>(0x804E0902);
    constexpr int initialized = static_cast<int>(0x804E0905);
    constexpr int notInitialized = static_cast<int>(0x804E0901);
    const auto appType = static_cast<std::int32_t>(std::strtoul(argv[1], nullptr, 16));
    std::vector<std::uint8_t> memory(0x40000, 0xAA);
    Require(sceVoiceQoSEnd() == notInitialized);
    std::int32_t uninitLocal = -1;
    std::int32_t uninitRemote = -1;
    Require(sceVoiceQoSGetLocalEndpoint(1, &uninitLocal) == notInitialized);
    Require(sceVoiceQoSGetRemoteEndpoint(1, &uninitRemote) == notInitialized);
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

    std::int32_t local = -1;
    std::int32_t remote = -1;
    std::int32_t connection = -1;
    Require(sceVoiceQoSCreateLocalEndpoint(nullptr, 1, 0, 0) == argumentInvalid);
    Require(sceVoiceQoSCreateLocalEndpoint(&local, 1, 0, 0) == 0 && local > 0);
    Require(sceVoiceQoSCreateRemoteEndpoint(&remote) == 0 && remote > 0 && remote != local);
    Require(sceVoiceQoSConnect(&connection, remote, local) == argumentInvalid);
    Require(sceVoiceQoSConnect(&connection, local, remote) == 0 && connection > 0);
    std::int32_t queriedLocal = -1;
    std::int32_t queriedRemote = -1;
    Require(sceVoiceQoSGetLocalEndpoint(connection, nullptr) == argumentInvalid);
    Require(sceVoiceQoSGetRemoteEndpoint(connection, nullptr) == argumentInvalid);
    Require(sceVoiceQoSGetLocalEndpoint(connection + 100, &queriedLocal) == argumentInvalid);
    Require(sceVoiceQoSGetRemoteEndpoint(connection + 100, &queriedRemote) == argumentInvalid);
    Require(sceVoiceQoSGetLocalEndpoint(connection, &queriedLocal) == 0 && queriedLocal == local);
    Require(sceVoiceQoSGetRemoteEndpoint(connection, &queriedRemote) == 0 && queriedRemote == remote);
    std::int32_t duplicate = -1;
    Require(sceVoiceQoSConnect(&duplicate, local, remote) == 0 && duplicate > 0 && duplicate != connection);
    Require(sceVoiceQoSDisconnect(duplicate) == 0);

    std::int32_t otherLocal = -1;
    std::int32_t otherConnection = -1;
    Require(sceVoiceQoSCreateLocalEndpoint(&otherLocal, 1, 0, 0) == 0);
    Require(sceVoiceQoSConnect(&otherConnection, otherLocal, remote) == 0);
    Require(sceVoiceQoSDeleteLocalEndpoint(otherLocal) == 0);
    Require(sceVoiceQoSDeleteLocalEndpoint(otherLocal) == argumentInvalid);
    Require(sceVoiceQoSDisconnect(otherConnection) == 0);

    const std::uint32_t level = 0x1234;
    std::uint32_t readLevel = 0;
    Require(sceVoiceQoSGetLocalEndpointAttribute(local, 3, &readLevel, sizeof(readLevel)) == 0 && readLevel == 0);
    Require(sceVoiceQoSSetLocalEndpointAttribute(local, 3, &level, sizeof(level)) == 0);
    Require(sceVoiceQoSGetLocalEndpointAttribute(local, 3, &readLevel, sizeof(readLevel)) == 0 && readLevel == level);
    Require(sceVoiceQoSGetLocalEndpointAttribute(local, 3, nullptr, sizeof(readLevel)) == argumentInvalid);
    Require(sceVoiceQoSGetLocalEndpointAttribute(local, 3, &readLevel, 0) == argumentInvalid);
    Require(sceVoiceQoSGetLocalEndpointAttribute(local, 3, &readLevel, -1) == argumentInvalid);
    Require(sceVoiceQoSGetLocalEndpointAttribute(local + 100, 3, &readLevel, sizeof(readLevel)) == argumentInvalid);
    Require(sceVoiceQoSSetLocalEndpointAttribute(local, 3, nullptr, sizeof(level)) == argumentInvalid);
    Require(sceVoiceQoSSetLocalEndpointAttribute(local, 3, &level, 0) == 0);
    Require(sceVoiceQoSSetLocalEndpointAttribute(local, 3, &level, -1) == 0);
    Require(sceVoiceQoSSetLocalEndpointAttribute(local + 100, 3, &level, sizeof(level)) == argumentInvalid);

    Require(sceVoiceQoSSetRemoteEndpointAttribute(remote, 3, &level, sizeof(level)) == 0);
    Require(sceVoiceQoSSetRemoteEndpointAttribute(remote, 3, nullptr, sizeof(level)) == argumentInvalid);
    Require(sceVoiceQoSSetRemoteEndpointAttribute(remote, 3, &level, 0) == 0);
    Require(sceVoiceQoSSetRemoteEndpointAttribute(remote, 3, &level, -1) == 0);
    Require(sceVoiceQoSSetRemoteEndpointAttribute(remote + 100, 3, &level, sizeof(level)) == argumentInvalid);

    const std::uint32_t value = 0x12345678;
    std::uint32_t readRemote = 0xFFFFFFFF;
    Require(ReportsNotImplemented("sceVoiceQoSGetRemoteEndpointAttribute", [&] { sceVoiceQoSGetRemoteEndpointAttribute(remote, 3, &readRemote, sizeof(readRemote)); }));
    Require(ReportsNotImplemented("sceVoiceQoSGetRemoteEndpointAttribute", [&] { sceVoiceQoSGetRemoteEndpointAttribute(remote, 2, &readRemote, sizeof(readRemote)); }));
    Require(sceVoiceQoSGetRemoteEndpointAttribute(remote, 3, nullptr, sizeof(readRemote)) == argumentInvalid);
    Require(sceVoiceQoSGetRemoteEndpointAttribute(remote, 3, &readRemote, 0) == argumentInvalid);
    Require(sceVoiceQoSGetRemoteEndpointAttribute(remote, 3, &readRemote, -1) == argumentInvalid);
    Require(sceVoiceQoSGetRemoteEndpointAttribute(remote + 100, 3, &readRemote, sizeof(readRemote)) == argumentInvalid);
    Require(readRemote == 0xFFFFFFFF);
    Require(sceVoiceQoSSetRemoteEndpointAttribute(remote, 3, &value, sizeof(value)) == 0);
    Require(sceVoiceQoSGetRemoteEndpointAttribute(remote, 3, &readRemote, sizeof(readRemote)) == 0 && readRemote == value);

    std::uint32_t readConnection = 0xFFFFFFFF;
    std::uint64_t wide = ~0ull;
    Require(ReportsNotImplemented("sceVoiceQoSGetConnectionAttribute", [&] { sceVoiceQoSGetConnectionAttribute(connection, 4, &readConnection, sizeof(readConnection)); }));
    Require(sceVoiceQoSGetConnectionAttribute(connection, 4, nullptr, sizeof(readConnection)) == argumentInvalid);
    Require(sceVoiceQoSGetConnectionAttribute(connection, 4, &readConnection, 0) == argumentInvalid);
    Require(sceVoiceQoSGetConnectionAttribute(connection, 4, &readConnection, -1) == argumentInvalid);
    Require(sceVoiceQoSGetConnectionAttribute(connection + 100, 4, &readConnection, sizeof(readConnection)) == argumentInvalid);
    Require(sceVoiceQoSSetConnectionAttribute(connection, 4, nullptr, sizeof(value)) == argumentInvalid);
    Require(sceVoiceQoSSetConnectionAttribute(connection + 100, 4, &value, sizeof(value)) == argumentInvalid);
    Require(readConnection == 0xFFFFFFFF);
    Require(sceVoiceQoSSetConnectionAttribute(connection, 4, &value, sizeof(value)) == 0);
    Require(sceVoiceQoSGetConnectionAttribute(connection, 4, &wide, 2) == 0 && wide == 0xFFFFFFFFFFFF5678ull);
    Require(sceVoiceQoSGetConnectionAttribute(connection, 4, &wide, sizeof(wide)) == 0 && wide == value);
    std::int32_t second = -1;
    Require(sceVoiceQoSConnect(&second, local, remote) == 0);
    Require(ReportsNotImplemented("sceVoiceQoSGetConnectionAttribute", [&] { sceVoiceQoSGetConnectionAttribute(second, 4, &readConnection, sizeof(readConnection)); }));
    Require(sceVoiceQoSSetConnectionAttribute(second, 4, &level, sizeof(level)) == 0);
    Require(sceVoiceQoSGetConnectionAttribute(second, 4, &readConnection, sizeof(readConnection)) == 0 && readConnection == level);
    Require(sceVoiceQoSGetConnectionAttribute(connection, 4, &readConnection, sizeof(readConnection)) == 0 && readConnection == value);
    Require(sceVoiceQoSDisconnect(second) == 0);
    readConnection = 0xFFFFFFFF;
    Require(sceVoiceQoSSetConnectionAttribute(second, 4, &level, sizeof(level)) == argumentInvalid);
    Require(sceVoiceQoSGetConnectionAttribute(second, 4, &readConnection, sizeof(readConnection)) == argumentInvalid);
    Require(sceVoiceQoSSetConnectionAttribute(connection, 4, &value, 0) == 0);
    Require(ReportsNotImplemented("sceVoiceQoSGetConnectionAttribute", [&] { sceVoiceQoSGetConnectionAttribute(connection, 4, &readConnection, sizeof(readConnection)); }));
    Require(readConnection == 0xFFFFFFFF);
    Require(sceVoiceQoSSetConnectionAttribute(connection, 4, &value, -1) == 0);

    std::vector<std::uint8_t> packet(1024, 0);
    std::uint32_t packetSize = static_cast<std::uint32_t>(packet.size());
    Require(sceVoiceQoSReadPacket(connection, packet.data(), &packetSize) == 0 && packetSize == 0);
    Require(sceVoiceQoSReadPacket(connection, nullptr, &packetSize) == argumentInvalid);
    Require(sceVoiceQoSReadPacket(connection, packet.data(), nullptr) == argumentInvalid);
    Require(sceVoiceQoSReadPacket(connection + 100, packet.data(), &packetSize) == argumentInvalid);

    packetSize = 100;
    Require(sceVoiceQoSWritePacket(connection, packet.data(), &packetSize) == 0);
    Require(sceVoiceQoSWritePacket(connection, nullptr, &packetSize) == argumentInvalid);
    Require(sceVoiceQoSWritePacket(connection, packet.data(), nullptr) == argumentInvalid);
    Require(sceVoiceQoSWritePacket(connection + 100, packet.data(), &packetSize) == argumentInvalid);

    Require(sceVoiceQoSDisconnect(connection) == 0);
    Require(sceVoiceQoSDisconnect(connection) == argumentInvalid);
    Require(sceVoiceQoSGetLocalEndpoint(connection, &queriedLocal) == argumentInvalid);
    Require(sceVoiceQoSGetRemoteEndpoint(connection, &queriedRemote) == argumentInvalid);
    Require(sceVoiceQoSDeleteLocalEndpoint(local) == 0);
    Require(sceVoiceQoSDeleteLocalEndpoint(local) == argumentInvalid);
    Require(sceVoiceQoSDeleteRemoteEndpoint(remote) == 0);
    Require(sceVoiceQoSGetRemoteEndpointAttribute(remote, 3, &readRemote, sizeof(readRemote)) == argumentInvalid);

    Require(sceVoiceQoSCreateRemoteEndpoint(&remote) == 0);
    Require(sceVoiceQoSCreateLocalEndpoint(&local, 1, 0, 0) == 0);
    Require(sceVoiceQoSConnect(&connection, local, remote) == 0);
    Require(sceVoiceQoSSetConnectionAttribute(connection, 4, &value, sizeof(value)) == 0);
    Require(sceVoiceQoSEnd() == 0);
    Require(sceVoiceQoSEnd() == notInitialized);
    Require(sceVoiceQoSGetLocalEndpoint(connection, &queriedLocal) == notInitialized);
    Require(sceVoiceQoSGetRemoteEndpoint(connection, &queriedRemote) == notInitialized);
    Require(sceVoiceQoSDeleteRemoteEndpoint(remote) == notInitialized);
    Require(sceVoiceQoSCreateLocalEndpoint(&local, 1, 0, 0) == notInitialized);
    Require(sceVoiceQoSSetRemoteEndpointAttribute(remote, 3, &level, sizeof(level)) == notInitialized);
    Require(sceVoiceQoSGetLocalEndpointAttribute(local, 3, &readLevel, sizeof(readLevel)) == notInitialized);
    readRemote = 0xFFFFFFFF;
    readConnection = 0xFFFFFFFF;
    Require(sceVoiceQoSGetRemoteEndpointAttribute(remote, 3, &readRemote, sizeof(readRemote)) == notInitialized);
    Require(sceVoiceQoSSetConnectionAttribute(connection, 4, &value, sizeof(value)) == notInitialized);
    Require(sceVoiceQoSGetConnectionAttribute(connection, 4, &readConnection, sizeof(readConnection)) == notInitialized);
    Require(sceVoiceQoSReadPacket(connection, packet.data(), &packetSize) == notInitialized);
    Require(sceVoiceQoSWritePacket(connection, packet.data(), &packetSize) == notInitialized);
    Require(sceVoiceQoSInit(memory.data(), 0x40000, appType) == 0);
    Require(sceVoiceQoSDeleteRemoteEndpoint(remote) == argumentInvalid);
    Require(sceVoiceQoSGetConnectionAttribute(connection, 4, &readConnection, sizeof(readConnection)) == argumentInvalid);
    Require(readRemote == 0xFFFFFFFF && readConnection == 0xFFFFFFFF);
    Require(sceVoiceQoSEnd() == 0);
    Require(sceVoiceQoSEnd() == notInitialized);
    Require(sceVoiceQoSInit(nullptr, 0x40000, appType) == argumentInvalid);

    Require(NotImplemented([] { sceVoiceQoSEnableChat(); }));
    Require(NotImplemented([] { sceVoiceQoSDisableChat(); }));
    Require(NotImplemented([] { sceVoiceQoSGetStatus(1, 1, nullptr, 0); }));
    Require(NotImplemented([] { sceVoiceQoSDebugGetStatus(); }));
    Require(NotImplemented([] { sceVoiceQoSInitHQ(); }));
    Require(NotImplemented([] { sceVoiceQoSSetMode(); }));
    Require(NotImplemented([] { sceVoiceQoSTerminate(); }));
}
