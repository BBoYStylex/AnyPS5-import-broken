#include "prx/libc/include/GuestArena.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

extern "C" {
std::int32_t APS5_VABI sceRegMgrGetInt(std::int32_t optionId, std::int32_t* outInt);
std::int32_t APS5_VABI sceRegMgrSetInt(std::int32_t optionId, std::int32_t inInt);
std::int32_t APS5_VABI sceRegMgrGetStr(std::int32_t optionId, char* outString, std::size_t stringLength);
std::int32_t APS5_VABI sceRegMgrSetStr(std::int32_t optionId, const char* inString, std::size_t stringLength);
std::int32_t APS5_VABI sceRegMgrNonSysGetInt(std::int64_t key, std::int32_t unk, std::int32_t* data);
}

static void Require(bool value) { if (!value) std::abort(); }

static void Require(bool value, const char* message) {
    if (value) return;
    std::fprintf(stderr, "guest regmgr: %s\n", message);
    std::abort();
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

template <typename TCall>
static bool ReportsInvalidArgument(const char* name, TCall call) {
    try {
        call();
    } catch (const std::invalid_argument& error) {
        return std::string(error.what()) == std::string(name) + ": invalid argument";
    }
    return false;
}

static constexpr std::size_t GuestPage = 0x1000;
static constexpr std::uint32_t ProtectionReadWrite = 0x04;

static void* GuestMemory() {
    void* pointer = GuestArena::GuestArenaAllocate_nid_postfix(GuestPage, GuestPage);
    if (pointer == nullptr) return nullptr;
    GuestArena::GuestArenaCommit_nid_postfix(pointer, GuestPage, ProtectionReadWrite, GuestPage);
    return pointer;
}

static std::int32_t* GuestInt() {
    return static_cast<std::int32_t*>(GuestMemory());
}

static char* GuestText(std::size_t bytes) {
    return static_cast<char*>(GuestMemory());
}

int main() {
    std::int32_t value = 0;
    Require(ReportsNotImplemented("sceRegMgrNonSysGetInt", [&] { sceRegMgrNonSysGetInt(0, 0, &value); }));
    Require(ReportsNotImplemented("sceRegMgrNonSysGetInt", [&] { sceRegMgrNonSysGetInt(-1, 1, nullptr); }));

    std::int32_t* stored = GuestInt();
    Require(stored != nullptr, "the guest arena did not hand out memory");

    Require(sceRegMgrGetInt(0x1234, stored) == 0, "getInt of an unset option failed");
    Require(*stored == 0, "getInt of an unset option did not report zero");

    Require(sceRegMgrSetInt(0x1234, -7) == 0, "setInt failed");
    Require(sceRegMgrGetInt(0x1234, stored) == 0, "getInt failed");
    Require(*stored == -7, "getInt did not report the value that was set");

    Require(sceRegMgrSetInt(0x1234, 42) == 0, "setInt of a second value failed");
    Require(sceRegMgrGetInt(0x1234, stored) == 0 && *stored == 42, "getInt did not report the newer value");

    Require(ReportsInvalidArgument("sceRegMgrGetInt", [&] { sceRegMgrGetInt(1, nullptr); }));

    char* text = GuestText(64);
    Require(text != nullptr, "the guest arena did not hand out memory");
    Require(sceRegMgrGetStr(0x5678, text, 64) == 0, "getStr of an unset option failed");
    Require(text[0] == '\0', "getStr of an unset option did not report an empty string");

    const char* message = "hello";
    Require(sceRegMgrSetStr(0x5678, message, std::strlen(message) + 1) == 0, "setStr failed");
    Require(sceRegMgrGetStr(0x5678, text, 64) == 0, "getStr failed");
    Require(std::string(text) == "hello", "getStr did not report the value that was set");

    char* small = GuestText(4);
    Require(small != nullptr, "the guest arena did not hand out memory");
    Require(sceRegMgrGetStr(0x5678, small, 4) == 0, "getStr into a short buffer failed");
    Require(std::string(small) == "hel", "getStr did not truncate to the buffer");
    Require(small[3] == '\0', "getStr did not terminate the truncated string");

    Require(ReportsInvalidArgument("sceRegMgrGetStr", [&] { sceRegMgrGetStr(1, nullptr, 8); }));
    Require(ReportsInvalidArgument("sceRegMgrGetStr", [&] { sceRegMgrGetStr(1, text, 0); }));
    Require(ReportsInvalidArgument("sceRegMgrSetStr", [&] { sceRegMgrSetStr(1, nullptr, 8); }));
    Require(ReportsInvalidArgument("sceRegMgrSetStr", [&] { sceRegMgrSetStr(1, message, 0); }));

    const char* empty = "";
    Require(sceRegMgrSetStr(0x9abc, empty, 1) == 0, "setStr of an empty string failed");
    Require(sceRegMgrGetStr(0x9abc, text, 64) == 0 && text[0] == '\0', "getStr did not report the empty string that was set");
}
