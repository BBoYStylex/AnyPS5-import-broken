#include "prx/libSceSysUtil/SysUtil.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceSysUtilSendSystemNotification(std::int32_t messageId) {
    (void)messageId;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendSystemNotification2(void* input) {
    (void)input;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendSystemNotificationParamInit(SceNotificationRequest* notifyBuffer, SceNotificationRequestType type) {
    (void)notifyBuffer;
    (void)type;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendSystemNotificationWithAppId(std::int32_t messageId, std::int32_t appId) {
    (void)messageId;
    (void)appId;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendSystemNotificationWithAppInfo(std::int32_t messageId, const char* arg1, const char* arg2) {
    (void)messageId;
    (void)arg1;
    (void)arg2;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendSystemNotificationWithAppName(std::int32_t messageId, const char* appName) {
    (void)messageId;
    (void)appName;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendSystemNotificationWithAppNameRelatedToUser(std::int32_t messageId, std::int32_t userId, const char* appName) {
    (void)messageId;
    (void)userId;
    (void)appName;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendSystemNotificationWithDeviceId(std::int32_t messageId, std::int32_t deviceId) {
    (void)messageId;
    (void)deviceId;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendSystemNotificationWithDeviceIdRelatedToUser(std::int32_t messageId, std::int32_t userId, std::int32_t deviceId) {
    (void)messageId;
    (void)userId;
    (void)deviceId;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendSystemNotificationWithErrorCode(std::int32_t messageId, std::int32_t errorCode) {
    (void)messageId;
    (void)errorCode;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendSystemNotificationWithParams() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendSystemNotificationWithText(std::int32_t type, const char* message) {
    (void)type;
    (void)message;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendSystemNotificationWithTextRelatedToUser(std::int32_t messageId, std::int32_t userId, const char* message) {
    (void)messageId;
    (void)userId;
    (void)message;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendSystemNotificationWithUserId(std::int32_t messageId, std::int32_t userId) {
    (void)messageId;
    (void)userId;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendSystemNotificationWithUserName(std::int32_t messageId, const char* userName) {
    (void)messageId;
    (void)userName;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendSystemNotificationWithUserNameInfo(std::int32_t messageId, const char* arg1, const char* arg2) {
    (void)messageId;
    (void)arg1;
    (void)arg2;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendAddressingSystemNotification(std::int32_t messageId, std::int32_t targetId) {
    (void)messageId;
    (void)targetId;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendAddressingSystemNotificationWithDeviceId(std::int32_t messageId, std::int32_t targetId, std::int32_t deviceId) {
    (void)messageId;
    (void)targetId;
    (void)deviceId;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendAddressingSystemNotificationWithUserId(std::int32_t messageId, std::int32_t targetId, std::int32_t userId) {
    (void)messageId;
    (void)targetId;
    (void)userId;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendAddressingSystemNotificationWithUserName(std::int32_t messageId, std::int32_t targetId, const char* userName) {
    (void)messageId;
    (void)targetId;
    (void)userName;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendNotificationRequest(const char* message, bool blocking) {
    (void)message;
    (void)blocking;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendTrcCheckNotificationRequest(const char* message, bool blocking) {
    (void)message;
    (void)blocking;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendNpDebugNotificationRequest(const char* message, bool blocking) {
    (void)message;
    (void)blocking;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceSysUtilSendWebDebugNotificationRequest(const char* message, bool blocking) {
    (void)message;
    (void)blocking;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
