#include "prx/libSceSaveDataDialog.native/SaveDataDialog.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <windows.h>
#include <commctrl.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <limits>
#include <thread>

extern "C" {
int APS5_VABI sceSaveDataDialogInitialize();
int APS5_VABI sceSaveDataDialogOpen(const void*);
int APS5_VABI sceSaveDataDialogGetStatus();
int APS5_VABI sceSaveDataDialogUpdateStatus();
int APS5_VABI sceSaveDataDialogGetResult(void*);
int APS5_VABI sceSaveDataDialogClose(const void*);
int APS5_VABI sceSaveDataDialogTerminate();
int APS5_VABI sceSaveDataDialogProgressBarInc(int, std::uint32_t);
int APS5_VABI sceSaveDataDialogProgressBarSetValue(int, std::uint32_t);
}

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        std::abort();
    }
}

template<typename TCondition>
void Await(TCondition condition, const char* message) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!condition()) {
        Require(std::chrono::steady_clock::now() < deadline, message);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

HWND FindOwnedWindow(const wchar_t* type, const wchar_t* title = nullptr) {
    struct Query { const wchar_t* type; const wchar_t* title; HWND found; } query{type, title, nullptr};
    EnumWindows([](HWND handle, LPARAM data) -> BOOL {
        auto& query = *reinterpret_cast<Query*>(data);
        DWORD process = 0;
        GetWindowThreadProcessId(handle, &process);
        if (process != GetCurrentProcessId()) return TRUE;
        wchar_t type[128]{}, title[128]{};
        GetClassNameW(handle, type, 128);
        GetWindowTextW(handle, title, 128);
        if (std::wcscmp(type, query.type) == 0 && (!query.title || std::wcscmp(title, query.title) == 0)) {
            query.found = handle;
            return FALSE;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&query));
    return query.found;
}

HWND Window() {
    HWND window = FindOwnedWindow(L"AnyPS5SaveDataDialog");
    Require(window != nullptr, "native dialog was not created");
    return window;
}

LRESULT Send(HWND window, UINT message, WPARAM wparam = 0, LPARAM lparam = 0) {
    DWORD_PTR result = 0;
    Require(SendMessageTimeoutW(window, message, wparam, lparam, SMTO_ABORTIFHUNG, 3000, &result) != 0, "native UI did not respond");
    return static_cast<LRESULT>(result);
}

SaveDataDialogResult Result(SaveDataDirName& name) {
    SaveDataDialogResult result{};
    result.dir_name = &name;
    Require(sceSaveDataDialogGetResult(&result) == 0, "result unavailable");
    return result;
}

}

int main(int argc, char** argv) {
    Require(SetEnvironmentVariableW(L"APS5_SAVEDATA_DIALOG_UI", L"1") != 0, "enable UI");
    Require(sceSaveDataDialogInitialize() == 0, "initialize");
    SaveDataDirName names[] = {{"slot-01"}, {"slot-02"}, {""}};
    SaveDataDialogItems items{};
    items.dir_names = names;
    items.dir_names_num = 3;
    int context = 42;
    SaveDataDialogParam param{};
    param.size = sizeof(param);
    param.mode = SAVE_DATA_DIALOG_MODE_LIST;
    param.disp_type = 2;
    param.items = &items;
    param.user_data = &context;

    if (argc > 1 && std::strcmp(argv[1], "--demo") == 0) {
        if (argc > 2 && std::strcmp(argv[2], "save") == 0) param.disp_type = 1;
        if (argc > 2 && std::strcmp(argv[2], "progress") == 0) param.mode = SAVE_DATA_DIALOG_MODE_PROGRESS_BAR;
        Require(sceSaveDataDialogOpen(&param) == 0, "open demo");
        if (param.mode == SAVE_DATA_DIALOG_MODE_PROGRESS_BAR) {
            for (unsigned value = 0; value <= 100; value += 5) {
                sceSaveDataDialogProgressBarSetValue(0, value);
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
            }
            sceSaveDataDialogClose(nullptr);
        }
        while (sceSaveDataDialogUpdateStatus() == SAVE_DATA_DIALOG_STATUS_RUNNING) std::this_thread::sleep_for(std::chrono::milliseconds(20));
        SaveDataDirName selected{};
        const auto result = Result(selected);
        std::printf("result=%d button=%d directory=%s\n", result.result, result.button_id, selected.data);
        sceSaveDataDialogTerminate();
        return 0;
    }

    Require(sceSaveDataDialogOpen(&param) == 0, "open load list");
    HWND window = Window();
    Require(sceSaveDataDialogGetStatus() == SAVE_DATA_DIALOG_STATUS_RUNNING, "list completed without input");
    SaveDataDialogResult pending{};
    Require(sceSaveDataDialogGetResult(&pending) == SAVE_DATA_DIALOG_ERROR_NOT_FINISHED, "running result accepted");
    Require(sceSaveDataDialogOpen(&param) == SAVE_DATA_DIALOG_ERROR_INVALID_STATE, "second open interrupted selection");
    HWND list = GetDlgItem(window, 100);
    Require(Send(list, LB_GETCOUNT) == 2, "empty directory was not filtered");
    PostMessageW(list, WM_KEYDOWN, VK_DOWN, 0);
    Await([&] { return Send(list, LB_GETCURSEL) == 1; }, "arrow navigation did not change selection");
    std::memset(names, 0, sizeof(names));
    PostMessageW(list, WM_KEYDOWN, VK_RETURN, 0);
    Await([] { return sceSaveDataDialogUpdateStatus() == SAVE_DATA_DIALOG_STATUS_FINISHED; }, "selection did not finish");
    SaveDataDirName selected{};
    auto result = Result(selected);
    Require(std::strcmp(selected.data, "slot-02") == 0 && result.user_data == &context && result.result == 0 && result.button_id == 1, "selection or snapshot was lost");

    std::snprintf(names[0].data, sizeof(names[0].data), "slot-01");
    for (const bool closeButton : {false, true}) {
        Require(sceSaveDataDialogOpen(&param) == 0, "reopen selection");
        window = Window();
        PostMessageW(window, closeButton ? WM_CLOSE : WM_KEYDOWN, closeButton ? 0 : VK_ESCAPE, 0);
        Await([] { return sceSaveDataDialogGetStatus() == SAVE_DATA_DIALOG_STATUS_FINISHED; }, "cancel did not finish");
        std::memset(selected.data, 'x', sizeof(selected.data));
        result = Result(selected);
        Require(result.result == SAVE_DATA_DIALOG_RESULT_CANCELED && result.button_id == 0 && selected.data[0] == 0, "cancel returned an accepted directory");
    }

    param.disp_type = 1;
    Require(sceSaveDataDialogOpen(&param) == 0, "open save list");
    window = Window();
    PostMessageW(window, WM_COMMAND, IDOK, 0);
    HWND confirmation = nullptr;
    Await([&] { confirmation = FindOwnedWindow(L"#32770", L"Confirm save"); return confirmation != nullptr; }, "missing save confirmation");
    Require(sceSaveDataDialogGetStatus() == SAVE_DATA_DIALOG_STATUS_RUNNING, "save completed before confirmation");
    PostMessageW(confirmation, WM_COMMAND, IDNO, 0);
    Await([&] { return !IsWindow(confirmation); }, "declining confirmation did not return to selection");
    Require(sceSaveDataDialogGetStatus() == SAVE_DATA_DIALOG_STATUS_RUNNING, "declining confirmation canceled selection");
    PostMessageW(window, WM_COMMAND, IDOK, 0);
    Await([&] { confirmation = FindOwnedWindow(L"#32770", L"Confirm save"); return confirmation != nullptr; }, "missing second confirmation");
    PostMessageW(confirmation, WM_COMMAND, IDYES, 0);
    Await([] { return sceSaveDataDialogGetStatus() == SAVE_DATA_DIALOG_STATUS_FINISHED; }, "save confirmation did not finish");
    Require(Result(selected).result == 0 && std::strcmp(selected.data, "slot-01") == 0, "save result incorrect");

    param.disp_type = 2;
    items.dir_names_num = 1;
    std::snprintf(names[0].data, sizeof(names[0].data), "slot-\xc3\xa9");
    Require(sceSaveDataDialogOpen(&param) == 0, "open Unicode name");
    window = Window();
    wchar_t unicodeName[32]{};
    Send(GetDlgItem(window, 100), LB_GETTEXT, 0, reinterpret_cast<LPARAM>(unicodeName));
    Require(std::wcscmp(unicodeName, L"slot-\u00e9") == 0, "Unicode name rendered incorrectly");
    PostMessageW(window, WM_COMMAND, IDOK, 0);
    Await([] { return sceSaveDataDialogGetStatus() == SAVE_DATA_DIALOG_STATUS_FINISHED; }, "Unicode selection did not finish");
    Require(Result(selected).result == 0 && std::strcmp(selected.data, "slot-\xc3\xa9") == 0, "Unicode result changed");

    items.dir_names_num = 0;
    Require(sceSaveDataDialogOpen(&param) == 0, "open empty list");
    window = Window();
    Require(!IsWindowEnabled(GetDlgItem(window, IDOK)), "empty list permits confirmation");
    Require(sceSaveDataDialogClose(nullptr) == 0, "close empty list");
    Require(Result(selected).result == SAVE_DATA_DIALOG_RESULT_CANCELED, "programmatic list close must cancel");

    param.mode = SAVE_DATA_DIALOG_MODE_PROGRESS_BAR;
    SaveDataDialogProgressBarParam bar{};
    bar.message = "Writing save data";
    param.prog_bar_param = &bar;
    Require(sceSaveDataDialogOpen(&param) == 0, "open progress");
    window = Window();
    wchar_t progressMessage[64]{};
    GetWindowTextW(GetDlgItem(window, 104), progressMessage, 64);
    Require(std::wcscmp(progressMessage, L"Writing save data") == 0, "caller progress message was not displayed");
    Require(sceSaveDataDialogProgressBarSetValue(1, 10) == SAVE_DATA_DIALOG_ERROR_INVALID_PARAMETER, "unknown progress target accepted");
    Require(sceSaveDataDialogProgressBarSetValue(0, 101) == SAVE_DATA_DIALOG_ERROR_INVALID_PARAMETER, "invalid progress percentage accepted");
    Require(sceSaveDataDialogProgressBarSetValue(0, 25) == 0, "set progress");
    Require(sceSaveDataDialogProgressBarInc(0, 25) == 0, "increment progress");
    Await([&] { return Send(GetDlgItem(window, 101), PBM_GETPOS) == 50; }, "progress not displayed");
    Require(sceSaveDataDialogProgressBarInc(0, std::numeric_limits<std::uint32_t>::max()) == 0, "saturate progress");
    Await([&] { return Send(GetDlgItem(window, 101), PBM_GETPOS) == 100; }, "progress overflowed");
    Send(window, WM_CLOSE);
    Require(sceSaveDataDialogUpdateStatus() == SAVE_DATA_DIALOG_STATUS_RUNNING, "progress dismissed before caller close");
    Require(sceSaveDataDialogClose(nullptr) == 0, "close progress");
    Require(Result(selected).result == 0, "progress close failed");

    param.mode = SAVE_DATA_DIALOG_MODE_LIST;
    param.prog_bar_param = nullptr;
    param.disp_type = 1;
    items.dir_names_num = 1;
    std::snprintf(names[0].data, sizeof(names[0].data), "slot-01");
    Require(sceSaveDataDialogOpen(&param) == 0, "open before terminate");
    PostMessageW(Window(), WM_COMMAND, IDOK, 0);
    Await([&] { confirmation = FindOwnedWindow(L"#32770", L"Confirm save"); return confirmation != nullptr; }, "missing confirmation before terminate");
    Require(sceSaveDataDialogTerminate() == 0, "terminate active confirmation");
    Require(FindOwnedWindow(L"AnyPS5SaveDataDialog") == nullptr, "terminate left window alive");
    Require(sceSaveDataDialogGetStatus() == SAVE_DATA_DIALOG_STATUS_NONE, "terminate left session alive");
    Require(sceSaveDataDialogInitialize() == 0, "reinitialize");
    param.size = 0;
    Require(sceSaveDataDialogOpen(&param) == SAVE_DATA_DIALOG_ERROR_INVALID_PARAMETER, "invalid ABI size accepted");
    param.size = sizeof(param);
    items.dir_names_num = 1;
    std::memset(names[0].data, 'x', sizeof(names[0].data));
    Require(sceSaveDataDialogOpen(&param) == SAVE_DATA_DIALOG_ERROR_INVALID_PARAMETER, "unterminated name accepted");
    Require(sceSaveDataDialogTerminate() == 0, "final terminate");
    std::puts("Native save-dialog tests passed");
}
