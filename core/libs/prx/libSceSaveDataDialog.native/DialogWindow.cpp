#include "DialogWindow.hpp"
#include "SaveDataDialog.hpp"
#include <future>
#include <atomic>
#include <thread>
#include <stdexcept>
#include <exception>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commctrl.h>
#include <cwchar>

namespace SaveDialog {
namespace {

class DialogWindow {
public:
    ~DialogWindow();
    void Open(View view, std::function<void(bool, std::size_t)> complete);
    void Close();
    void SetProgress(std::uint32_t value);
private:
    std::thread worker;
    std::atomic<void*> window{nullptr};
};

constexpr UINT CloseMessage = WM_APP + 1;
constexpr UINT ProgressMessage = WM_APP + 2;
constexpr int ListId = 100;
constexpr int ProgressId = 101;
constexpr int PercentId = 102;
constexpr wchar_t WindowClass[] = L"AnyPS5SaveDataDialog";

std::wstring Wide(const std::string& text) {
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (!text.empty() && count == 0) throw std::runtime_error("SaveDataDialog: directory name is not UTF-8");
    std::wstring result(count, L'\0');
    if (count != 0) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), count);
    return result;
}

struct WindowState {
    View view;
    std::vector<std::wstring> names;
    std::wstring message;
    std::function<void(bool, std::size_t)> complete;
    std::atomic<void*>* published = nullptr;
    HWND list = nullptr;
    HWND progress = nullptr;
    HWND percent = nullptr;
    HWND primary = nullptr;
    HWND cancel = nullptr;
    HWND heading = nullptr;
    HWND instruction = nullptr;
    HFONT font = nullptr;
    HFONT headingFont = nullptr;
    bool finished = false;

    const wchar_t* Action() const {
        return view.DisplayType == 1 ? L"Save" : view.DisplayType == 3 ? L"Delete" : L"Load";
    }

    void Layout(HWND handle) const {
        RECT area{};
        GetClientRect(handle, &area);
        const int width = area.right;
        const int height = area.bottom;
        const int margin = 24;
        MoveWindow(heading, margin, 20, width - 2 * margin, 28, TRUE);
        MoveWindow(instruction, margin, 56, width - 2 * margin, 36, TRUE);
        if (list) MoveWindow(list, margin, 100, width - 2 * margin, height - 176, TRUE);
        if (progress) MoveWindow(progress, margin, 108, width - 2 * margin, 22, TRUE);
        if (percent) MoveWindow(percent, margin, 144, width - 2 * margin, 24, TRUE);
        if (primary) MoveWindow(primary, width - margin - 208, height - 56, 96, 32, TRUE);
        if (cancel) MoveWindow(cancel, width - margin - 96, height - 56, 96, 32, TRUE);
    }

    HWND Control(HWND parent, const wchar_t* type, const wchar_t* text, DWORD style, int id) const {
        HWND handle = CreateWindowExW(std::wcscmp(type, WC_LISTBOXW) == 0 ? WS_EX_CLIENTEDGE : 0, type, text,
            WS_CHILD | WS_VISIBLE | style, 0, 0, 1, 1, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
        if (handle) SendMessageW(handle, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        return handle;
    }

    void Finish(HWND handle, bool accepted) {
        if (finished) return;
        const LRESULT selected = list ? SendMessageW(list, LB_GETCURSEL, 0, 0) : LB_ERR;
        if (accepted && (selected == LB_ERR || static_cast<std::size_t>(selected) >= names.size())) return;
        if (accepted && view.DisplayType != 2) {
            wchar_t message[256]{};
            std::swprintf(message, std::size(message), view.DisplayType == 3 ? L"Delete the save in slot \"%ls\"?" : L"Save to slot \"%ls\"?", names[selected].c_str());
            if (MessageBoxW(handle, message, view.DisplayType == 3 ? L"Confirm deletion" : L"Confirm save", MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) != IDYES) return;
        }
        finished = true;
        complete(accepted, accepted ? static_cast<std::size_t>(selected) : 0);
        DestroyWindow(handle);
    }
};

LRESULT CALLBACK Procedure(HWND handle, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* state = reinterpret_cast<WindowState*>(GetWindowLongPtrW(handle, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        state = static_cast<WindowState*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }
    if (!state) return DefWindowProcW(handle, message, wparam, lparam);
    switch (message) {
    case WM_CREATE: {
        NONCLIENTMETRICSW metrics{};
        metrics.cbSize = sizeof(metrics);
        if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)) return -1;
        metrics.lfMessageFont.lfHeight = -16;
        state->font = CreateFontIndirectW(&metrics.lfMessageFont);
        metrics.lfMessageFont.lfHeight = -22;
        metrics.lfMessageFont.lfWeight = FW_SEMIBOLD;
        state->headingFont = CreateFontIndirectW(&metrics.lfMessageFont);
        if (!state->font || !state->headingFont) return -1;
        const bool progress = state->view.Mode == SAVE_DATA_DIALOG_MODE_PROGRESS_BAR;
        const wchar_t* heading = progress ? (state->view.DisplayType == 2 ? L"Loading game" : L"Saving game")
            : state->view.DisplayType == 1 ? L"Save game" : state->view.DisplayType == 3 ? L"Delete save" : L"Load game";
        state->heading = state->Control(handle, WC_STATICW, heading, SS_LEFT, 103);
        SendMessageW(state->heading, WM_SETFONT, reinterpret_cast<WPARAM>(state->headingFont), TRUE);
        const wchar_t* help = progress ? L"Please wait until the operation completes."
            : state->names.empty() ? L"No save slots are available. Cancel to return to the game."
            : L"Choose a save slot, then confirm your selection.";
        state->instruction = state->Control(handle, WC_STATICW, progress && !state->message.empty() ? state->message.c_str() : help, SS_LEFT, 104);
        if (progress) {
            state->progress = state->Control(handle, PROGRESS_CLASSW, L"", PBS_SMOOTH, ProgressId);
            state->percent = state->Control(handle, WC_STATICW, L"0%", SS_LEFT, PercentId);
            SendMessageW(state->progress, PBM_SETRANGE32, 0, 100);
            EnableMenuItem(GetSystemMenu(handle, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
            if (!state->progress || !state->percent) return -1;
        } else {
            state->list = state->Control(handle, WC_LISTBOXW, L"", WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, ListId);
            for (const auto& name : state->names) {
                if (SendMessageW(state->list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str())) < 0) return -1;
            }
            state->primary = state->Control(handle, WC_BUTTONW, state->Action(), WS_TABSTOP | BS_DEFPUSHBUTTON, IDOK);
            state->cancel = state->Control(handle, WC_BUTTONW, L"Cancel", WS_TABSTOP | BS_PUSHBUTTON, IDCANCEL);
            if (!state->names.empty()) SendMessageW(state->list, LB_SETCURSEL, 0, 0);
            EnableWindow(state->primary, !state->names.empty());
            if (!state->list || !state->primary || !state->cancel) return -1;
        }
        if (!state->heading || !state->instruction) return -1;
        state->Layout(handle);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        auto dc = reinterpret_cast<HDC>(wparam);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
        return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
    }
    case WM_SIZE:
        state->Layout(handle);
        return 0;
    case WM_GETMINMAXINFO: {
        auto* limits = reinterpret_cast<MINMAXINFO*>(lparam);
        limits->ptMinTrackSize = {440, state->view.Mode == SAVE_DATA_DIALOG_MODE_PROGRESS_BAR ? 240 : 360};
        return 0;
    }
    case WM_COMMAND:
        if (state->view.Mode == SAVE_DATA_DIALOG_MODE_PROGRESS_BAR) return 0;
        if (LOWORD(wparam) == IDOK || (LOWORD(wparam) == ListId && HIWORD(wparam) == LBN_DBLCLK)) state->Finish(handle, true);
        else if (LOWORD(wparam) == IDCANCEL) state->Finish(handle, false);
        return 0;
    case WM_CLOSE:
        if (state->view.Mode != SAVE_DATA_DIALOG_MODE_PROGRESS_BAR) state->Finish(handle, false);
        return 0;
    case CloseMessage:
        EnumThreadWindows(GetCurrentThreadId(), [](HWND other, LPARAM owner) -> BOOL {
            if (other != reinterpret_cast<HWND>(owner) && GetWindow(other, GW_OWNER) == reinterpret_cast<HWND>(owner)) EndDialog(other, IDCANCEL);
            return TRUE;
        }, reinterpret_cast<LPARAM>(handle));
        DestroyWindow(handle);
        return 0;
    case ProgressMessage: {
        SendMessageW(state->progress, PBM_SETPOS, wparam, 0);
        wchar_t text[16]{};
        std::swprintf(text, std::size(text), L"%u%%", static_cast<unsigned>(wparam));
        SetWindowTextW(state->percent, text);
        return 0;
    }
    case WM_DESTROY:
        if (state->font) DeleteObject(state->font);
        if (state->headingFont) DeleteObject(state->headingFont);
        state->published->store(nullptr);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(handle, message, wparam, lparam);
}


bool Enabled() {
    char value[8]{};
    return GetEnvironmentVariableA("APS5_SAVEDATA_DIALOG_UI", value, sizeof(value)) == 1 && value[0] == '1';
}

DialogWindow::~DialogWindow() { Close(); }

void DialogWindow::Open(View view, std::function<void(bool, std::size_t)> complete) {
    Close();
    std::promise<void> ready;
    auto initialized = ready.get_future();
    worker = std::thread([this, view = std::move(view), complete = std::move(complete), ready = std::move(ready)]() mutable {
        bool readySent = false;
        try {
            WindowState state;
            state.view = std::move(view);
            state.complete = std::move(complete);
            state.published = &window;
            for (const auto& name : state.view.Names) state.names.push_back(Wide(name));
            state.message = Wide(state.view.Message);
            INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_PROGRESS_CLASS};
            if (!InitCommonControlsEx(&controls)) throw std::runtime_error("SaveDataDialog: cannot initialize Windows controls");
            WNDCLASSW type{};
            type.lpfnWndProc = Procedure;
            type.hInstance = GetModuleHandleW(nullptr);
            type.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
            type.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            type.lpszClassName = WindowClass;
            if (!RegisterClassW(&type) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) throw std::runtime_error("SaveDataDialog: cannot register dialog window");
            const bool progress = state.view.Mode == SAVE_DATA_DIALOG_MODE_PROGRESS_BAR;
            HWND handle = CreateWindowExW(WS_EX_CONTROLPARENT, WindowClass, L"AnyPS5 - Save data", WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, 560, progress ? 260 : 440, nullptr, nullptr, type.hInstance, &state);
            if (!handle) throw std::runtime_error("SaveDataDialog: cannot create dialog window");
            window.store(handle);
            ShowWindow(handle, SW_SHOWNORMAL);
            UpdateWindow(handle);
            if (state.list) SetFocus(state.names.empty() ? state.cancel : state.list);
            ready.set_value();
            readySent = true;
            MSG message{};
            int result;
            while ((result = GetMessageW(&message, nullptr, 0, 0)) > 0) {
                if (!IsDialogMessageW(handle, &message)) {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
            }
            if (result < 0 && IsWindow(handle)) state.Finish(handle, false);
            UnregisterClassW(WindowClass, type.hInstance);
        } catch (...) {
            UnregisterClassW(WindowClass, GetModuleHandleW(nullptr));
            if (!readySent) ready.set_exception(std::current_exception());
            else std::terminate();
        }
    });
    try { initialized.get(); }
    catch (...) { Close(); throw; }
}

void DialogWindow::Close() {
    if (auto handle = static_cast<HWND>(window.load())) PostMessageW(handle, CloseMessage, 0, 0);
    if (worker.joinable()) worker.join();
}

void DialogWindow::SetProgress(std::uint32_t value) {
    if (auto handle = static_cast<HWND>(window.load())) PostMessageW(handle, ProgressMessage, value, 0);
}

DialogWindow& NativeWindow() {
    static DialogWindow window;
    return window;
}

}

extern "C" bool SaveDataDialogUiEnabled_nid_no_patch() { return Enabled(); }
extern "C" void SaveDataDialogUiOpen_nid_no_patch(View view, std::function<void(bool, std::size_t)> complete) {
    NativeWindow().Open(std::move(view), std::move(complete));
}
extern "C" void SaveDataDialogUiClose_nid_no_patch() { NativeWindow().Close(); }
extern "C" void SaveDataDialogUiProgress_nid_no_patch(std::uint32_t value) { NativeWindow().SetProgress(value); }

}
#elif !defined(__linux__)
#include <cstdlib>
#include <cstring>
namespace SaveDialog {
extern "C" bool SaveDataDialogUiEnabled_nid_no_patch() {
    const char* value = std::getenv("APS5_SAVEDATA_DIALOG_UI");
    return value && std::strcmp(value, "1") == 0;
}
extern "C" void SaveDataDialogUiOpen_nid_no_patch(View, std::function<void(bool, std::size_t)>) {
    throw std::runtime_error("SaveDataDialog: interactive dialogs are unavailable on this platform");
}
extern "C" void SaveDataDialogUiClose_nid_no_patch() {}
extern "C" void SaveDataDialogUiProgress_nid_no_patch(std::uint32_t) {}
}
#endif
