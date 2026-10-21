#ifndef CORE_LIBS_PRX_LIBSCEUSERSERVICE_INCLUDE_GUESTUSERSERVICE_HPP
#define CORE_LIBS_PRX_LIBSCEUSERSERVICE_INCLUDE_GUESTUSERSERVICE_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "SceTypes.hpp"
#include "prx/libSceUserService/UserService.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

extern "C" {

int APS5_VABI sceUserServiceGetAccessibilityChatTranscription(int user_id, std::int32_t* chat_transcription);
int APS5_VABI sceUserServiceGetAccessibilityKeyremapEnable(std::int32_t user_id, std::int32_t* enabled);
int APS5_VABI sceUserServiceGetAccessibilityPressAndHoldDelay(int user_id, std::int32_t* press_and_hold_delay);
int APS5_VABI sceUserServiceGetAccessibilityTriggerEffect(int user_id, std::int32_t* trigger_effect);
int APS5_VABI sceUserServiceGetAccessibilityVibration(int user_id, std::int32_t* vibration);
int APS5_VABI sceUserServiceGetAccessibilityZoom(std::int32_t user_id, std::int32_t* zoom);
int APS5_VABI sceUserServiceGetAccessibilityZoomEnabled(int user_id, std::int32_t* zoom_enabled);
int APS5_VABI sceUserServiceGetAccessibilityZoomFollowFocus(int user_id, std::int32_t* zoom_follow_focus);
int APS5_VABI sceUserServiceGetAgeLevel(int user_id, std::uint32_t* age_level);
int APS5_VABI sceUserServiceDestroyUser(int user_id);
int APS5_VABI sceUserServiceGetEvent(SceUserServiceEvent* event);
int APS5_VABI sceUserServiceGetForegroundUser(int* user_id);
int APS5_VABI sceUserServiceGetGamePresets(int user_id, UserServiceGamePresets* presets);
int APS5_VABI sceUserServiceGetInitialUser(int* user_id);
int APS5_VABI sceUserServiceGetLoginUserIdList(UserServiceLoginUserIdList* user_id_list);
int APS5_VABI sceUserServiceGetNpAccountId(int user_id, std::uint64_t* account_id);
int APS5_VABI sceUserServiceGetPlatformPrivacyWs1(std::int32_t user_id, std::int32_t* value);
int APS5_VABI sceUserServiceGetRegisteredUserIdList(UserServiceRegisteredUserIdList* user_id_list);
int APS5_VABI sceUserServiceSetUserName(int user_id, const char* name);
int APS5_VABI sceUserServiceGetUserColor(int user_id, int* color);
int APS5_VABI sceUserServiceGetUserName(int user_id, char* name, std::size_t size);
int APS5_VABI sceUserServiceGetUserNumber(int user_id, std::int32_t* number);
int APS5_VABI sceUserServiceInitialize(const UserServiceInitParam* params);
int APS5_VABI sceUserServiceInitialize2(std::int32_t priority, KernelCpumask affinityMask);
int APS5_VABI sceUserServiceTerminate(void);

}

static_assert(std::is_same_v<decltype(&sceUserServiceGetAccessibilityChatTranscription), int (APS5_VABI*)(int, std::int32_t*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetAccessibilityKeyremapEnable), int (APS5_VABI*)(std::int32_t, std::int32_t*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetAccessibilityPressAndHoldDelay), int (APS5_VABI*)(int, std::int32_t*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetAccessibilityTriggerEffect), int (APS5_VABI*)(int, std::int32_t*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetAccessibilityVibration), int (APS5_VABI*)(int, std::int32_t*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetAccessibilityZoom), int (APS5_VABI*)(std::int32_t, std::int32_t*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetAccessibilityZoomEnabled), int (APS5_VABI*)(int, std::int32_t*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetAccessibilityZoomFollowFocus), int (APS5_VABI*)(int, std::int32_t*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetAgeLevel), int (APS5_VABI*)(int, std::uint32_t*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceDestroyUser), int (APS5_VABI*)(int)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetEvent), int (APS5_VABI*)(SceUserServiceEvent*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetForegroundUser), int (APS5_VABI*)(int*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetGamePresets), int (APS5_VABI*)(int, UserServiceGamePresets*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetInitialUser), int (APS5_VABI*)(int*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetLoginUserIdList), int (APS5_VABI*)(UserServiceLoginUserIdList*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetNpAccountId), int (APS5_VABI*)(int, std::uint64_t*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetPlatformPrivacyWs1), int (APS5_VABI*)(std::int32_t, std::int32_t*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetRegisteredUserIdList), int (APS5_VABI*)(UserServiceRegisteredUserIdList*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceSetUserName), int (APS5_VABI*)(int, const char*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetUserColor), int (APS5_VABI*)(int, int*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetUserName), int (APS5_VABI*)(int, char*, std::size_t)>);
static_assert(std::is_same_v<decltype(&sceUserServiceGetUserNumber), int (APS5_VABI*)(int, std::int32_t*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceInitialize), int (APS5_VABI*)(const UserServiceInitParam*)>);
static_assert(std::is_same_v<decltype(&sceUserServiceInitialize2), int (APS5_VABI*)(std::int32_t, KernelCpumask)>);
static_assert(std::is_same_v<decltype(&sceUserServiceTerminate), int (APS5_VABI*)()>);

#endif
