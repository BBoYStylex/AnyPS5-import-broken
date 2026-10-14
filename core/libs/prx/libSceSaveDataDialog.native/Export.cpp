#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>
#include "SaveDataDialog.hpp"
#include "DialogWindow.hpp"
#include "prx/libc/include/General.hpp"

namespace {

std::mutex operationMutex;
std::mutex stateMutex;
int status = SAVE_DATA_DIALOG_STATUS_NONE;
int mode = 0;
int resultCode = SAVE_DATA_DIALOG_RESULT_OK;
int button = SAVE_DATA_DIALOG_BUTTON_ID_OK;
void* userData = nullptr;
std::vector<std::string> names;
std::size_t selected = 0;
std::uint32_t progress = 0;
bool interactive = false;

void Complete(bool accepted, std::size_t index) {
    std::lock_guard lock(stateMutex);
    if (status != SAVE_DATA_DIALOG_STATUS_RUNNING) return;
    resultCode = accepted ? SAVE_DATA_DIALOG_RESULT_OK : SAVE_DATA_DIALOG_RESULT_CANCELED;
    button = accepted ? SAVE_DATA_DIALOG_BUTTON_ID_OK : SAVE_DATA_DIALOG_BUTTON_ID_INVALID;
    selected = accepted ? index : names.size();
    status = SAVE_DATA_DIALOG_STATUS_FINISHED;
}

}

extern "C" {

int APS5_VABI sceSaveDataDialogInitialize(void) {
    std::lock_guard operation(operationMutex);
    std::lock_guard lock(stateMutex);
    if (status != SAVE_DATA_DIALOG_STATUS_NONE) return SAVE_DATA_DIALOG_ERROR_ALREADY_INITIALIZED;
    status = SAVE_DATA_DIALOG_STATUS_INITIALIZED;
    interactive = SaveDialog::SaveDataDialogUiEnabled_nid_no_patch();
    mode = 0;
    userData = nullptr;
    names.clear();
    selected = 0;
    progress = 0;
    resultCode = SAVE_DATA_DIALOG_RESULT_OK;
    button = SAVE_DATA_DIALOG_BUTTON_ID_OK;
    return SAVE_DATA_DIALOG_OK;
}

int APS5_VABI sceSaveDataDialogGetStatus(void) {
    std::lock_guard lock(stateMutex);
    return status;
}

int APS5_VABI sceSaveDataDialogUpdateStatus(void) {
    return sceSaveDataDialogGetStatus();
}

int APS5_VABI sceSaveDataDialogGetResult(void* result) {
    std::lock_guard lock(stateMutex);
    if (!result) return SAVE_DATA_DIALOG_ERROR_ARG_NULL;
    if (interactive && status != SAVE_DATA_DIALOG_STATUS_FINISHED) return SAVE_DATA_DIALOG_ERROR_NOT_FINISHED;
    auto* r = static_cast<SaveDataDialogResult*>(result);
    if (interactive && r->param) NotImplemented_nid_no_patch(__func__);
    if (interactive) {
        r->pad0 = 0;
        std::memset(r->reserved, 0, sizeof(r->reserved));
    }
    r->mode = mode;
    r->result = resultCode;
    r->button_id = button;
    r->user_data = userData;
    if (r->dir_name) {
        auto* directory = static_cast<SaveDataDirName*>(r->dir_name);
        if (selected < names.size()) std::snprintf(directory->data, sizeof(directory->data), "%s", names[selected].c_str());
        else if (interactive) std::memset(directory->data, 0, sizeof(directory->data));
    }
    return SAVE_DATA_DIALOG_OK;
}

int APS5_VABI sceSaveDataDialogOpen(const void* param) {
    std::lock_guard operation(operationMutex);
    SaveDialog::View view;
    {
        std::lock_guard lock(stateMutex);
        if (status != SAVE_DATA_DIALOG_STATUS_INITIALIZED && status != SAVE_DATA_DIALOG_STATUS_FINISHED) return SAVE_DATA_DIALOG_ERROR_INVALID_STATE;
        if (!param) return SAVE_DATA_DIALOG_ERROR_ARG_NULL;
        const auto* p = static_cast<const SaveDataDialogParam*>(param);
        if (interactive) {
            if (p->size != sizeof(SaveDataDialogParam)) return SAVE_DATA_DIALOG_ERROR_INVALID_PARAMETER;
            if (p->mode != SAVE_DATA_DIALOG_MODE_LIST && p->mode != SAVE_DATA_DIALOG_MODE_PROGRESS_BAR) NotImplemented_nid_no_patch(__func__);
            if (p->disp_type < 1 || p->disp_type > 3) return SAVE_DATA_DIALOG_ERROR_INVALID_PARAMETER;
            if (p->anim_param || p->user_msg_param || p->sys_msg_param || p->error_code_param || p->option_param || p->wizard_param) NotImplemented_nid_no_patch(__func__);
        }
        view.Mode = p->mode;
        view.DisplayType = p->disp_type;
        if (interactive && p->prog_bar_param) {
            if (p->mode != SAVE_DATA_DIALOG_MODE_PROGRESS_BAR) NotImplemented_nid_no_patch(__func__);
            const auto* bar = static_cast<const SaveDataDialogProgressBarParam*>(p->prog_bar_param);
            if (bar->bar_type != 0 || bar->system_message_type > 2) return SAVE_DATA_DIALOG_ERROR_INVALID_PARAMETER;
            if (bar->message) view.Message = bar->message;
            else if (bar->system_message_type == 2) view.Message = "Restoring saved data. Please wait.";
        }
        const auto* items = static_cast<const SaveDataDialogItems*>(p->items);
        if (items) {
            if (items->dir_names_num != 0 && !items->dir_names) return SAVE_DATA_DIALOG_ERROR_ARG_NULL;
            for (std::uint32_t i = 0; i < items->dir_names_num; ++i) {
                const auto* end = static_cast<const char*>(std::memchr(items->dir_names[i].data, 0, sizeof(SaveDataDirName::data)));
                if (!end) return SAVE_DATA_DIALOG_ERROR_INVALID_PARAMETER;
                if (end != items->dir_names[i].data) view.Names.emplace_back(items->dir_names[i].data, end);
            }
        }
        mode = p->mode;
        userData = p->user_data;
    }
    SaveDialog::SaveDataDialogUiClose_nid_no_patch();
    {
        std::lock_guard lock(stateMutex);
        names = view.Names;
        selected = 0;
        progress = 0;
        resultCode = SAVE_DATA_DIALOG_RESULT_OK;
        button = SAVE_DATA_DIALOG_BUTTON_ID_OK;
        status = interactive || mode == SAVE_DATA_DIALOG_MODE_PROGRESS_BAR ? SAVE_DATA_DIALOG_STATUS_RUNNING : SAVE_DATA_DIALOG_STATUS_FINISHED;
    }
    if (interactive) {
        try { SaveDialog::SaveDataDialogUiOpen_nid_no_patch(std::move(view), Complete); }
        catch (...) {
            std::lock_guard lock(stateMutex);
            status = SAVE_DATA_DIALOG_STATUS_INITIALIZED;
            names.clear();
            throw;
        }
    }
    return SAVE_DATA_DIALOG_OK;
}

int APS5_VABI sceSaveDataDialogClose(const void* closeParam) {
    (void)closeParam;
    std::lock_guard operation(operationMutex);
    {
        std::lock_guard lock(stateMutex);
        if (interactive && status != SAVE_DATA_DIALOG_STATUS_RUNNING) return SAVE_DATA_DIALOG_ERROR_NOT_RUNNING;
        if (interactive && mode != SAVE_DATA_DIALOG_MODE_PROGRESS_BAR) {
            resultCode = SAVE_DATA_DIALOG_RESULT_CANCELED;
            button = SAVE_DATA_DIALOG_BUTTON_ID_INVALID;
            selected = names.size();
        }
        status = SAVE_DATA_DIALOG_STATUS_FINISHED;
    }
    SaveDialog::SaveDataDialogUiClose_nid_no_patch();
    return SAVE_DATA_DIALOG_OK;
}

int APS5_VABI sceSaveDataDialogIsReadyToDisplay(void) {
 return 1;
}

int APS5_VABI sceSaveDataDialogTerminate(void) {
    std::lock_guard operation(operationMutex);
    SaveDialog::SaveDataDialogUiClose_nid_no_patch();
    std::lock_guard lock(stateMutex);
    status = SAVE_DATA_DIALOG_STATUS_NONE;
    mode = 0;
    userData = nullptr;
    names.clear();
    selected = 0;
    progress = 0;
    return SAVE_DATA_DIALOG_OK;
}

int APS5_VABI sceSaveDataDialogProgressBarInc(int target, std::uint32_t delta) {
    std::lock_guard operation(operationMutex);
    std::lock_guard lock(stateMutex);
    if (interactive) {
        if (status != SAVE_DATA_DIALOG_STATUS_RUNNING) return SAVE_DATA_DIALOG_ERROR_NOT_RUNNING;
        if (mode != SAVE_DATA_DIALOG_MODE_PROGRESS_BAR) return SAVE_DATA_DIALOG_ERROR_INVALID_STATE;
        if (target != 0) return SAVE_DATA_DIALOG_ERROR_INVALID_PARAMETER;
    }
    progress += std::min(delta, 100u - progress);
    SaveDialog::SaveDataDialogUiProgress_nid_no_patch(progress);
    return SAVE_DATA_DIALOG_OK;
}

int APS5_VABI sceSaveDataDialogProgressBarSetValue(int target, std::uint32_t rate) {
    std::lock_guard operation(operationMutex);
    std::lock_guard lock(stateMutex);
    if (interactive) {
        if (status != SAVE_DATA_DIALOG_STATUS_RUNNING) return SAVE_DATA_DIALOG_ERROR_NOT_RUNNING;
        if (mode != SAVE_DATA_DIALOG_MODE_PROGRESS_BAR) return SAVE_DATA_DIALOG_ERROR_INVALID_STATE;
        if (target != 0 || rate > 100) return SAVE_DATA_DIALOG_ERROR_INVALID_PARAMETER;
    }
    progress = std::min(rate, 100u);
    SaveDialog::SaveDataDialogUiProgress_nid_no_patch(progress);
    return SAVE_DATA_DIALOG_OK;
}

}
