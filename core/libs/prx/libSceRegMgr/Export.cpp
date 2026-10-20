#include <cstdint>
#include <cstddef>
#include <cstring>
#include <map>
#include <mutex>
#include <string>

#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestArena.hpp"

namespace {

struct Value {
    bool text = false;
    std::int64_t integer = 0;
    std::string characters;
};

std::mutex registryMutex;
std::map<std::uint32_t, Value> values;

}

extern "C" {

int APS5_VABI sceRegMgrGetBin() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrGetInt(std::int32_t optionId, std::int32_t* outInt) {
    if (outInt == nullptr) APS5_INVALID_ARG_EX;
    std::int64_t value = 0;
    {
        std::lock_guard lock(registryMutex);
        const auto found = values.find(static_cast<std::uint32_t>(optionId));
        if (found != values.end() && !found->second.text) value = found->second.integer;
    }
    const GuestArena::HostWrite write(outInt, sizeof(*outInt));
    if (!write.Open()) throw std::invalid_argument(std::string(__func__) + ": the output is not writable");
    *outInt = static_cast<std::int32_t>(value);
    return 0;
}

int APS5_VABI sceRegMgrSetInt(std::int32_t optionId, std::int32_t inInt) {
    std::lock_guard lock(registryMutex);
    values[static_cast<std::uint32_t>(optionId)] = {false, inInt, {}};
    return 0;
}

int APS5_VABI sceRegMgrGetStr(std::int32_t optionId, char* outString, std::size_t stringLength) {
    if (outString == nullptr || stringLength == 0) APS5_INVALID_ARG_EX;
    std::string value;
    {
        std::lock_guard lock(registryMutex);
        const auto found = values.find(static_cast<std::uint32_t>(optionId));
        if (found != values.end() && found->second.text) value = found->second.characters;
    }
    const auto length = value.size() < stringLength - 1 ? value.size() : stringLength - 1;
    const GuestArena::HostWrite write(outString, length + 1);
    if (!write.Open()) throw std::invalid_argument(std::string(__func__) + ": the output is not writable");
    if (length != 0) std::memcpy(outString, value.data(), length);
    outString[length] = '\0';
    return 0;
}

int APS5_VABI sceRegMgrSetStr(std::int32_t optionId, const char* inString, std::size_t stringLength) {
    if (inString == nullptr || stringLength == 0) APS5_INVALID_ARG_EX;
    std::string value;
    {
        const auto length = ::strnlen(inString, stringLength);
        value.assign(inString, length);
    }
    std::lock_guard lock(registryMutex);
    values[static_cast<std::uint32_t>(optionId)] = {true, 0, std::move(value)};
    return 0;
}

int APS5_VABI sceRegMgrSetBin() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceRegMgrNonSysGetInt(std::int64_t key, std::int32_t unk, std::int32_t* data) {
    (void)key;
    (void)unk;
    (void)data;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
