#include "SceTypes.hpp"
#include "prx/libSceUserService/UserService.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" {
int APS5_VABI sceUserServiceGetForegroundUser(int* user_id);
int APS5_VABI sceUserServiceGetRegisteredUserIdList(UserServiceRegisteredUserIdList* user_id_list);
int APS5_VABI sceUserServiceGetUserColor(int user_id, int* color);
int APS5_VABI sceUserServiceGetNpAccountId(int user_id, std::uint64_t* account_id);
int APS5_VABI sceUserServiceGetAgeLevel(int user_id, std::uint32_t* age_level);
int APS5_VABI sceUserServiceGetUserStatus(int user_id, std::int32_t* status);
int APS5_VABI sceUserServiceGetUserIdByNpAccountId(std::uint64_t account_id, int* user_id);
int APS5_VABI sceUserServiceGetUserIdByUserName(const char* name, int* user_id);
int APS5_VABI sceUserServiceGetAccessibilityChatTranscription(int user_id, std::int32_t* value);
int APS5_VABI sceUserServiceGetAccessibilityPressAndHoldDelay(int user_id, std::int32_t* value);
int APS5_VABI sceUserServiceGetAccessibilityTriggerEffect(int user_id, std::int32_t* value);
int APS5_VABI sceUserServiceGetAccessibilityVibration(int user_id, std::int32_t* value);
int APS5_VABI sceUserServiceGetAccessibilityZoomEnabled(int user_id, std::int32_t* value);
int APS5_VABI sceUserServiceGetAccessibilityZoomFollowFocus(int user_id, std::int32_t* value);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

}

int main() {
    Require(sceUserServiceGetForegroundUser(nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    int foreground = -1;
    Require(sceUserServiceGetForegroundUser(&foreground) == USER_SERVICE_OK);
    Require(foreground == USER_SERVICE_INITIAL_USER_ID);

    Require(sceUserServiceGetRegisteredUserIdList(nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    UserServiceRegisteredUserIdList registered{};
    Require(sceUserServiceGetRegisteredUserIdList(&registered) == USER_SERVICE_OK);
    Require(registered.user_id[0] == USER_SERVICE_INITIAL_USER_ID);
    for (int i = 1; i < 16; ++i) {
        Require(registered.user_id[i] == USER_SERVICE_USER_ID_INVALID);
    }

    int color = -1;
    Require(sceUserServiceGetUserColor(USER_SERVICE_INITIAL_USER_ID, nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    Require(sceUserServiceGetUserColor(123, &color) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    Require(sceUserServiceGetUserColor(USER_SERVICE_INITIAL_USER_ID, &color) == USER_SERVICE_OK);
    Require(color == 0);

    std::uint64_t account_id = 0xdeadbeefu;
    Require(sceUserServiceGetNpAccountId(USER_SERVICE_INITIAL_USER_ID, nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    Require(sceUserServiceGetNpAccountId(123, &account_id) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    Require(sceUserServiceGetNpAccountId(USER_SERVICE_INITIAL_USER_ID, &account_id) == USER_SERVICE_OK);
    Require(account_id == 0);

    std::uint32_t age_level = 0x5a5a5a5au;
    Require(sceUserServiceGetAgeLevel(USER_SERVICE_INITIAL_USER_ID, nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    const int absent_users[] = {0, USER_SERVICE_USER_ID_INVALID, 0xff, 0x12345678};
    for (int user_id : absent_users) {
        Require(sceUserServiceGetAgeLevel(user_id, &age_level) == USER_SERVICE_ERROR_NOT_LOGGED_IN);
        Require(age_level == 0x5a5a5a5au);
        Require(sceUserServiceGetAgeLevel(user_id, nullptr) == USER_SERVICE_ERROR_NOT_LOGGED_IN);
    }
    Require(sceUserServiceGetAgeLevel(USER_SERVICE_INITIAL_USER_ID, &age_level) == USER_SERVICE_OK);
    Require(age_level == 0xffffffffu);

    std::int32_t status = 0x5a5a5a5a;
    Require(sceUserServiceGetUserStatus(USER_SERVICE_INITIAL_USER_ID, nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    Require(sceUserServiceGetUserStatus(USER_SERVICE_INITIAL_USER_ID, &status) == USER_SERVICE_OK);
    Require(status == USER_SERVICE_USER_STATUS_LOGGED_IN);
    for (int user_id : absent_users) {
        status = 0x5a5a5a5a;
        Require(sceUserServiceGetUserStatus(user_id, &status) == USER_SERVICE_ERROR_NOT_LOGGED_IN);
        Require(status == 0x5a5a5a5a);
        Require(sceUserServiceGetUserStatus(user_id, nullptr) == USER_SERVICE_ERROR_NOT_LOGGED_IN);
    }

    int resolved = 0x5a5a5a5a;
    Require(sceUserServiceGetUserIdByNpAccountId(0, nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    Require(sceUserServiceGetUserIdByNpAccountId(1, nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    Require(sceUserServiceGetUserIdByNpAccountId(0, &resolved) == USER_SERVICE_OK);
    Require(resolved == USER_SERVICE_INITIAL_USER_ID);
    const std::uint64_t unknown_accounts[] = {1, 0xdeadbeefu, 0x100000000ull};
    for (std::uint64_t unknown_account : unknown_accounts) {
        resolved = 0x5a5a5a5a;
        Require(sceUserServiceGetUserIdByNpAccountId(unknown_account, &resolved) == USER_SERVICE_ERROR_UNKNOWN_USER);
        Require(resolved == 0x5a5a5a5a);
    }

    resolved = 0x5a5a5a5a;
    Require(sceUserServiceGetUserIdByUserName(nullptr, &resolved) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    Require(sceUserServiceGetUserIdByUserName(USER_SERVICE_INITIAL_USER_NAME, nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    Require(resolved == 0x5a5a5a5a);
    Require(sceUserServiceGetUserIdByUserName(USER_SERVICE_INITIAL_USER_NAME, &resolved) == USER_SERVICE_OK);
    Require(resolved == USER_SERVICE_INITIAL_USER_ID);
    const char* unknown_names[] = {"", "Playe", "Player2"};
    for (const char* unknown_name : unknown_names) {
        resolved = 0x5a5a5a5a;
        Require(sceUserServiceGetUserIdByUserName(unknown_name, &resolved) == USER_SERVICE_ERROR_UNKNOWN_USER);
        Require(resolved == 0x5a5a5a5a);
    }

    struct Setting {
        int (APS5_VABI *get)(int, std::int32_t*);
        std::int32_t value;
    };
    const Setting settings[] = {
        {sceUserServiceGetAccessibilityChatTranscription, 0},
        {sceUserServiceGetAccessibilityPressAndHoldDelay, 0},
        {sceUserServiceGetAccessibilityTriggerEffect, 1},
        {sceUserServiceGetAccessibilityVibration, 1},
        {sceUserServiceGetAccessibilityZoomEnabled, 0},
        {sceUserServiceGetAccessibilityZoomFollowFocus, 1},
    };
    for (const Setting& setting : settings) {
        std::int32_t value = 0x5a5a5a5a;
        Require(setting.get(USER_SERVICE_INITIAL_USER_ID, nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
        for (int user_id : absent_users) {
            Require(setting.get(user_id, &value) == USER_SERVICE_ERROR_NOT_LOGGED_IN);
            Require(value == 0x5a5a5a5a);
            Require(setting.get(user_id, nullptr) == USER_SERVICE_ERROR_NOT_LOGGED_IN);
        }
        Require(setting.get(USER_SERVICE_INITIAL_USER_ID, &value) == USER_SERVICE_OK);
        Require(value == setting.value);
    }
}
