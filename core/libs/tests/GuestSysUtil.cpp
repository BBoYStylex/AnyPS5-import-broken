#include "prx/libSceSysUtil/SysUtil.hpp"
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>

static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotification), int (APS5_VABI*)(std::int32_t)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotification2), int (APS5_VABI*)(void*)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotificationParamInit), int (APS5_VABI*)(SceNotificationRequest*, SceNotificationRequestType)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotificationWithAppId), int (APS5_VABI*)(std::int32_t, std::int32_t)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotificationWithAppInfo), int (APS5_VABI*)(std::int32_t, const char*, const char*)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotificationWithAppName), int (APS5_VABI*)(std::int32_t, const char*)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotificationWithAppNameRelatedToUser), int (APS5_VABI*)(std::int32_t, std::int32_t, const char*)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotificationWithDeviceId), int (APS5_VABI*)(std::int32_t, std::int32_t)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotificationWithDeviceIdRelatedToUser), int (APS5_VABI*)(std::int32_t, std::int32_t, std::int32_t)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotificationWithErrorCode), int (APS5_VABI*)(std::int32_t, std::int32_t)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotificationWithParams), int (APS5_VABI*)()>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotificationWithText), int (APS5_VABI*)(std::int32_t, const char*)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotificationWithTextRelatedToUser), int (APS5_VABI*)(std::int32_t, std::int32_t, const char*)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotificationWithUserId), int (APS5_VABI*)(std::int32_t, std::int32_t)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotificationWithUserName), int (APS5_VABI*)(std::int32_t, const char*)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendSystemNotificationWithUserNameInfo), int (APS5_VABI*)(std::int32_t, const char*, const char*)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendAddressingSystemNotification), int (APS5_VABI*)(std::int32_t, std::int32_t)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendAddressingSystemNotificationWithDeviceId), int (APS5_VABI*)(std::int32_t, std::int32_t, std::int32_t)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendAddressingSystemNotificationWithUserId), int (APS5_VABI*)(std::int32_t, std::int32_t, std::int32_t)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendAddressingSystemNotificationWithUserName), int (APS5_VABI*)(std::int32_t, std::int32_t, const char*)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendNotificationRequest), int (APS5_VABI*)(const char*, bool)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendTrcCheckNotificationRequest), int (APS5_VABI*)(const char*, bool)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendNpDebugNotificationRequest), int (APS5_VABI*)(const char*, bool)>);
static_assert(std::is_same_v<decltype(&sceSysUtilSendWebDebugNotificationRequest), int (APS5_VABI*)(const char*, bool)>);

namespace {

template <typename Call>
void RequireThrows(const char* name, Call&& call) {
    try {
        call();
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()).rfind(name, 0) == 0) return;
        std::fprintf(stderr, "%s threw %s\n", name, error.what());
        std::exit(1);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s threw a non-runtime error: %s\n", name, error.what());
        std::exit(1);
    }
    std::fprintf(stderr, "%s returned instead of throwing\n", name);
    std::exit(1);
}

}

int main() {
    RequireThrows("sceSysUtilSendSystemNotification", [] { sceSysUtilSendSystemNotification(1); });
    RequireThrows("sceSysUtilSendSystemNotification2", [] { sceSysUtilSendSystemNotification2(nullptr); });
    RequireThrows("sceSysUtilSendSystemNotificationParamInit", [] {
        sceSysUtilSendSystemNotificationParamInit(nullptr, SceNotificationRequestType::SystemNotification);
    });
    RequireThrows("sceSysUtilSendSystemNotificationWithAppId", [] { sceSysUtilSendSystemNotificationWithAppId(1, 2); });
    RequireThrows("sceSysUtilSendSystemNotificationWithAppInfo", [] { sceSysUtilSendSystemNotificationWithAppInfo(1, "a", "b"); });
    RequireThrows("sceSysUtilSendSystemNotificationWithAppName", [] { sceSysUtilSendSystemNotificationWithAppName(1, "name"); });
    RequireThrows("sceSysUtilSendSystemNotificationWithAppNameRelatedToUser", [] {
        sceSysUtilSendSystemNotificationWithAppNameRelatedToUser(1, 2, "name");
    });
    RequireThrows("sceSysUtilSendSystemNotificationWithDeviceId", [] { sceSysUtilSendSystemNotificationWithDeviceId(1, 2); });
    RequireThrows("sceSysUtilSendSystemNotificationWithDeviceIdRelatedToUser", [] {
        sceSysUtilSendSystemNotificationWithDeviceIdRelatedToUser(1, 2, 3);
    });
    RequireThrows("sceSysUtilSendSystemNotificationWithErrorCode", [] { sceSysUtilSendSystemNotificationWithErrorCode(1, 2); });
    RequireThrows("sceSysUtilSendSystemNotificationWithParams", [] { sceSysUtilSendSystemNotificationWithParams(); });
    RequireThrows("sceSysUtilSendSystemNotificationWithText", [] { sceSysUtilSendSystemNotificationWithText(1, "text"); });
    RequireThrows("sceSysUtilSendSystemNotificationWithTextRelatedToUser", [] {
        sceSysUtilSendSystemNotificationWithTextRelatedToUser(1, 2, "text");
    });
    RequireThrows("sceSysUtilSendSystemNotificationWithUserId", [] { sceSysUtilSendSystemNotificationWithUserId(1, 2); });
    RequireThrows("sceSysUtilSendSystemNotificationWithUserName", [] { sceSysUtilSendSystemNotificationWithUserName(1, "name"); });
    RequireThrows("sceSysUtilSendSystemNotificationWithUserNameInfo", [] {
        sceSysUtilSendSystemNotificationWithUserNameInfo(1, "a", "b");
    });
    RequireThrows("sceSysUtilSendAddressingSystemNotification", [] { sceSysUtilSendAddressingSystemNotification(1, 2); });
    RequireThrows("sceSysUtilSendAddressingSystemNotificationWithDeviceId", [] {
        sceSysUtilSendAddressingSystemNotificationWithDeviceId(1, 2, 3);
    });
    RequireThrows("sceSysUtilSendAddressingSystemNotificationWithUserId", [] {
        sceSysUtilSendAddressingSystemNotificationWithUserId(1, 2, 3);
    });
    RequireThrows("sceSysUtilSendAddressingSystemNotificationWithUserName", [] {
        sceSysUtilSendAddressingSystemNotificationWithUserName(1, 2, "name");
    });
    RequireThrows("sceSysUtilSendNotificationRequest", [] { sceSysUtilSendNotificationRequest("message", true); });
    RequireThrows("sceSysUtilSendTrcCheckNotificationRequest", [] { sceSysUtilSendTrcCheckNotificationRequest("message", false); });
    RequireThrows("sceSysUtilSendNpDebugNotificationRequest", [] { sceSysUtilSendNpDebugNotificationRequest("message", true); });
    RequireThrows("sceSysUtilSendWebDebugNotificationRequest", [] { sceSysUtilSendWebDebugNotificationRequest("message", false); });
    std::printf("libSceSysUtil export prototypes passed\n");
    return 0;
}
