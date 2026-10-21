#ifndef CORE_LIBS_PRX_LIBSCESYSUTIL_SYSUTIL_HPP
#define CORE_LIBS_PRX_LIBSCESYSUTIL_SYSUTIL_HPP

#include <cstdint>
#include "prx/libc/include/general/VabiMacros.hpp"

enum class SceNotificationRequestType : std::uint32_t {
    NotificationRequest = 0,
    SystemNotification = 1,
    SystemNotificationWithUserId = 2,
    SystemNotificationWithDeviceId = 3,
    SystemNotificationWithDeviceIdRelatedToUser = 4,
    SystemNotificationWithText = 5,
    SystemNotificationWithTextRelatedToUser = 6,
    SystemNotificationWithErrorCode = 7,
    SystemNotificationWithAppId = 8,
    SystemNotificationWithAppName = 9,
    SystemNotificationWithAppInfo = 9,
    SystemNotificationWithAppNameRelatedToUser = 10,
    SystemNotificationWithParams = 11,
    SendSystemNotificationWithUserName = 12,
    SystemNotificationWithUserNameInfo = 13,
    SendAddressingSystemNotification = 14,
    AddressingSystemNotificationWithDeviceId = 15,
    AddressingSystemNotificationWithUserName = 16,
    AddressingSystemNotificationWithUserId = 17,
    UNK_1 = 100,
    TrcCheckNotificationRequest = 101,
    NpDebugNotificationRequest = 102,
    UNK_2 = 102,
};

struct SceNotificationRequest;

extern "C" {

int APS5_VABI sceSysUtilSendSystemNotification(std::int32_t messageId);
int APS5_VABI sceSysUtilSendSystemNotification2(void* input);
int APS5_VABI sceSysUtilSendSystemNotificationParamInit(SceNotificationRequest* notifyBuffer, SceNotificationRequestType type);
int APS5_VABI sceSysUtilSendSystemNotificationWithAppId(std::int32_t messageId, std::int32_t appId);
int APS5_VABI sceSysUtilSendSystemNotificationWithAppInfo(std::int32_t messageId, const char* arg1, const char* arg2);
int APS5_VABI sceSysUtilSendSystemNotificationWithAppName(std::int32_t messageId, const char* appName);
int APS5_VABI sceSysUtilSendSystemNotificationWithAppNameRelatedToUser(std::int32_t messageId, std::int32_t userId, const char* appName);
int APS5_VABI sceSysUtilSendSystemNotificationWithDeviceId(std::int32_t messageId, std::int32_t deviceId);
int APS5_VABI sceSysUtilSendSystemNotificationWithDeviceIdRelatedToUser(std::int32_t messageId, std::int32_t userId, std::int32_t deviceId);
int APS5_VABI sceSysUtilSendSystemNotificationWithErrorCode(std::int32_t messageId, std::int32_t errorCode);
int APS5_VABI sceSysUtilSendSystemNotificationWithParams();
int APS5_VABI sceSysUtilSendSystemNotificationWithText(std::int32_t type, const char* message);
int APS5_VABI sceSysUtilSendSystemNotificationWithTextRelatedToUser(std::int32_t messageId, std::int32_t userId, const char* message);
int APS5_VABI sceSysUtilSendSystemNotificationWithUserId(std::int32_t messageId, std::int32_t userId);
int APS5_VABI sceSysUtilSendSystemNotificationWithUserName(std::int32_t messageId, const char* userName);
int APS5_VABI sceSysUtilSendSystemNotificationWithUserNameInfo(std::int32_t messageId, const char* arg1, const char* arg2);
int APS5_VABI sceSysUtilSendAddressingSystemNotification(std::int32_t messageId, std::int32_t targetId);
int APS5_VABI sceSysUtilSendAddressingSystemNotificationWithDeviceId(std::int32_t messageId, std::int32_t targetId, std::int32_t deviceId);
int APS5_VABI sceSysUtilSendAddressingSystemNotificationWithUserId(std::int32_t messageId, std::int32_t targetId, std::int32_t userId);
int APS5_VABI sceSysUtilSendAddressingSystemNotificationWithUserName(std::int32_t messageId, std::int32_t targetId, const char* userName);
int APS5_VABI sceSysUtilSendNotificationRequest(const char* message, bool blocking);
int APS5_VABI sceSysUtilSendTrcCheckNotificationRequest(const char* message, bool blocking);
int APS5_VABI sceSysUtilSendNpDebugNotificationRequest(const char* message, bool blocking);
int APS5_VABI sceSysUtilSendWebDebugNotificationRequest(const char* message, bool blocking);

}

#endif
