#include "SceTypes.hpp"
#include "prx/libSceUserService/UserService.hpp"
#include <cstdint>
#include <cstdlib>

extern "C" {
int APS5_VABI sceUserServiceGetForegroundUser(int* user_id);
int APS5_VABI sceUserServiceGetRegisteredUserIdList(UserServiceRegisteredUserIdList* user_id_list);
int APS5_VABI sceUserServiceGetUserColor(int user_id, int* color);
int APS5_VABI sceUserServiceGetNpAccountId(int user_id, std::uint64_t* account_id);
int APS5_VABI sceUserServiceGetAgeLevel(int user_id, std::uint32_t* age_level);
int APS5_VABI sceUserServiceGetAccessibilityChatTranscription(int user_id, std::int32_t* value);
int APS5_VABI sceUserServiceGetAccessibilityPressAndHoldDelay(int user_id, std::int32_t* value);
int APS5_VABI sceUserServiceGetAccessibilityTriggerEffect(int user_id, std::int32_t* value);
int APS5_VABI sceUserServiceGetAccessibilityVibration(int user_id, std::int32_t* value);
int APS5_VABI sceUserServiceGetAccessibilityZoomEnabled(int user_id, std::int32_t* value);
int APS5_VABI sceUserServiceGetAccessibilityZoomFollowFocus(int user_id, std::int32_t* value);
int APS5_VABI sceUserServiceInitialize(const void*);
int APS5_VABI sceUserServiceInitialize2(void);
int APS5_VABI sceUserServiceTerminate(void);
int APS5_VABI sceUserServiceGetEvent(SceUserServiceEvent*);
int APS5_VABI sceUserServiceGetPlatformPrivacyWs1(std::int32_t, std::int32_t*);
int APS5_VABI sceUserServiceGetInitialUser(int*);
int APS5_VABI sceUserServiceGetLoginUserIdList(UserServiceLoginUserIdList*);
int APS5_VABI sceUserServiceGetUserNumber(int, std::int32_t*);
int APS5_VABI sceUserServiceGetGamePresets(int, UserServiceGamePresets*);
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

    Require(sceUserServiceInitialize(nullptr) == USER_SERVICE_OK);
    Require(sceUserServiceInitialize2() == USER_SERVICE_OK);

    Require(sceUserServiceGetEvent(nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    SceUserServiceEvent event{};
    Require(sceUserServiceGetEvent(&event) == USER_SERVICE_OK);
    Require(event.event_type == 0 && event.user_id == USER_SERVICE_INITIAL_USER_ID);
    Require(sceUserServiceGetEvent(&event) == USER_SERVICE_ERROR_NO_EVENT);

    Require(sceUserServiceTerminate() == USER_SERVICE_OK);
    Require(sceUserServiceGetEvent(&event) == USER_SERVICE_OK);
    Require(event.event_type == 0 && event.user_id == USER_SERVICE_INITIAL_USER_ID);

    Require(sceUserServiceGetPlatformPrivacyWs1(USER_SERVICE_INITIAL_USER_ID, nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    std::int32_t privacy = -1;
    for (int user_id : absent_users) {
        Require(sceUserServiceGetPlatformPrivacyWs1(user_id, &privacy) == USER_SERVICE_ERROR_NOT_LOGGED_IN);
    }
    Require(sceUserServiceGetPlatformPrivacyWs1(USER_SERVICE_INITIAL_USER_ID, &privacy) == 0 && privacy == 0);

    Require(sceUserServiceGetInitialUser(nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    int initialUser = -1;
    Require(sceUserServiceGetInitialUser(&initialUser) == USER_SERVICE_OK && initialUser == USER_SERVICE_INITIAL_USER_ID);

    Require(sceUserServiceGetLoginUserIdList(nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    UserServiceLoginUserIdList loginList{};
    Require(sceUserServiceGetLoginUserIdList(&loginList) == USER_SERVICE_OK);
    Require(loginList.user_id[0] == USER_SERVICE_INITIAL_USER_ID);
    for (int i = 1; i < 4; ++i) {
        Require(loginList.user_id[i] == USER_SERVICE_USER_ID_INVALID);
    }

    Require(sceUserServiceGetUserNumber(USER_SERVICE_INITIAL_USER_ID, nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    std::int32_t userNumber = -1;
    Require(sceUserServiceGetUserNumber(123, &userNumber) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    Require(sceUserServiceGetUserNumber(USER_SERVICE_INITIAL_USER_ID, &userNumber) == USER_SERVICE_OK && userNumber == 1);

    Require(sceUserServiceGetGamePresets(USER_SERVICE_INITIAL_USER_ID, nullptr) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    UserServiceGamePresets presets{};
    Require(sceUserServiceGetGamePresets(123, &presets) == USER_SERVICE_ERROR_INVALID_ARGUMENT);
    Require(sceUserServiceGetGamePresets(USER_SERVICE_INITIAL_USER_ID, &presets) == USER_SERVICE_OK);
    Require(presets.difficulty == 0 && presets.priority == 0 && presets.audio_language == 0);
}
