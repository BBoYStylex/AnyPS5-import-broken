#ifndef CORE_LIBS_PRX_LIBSCESAVEDATADIALOGNATIVE_SAVEDATADIALOG_HPP
#define CORE_LIBS_PRX_LIBSCESAVEDATADIALOGNATIVE_SAVEDATADIALOG_HPP

#include <cstdint>
#include <cstddef>

constexpr int SAVE_DATA_DIALOG_OK = 0;
constexpr int SAVE_DATA_DIALOG_ERROR_NOT_INITIALIZED = static_cast<int>(0x80B80003);
constexpr int SAVE_DATA_DIALOG_ERROR_ALREADY_INITIALIZED = static_cast<int>(0x80B80004);
constexpr int SAVE_DATA_DIALOG_ERROR_INVALID_STATE = static_cast<int>(0x80B80006);
constexpr int SAVE_DATA_DIALOG_ERROR_NOT_FINISHED = static_cast<int>(0x80B80005);
constexpr int SAVE_DATA_DIALOG_ERROR_INVALID_PARAMETER = static_cast<int>(0x80B8000A);
constexpr int SAVE_DATA_DIALOG_ERROR_NOT_RUNNING = static_cast<int>(0x80B8000B);
constexpr int SAVE_DATA_DIALOG_ERROR_ARG_NULL = static_cast<int>(0x80B8000D);

constexpr int SAVE_DATA_DIALOG_STATUS_NONE = 0;
constexpr int SAVE_DATA_DIALOG_STATUS_INITIALIZED = 1;
constexpr int SAVE_DATA_DIALOG_STATUS_RUNNING = 2;
constexpr int SAVE_DATA_DIALOG_STATUS_FINISHED = 3;

constexpr int SAVE_DATA_DIALOG_MODE_LIST = 1;
constexpr int SAVE_DATA_DIALOG_MODE_PROGRESS_BAR = 5;

constexpr int SAVE_DATA_DIALOG_RESULT_OK = 0;
constexpr int SAVE_DATA_DIALOG_RESULT_CANCELED = 1;
constexpr int SAVE_DATA_DIALOG_BUTTON_ID_INVALID = 0;
constexpr int SAVE_DATA_DIALOG_BUTTON_ID_OK = 1;

struct SaveDataDirName {
    char data[32];
};

struct SaveDataDialogItems {
    std::int32_t user_id;
    std::int32_t pad0;
    const void* title_id;
    const SaveDataDirName* dir_names;
    std::uint32_t dir_names_num;
};

struct SaveDataDialogProgressBarParam {
    std::uint32_t bar_type;
    std::uint32_t pad0;
    const char* message;
    std::uint32_t system_message_type;
    std::uint8_t reserved[28];
};

struct SaveDataDialogParam {
    std::uint8_t base_param[48];
    std::int32_t size;
    std::int32_t mode;
    std::int32_t disp_type;
    std::uint32_t pad0;
    void* anim_param;
    void* items;
    void* user_msg_param;
    void* sys_msg_param;
    void* error_code_param;
    void* prog_bar_param;
    void* user_data;
    void* option_param;
    void* wizard_param;
    std::uint8_t reserved[16];
};

struct SaveDataDialogResult {
    std::int32_t mode;
    std::int32_t result;
    std::int32_t button_id;
    std::uint32_t pad0;
    void* dir_name;
    void* param;
    void* user_data;
    std::uint8_t reserved[32];
};

static_assert(sizeof(SaveDataDialogParam) == 0x98);
static_assert(offsetof(SaveDataDialogParam, mode) == 0x34);
static_assert(offsetof(SaveDataDialogParam, items) == 0x48);
static_assert(offsetof(SaveDataDialogParam, user_data) == 0x70);
static_assert(offsetof(SaveDataDialogProgressBarParam, message) == 8);
static_assert(sizeof(SaveDataDialogProgressBarParam) == 0x30);
static_assert(sizeof(SaveDataDialogResult) == 0x48);

#endif
