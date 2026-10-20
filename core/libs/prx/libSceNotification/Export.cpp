#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" int APS5_VABI sceKernelSendNotificationRequest(int device, const void* request, std::size_t size, int flags);

namespace {

constexpr std::size_t RequestSize = 3120;
constexpr std::size_t MessageOffset = 45;
constexpr std::size_t MessageCapacity = 1024;

int SendText(const char* function, const char* text) {
    std::size_t length = 0;
    while (length < MessageCapacity && text[length] != '\0') ++length;
    if (length == MessageCapacity) {
        throw std::invalid_argument(std::string(function) + ": the text is not terminated within " + std::to_string(MessageCapacity) + " bytes");
    }
    std::uint8_t request[RequestSize] = {};
    const std::int32_t type = 0;
    std::memcpy(request, &type, sizeof(type));
    std::memcpy(request + MessageOffset, text, length);
    return sceKernelSendNotificationRequest(0, request, RequestSize, 0);
}

std::string storedBanner;

}

extern "C" {

int APS5_VABI sceNotificationSend(int type, std::uint8_t flag, std::uint8_t* message) {
    (void)type;
    (void)flag;
    if (message == nullptr) throw std::invalid_argument("sceNotificationSend: null message");
    return SendText(__func__, reinterpret_cast<const char*>(message));
}

int APS5_VABI sceNotificationSendById(int type, std::uint8_t flag, std::uint8_t* id, std::uint8_t* jsonData) {
    (void)type;
    (void)flag;
    if (id == nullptr) throw std::invalid_argument("sceNotificationSendById: null id");
    if (jsonData == nullptr) throw std::invalid_argument("sceNotificationSendById: null data");
    return SendText(__func__, reinterpret_cast<const char*>(jsonData));
}

int APS5_VABI sceNotificationShowPsButtonPersistentBanner(std::uint8_t* jsonParam) {
    if (jsonParam == nullptr) throw std::invalid_argument("sceNotificationShowPsButtonPersistentBanner: null parameter");
    const char* text = reinterpret_cast<const char*>(jsonParam);
    storedBanner = text;
    return SendText(__func__, text);
}

void APS5_VABI sceNotificationHidePsButtonPersistentBanner() {
    storedBanner.clear();
}

}
