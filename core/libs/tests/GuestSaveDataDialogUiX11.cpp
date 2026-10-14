#include "prx/libSceSaveDataDialog.native/SaveDataDialog.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <initializer_list>
#include <stdexcept>
#include <thread>
#include <unistd.h>

extern "C" {
int APS5_VABI sceSaveDataDialogInitialize();
int APS5_VABI sceSaveDataDialogOpen(const void*);
int APS5_VABI sceSaveDataDialogGetStatus();
int APS5_VABI sceSaveDataDialogGetResult(void*);
int APS5_VABI sceSaveDataDialogClose(const void*);
int APS5_VABI sceSaveDataDialogTerminate();
int APS5_VABI sceSaveDataDialogProgressBarInc(int, std::uint32_t);
int APS5_VABI sceSaveDataDialogProgressBarSetValue(int, std::uint32_t);
}

namespace {

void Require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "%s\n", message); std::abort(); }
}

template<typename TCondition>
void Await(TCondition condition, const char* message) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!condition()) {
        Require(std::chrono::steady_clock::now() < deadline, message);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

bool IsDialog(Display* display, Window window) {
    XClassHint type{};
    if (!XGetClassHint(display, window, &type)) return false;
    const bool match = type.res_class && std::strcmp(type.res_class, "AnyPS5SaveDataDialog") == 0;
    if (type.res_name) XFree(type.res_name);
    if (type.res_class) XFree(type.res_class);
    if (!match) return false;
    Atom actual = 0;
    int format = 0;
    unsigned long items = 0, remaining = 0;
    unsigned char* data = nullptr;
    const int result = XGetWindowProperty(display, window, XInternAtom(display, "_NET_WM_PID", False),
        0, 1, False, XA_CARDINAL, &actual, &format, &items, &remaining, &data);
    const bool owned = result == Success && actual == XA_CARDINAL && format == 32 && items == 1
        && *reinterpret_cast<unsigned long*>(data) == static_cast<unsigned long>(getpid());
    if (data) XFree(data);
    return owned;
}

Window FindWindow(Display* display, Window ancestor = 0, int depth = 0) {
    if (!ancestor) ancestor = DefaultRootWindow(display);
    Window root = 0, parent = 0, *children = nullptr;
    unsigned count = 0;
    Require(XQueryTree(display, ancestor, &root, &parent, &children, &count), "query windows");
    Window found = 0;
    for (unsigned i = 0; i < count && !found; ++i) {
        if (IsDialog(display, children[i])) found = children[i];
        else if (depth < 8) found = FindWindow(display, children[i], depth + 1);
    }
    if (children) XFree(children);
    return found;
}

Window Open(Display* display, const SaveDataDialogParam& param) {
    Require(sceSaveDataDialogOpen(&param) == 0, "open dialog");
    Window window = 0;
    Await([&] { window = FindWindow(display); return window != 0; }, "X11 dialog not created");
    Await([&] {
        Window focused = 0;
        int revert = 0;
        XGetInputFocus(display, &focused, &revert);
        return focused == window;
    }, "dialog did not receive keyboard focus");
    Require(sceSaveDataDialogGetStatus() == SAVE_DATA_DIALOG_STATUS_RUNNING, "completed before input");
    SaveDataDialogResult pending{};
    Require(sceSaveDataDialogGetResult(&pending) == SAVE_DATA_DIALOG_ERROR_NOT_FINISHED, "result before input");
    return window;
}

void AwaitTitle(Display* display, Window window, const char* expected) {
    Await([&] {
        char* title = nullptr;
        const bool match = XFetchName(display, window, &title) && title && std::strcmp(title, expected) == 0;
        if (title) XFree(title);
        return match;
    }, "confirmation title did not change");
}

void Key(Display* display, Window window, KeySym key) {
    XEvent event{};
    event.xkey.type = KeyPress;
    event.xkey.display = display;
    event.xkey.window = window;
    event.xkey.root = DefaultRootWindow(display);
    event.xkey.same_screen = True;
    event.xkey.keycode = XKeysymToKeycode(display, key);
    Require(XSendEvent(display, window, False, KeyPressMask, &event), "send key");
    XSync(display, False);
}

void Click(Display* display, Window window, int x, int y) {
    XEvent event{};
    event.xbutton.type = ButtonPress;
    event.xbutton.display = display;
    event.xbutton.window = window;
    event.xbutton.root = DefaultRootWindow(display);
    event.xbutton.same_screen = True;
    event.xbutton.button = Button1;
    event.xbutton.x = x;
    event.xbutton.y = y;
    Require(XSendEvent(display, window, False, ButtonPressMask, &event), "send click");
    XSync(display, False);
}

void WindowClose(Display* display, Window window) {
    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.display = display;
    event.xclient.window = window;
    event.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", False);
    event.xclient.format = 32;
    event.xclient.data.l[0] = static_cast<long>(XInternAtom(display, "WM_DELETE_WINDOW", False));
    Require(XSendEvent(display, window, False, NoEventMask, &event), "send window close");
    XSync(display, False);
}

SaveDataDialogResult Result(SaveDataDirName& name) {
    Await([] { return sceSaveDataDialogGetStatus() == SAVE_DATA_DIALOG_STATUS_FINISHED; }, "dialog did not finish");
    SaveDataDialogResult result{};
    result.dir_name = &name;
    Require(sceSaveDataDialogGetResult(&result) == 0, "result unavailable");
    return result;
}

}

int main(int argc, char** argv) {
    Require(setenv("APS5_SAVEDATA_DIALOG_UI", "1", 1) == 0, "enable UI");
    SaveDataDirName names[] = {{"slot-01"}, {"slot-\xc3\xa9"}, {""}};
    SaveDataDialogItems items{};
    items.dir_names = names;
    items.dir_names_num = 3;
    SaveDataDialogParam param{};
    param.size = sizeof(param);
    param.mode = SAVE_DATA_DIALOG_MODE_LIST;
    param.disp_type = 2;
    param.items = &items;
    int context = 42;
    param.user_data = &context;
    Require(sceSaveDataDialogInitialize() == 0, "initialize");

    if (argc > 1 && std::strcmp(argv[1], "--no-display") == 0) {
        Require(setenv("DISPLAY", "", 1) == 0, "clear display");
        bool threw = false;
        try { sceSaveDataDialogOpen(&param); }
        catch (const std::runtime_error& error) { threw = std::strstr(error.what(), "cannot open X11 display") != nullptr; }
        Require(threw, "missing display did not give a clear error");
        Require(sceSaveDataDialogGetStatus() == SAVE_DATA_DIALOG_STATUS_INITIALIZED, "failed open corrupted status");
        Require(sceSaveDataDialogTerminate() == 0, "terminate after failed open");
        return 0;
    }
    if (argc > 1 && std::strcmp(argv[1], "--demo") == 0) {
        if (argc > 2 && std::strcmp(argv[2], "save") == 0) param.disp_type = 1;
        if (argc > 2 && std::strcmp(argv[2], "progress") == 0) param.mode = SAVE_DATA_DIALOG_MODE_PROGRESS_BAR;
        Require(sceSaveDataDialogOpen(&param) == 0, "demo open");
        if (param.mode == SAVE_DATA_DIALOG_MODE_PROGRESS_BAR) {
            for (unsigned i = 0; i <= 100; i += 5) {
                Require(sceSaveDataDialogProgressBarSetValue(0, i) == 0, "demo progress");
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
            }
            Require(sceSaveDataDialogClose(nullptr) == 0, "demo close");
        }
        while (sceSaveDataDialogGetStatus() == SAVE_DATA_DIALOG_STATUS_RUNNING) std::this_thread::sleep_for(std::chrono::milliseconds(20));
        SaveDataDirName selected{};
        const auto result = Result(selected);
        std::printf("result=%d button=%d directory=%s\n", result.result, result.button_id, selected.data);
        Require(sceSaveDataDialogTerminate() == 0, "demo terminate");
        return 0;
    }
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::puts("skipped, no display for X11 save dialog; run with xvfb-run -a");
        sceSaveDataDialogTerminate();
        return 77;
    }
    Window window = Open(display, param);
    Require(sceSaveDataDialogOpen(&param) == SAVE_DATA_DIALOG_ERROR_INVALID_STATE, "second open accepted");
    std::memset(names, 0, sizeof(names));
    Key(display, window, XK_Down);
    Key(display, window, XK_Return);
    SaveDataDirName selected{};
    auto result = Result(selected);
    Require(result.result == 0 && result.button_id == 1 && result.user_data == &context
        && std::strcmp(selected.data, "slot-\xc3\xa9") == 0, "Unicode selection or snapshot lost");
    std::snprintf(names[0].data, sizeof(names[0].data), "slot-01");
    for (const bool close : {false, true}) {
        window = Open(display, param);
        if (close) WindowClose(display, window);
        else Key(display, window, XK_Escape);
        std::memset(selected.data, 'x', sizeof(selected.data));
        result = Result(selected);
        Require(result.result == 1 && result.button_id == 0 && selected.data[0] == 0, "cancel returned a slot");
    }
    for (const int type : {1, 3}) {
        param.disp_type = type;
        window = Open(display, param);
        Key(display, window, XK_Return);
        const char* title = type == 3 ? "AnyPS5 - Confirm deletion" : "AnyPS5 - Confirm save";
        AwaitTitle(display, window, title);
        Require(sceSaveDataDialogGetStatus() == SAVE_DATA_DIALOG_STATUS_RUNNING, "accepted before confirmation");
        Key(display, window, XK_Return);
        AwaitTitle(display, window, "AnyPS5 - Save data");
        Require(sceSaveDataDialogGetStatus() == SAVE_DATA_DIALOG_STATUS_RUNNING, "default confirmation did not decline");
        Key(display, window, XK_Return);
        AwaitTitle(display, window, title);
        Key(display, window, XK_Tab);
        Key(display, window, XK_Return);
        Require(Result(selected).result == 0 && std::strcmp(selected.data, "slot-01") == 0, "confirmation or safe default failed");
    }
    param.disp_type = 2;
    window = Open(display, param);
    Click(display, window, 50, 115);
    Click(display, window, 360, 400);
    Require(Result(selected).result == 0, "mouse selection failed");
    SaveDataDirName many[40]{};
    for (unsigned i = 0; i < 40; ++i) std::snprintf(many[i].data, sizeof(many[i].data), "slot-%02u", i);
    items.dir_names = many;
    items.dir_names_num = 40;
    window = Open(display, param);
    Key(display, window, XK_End);
    Key(display, window, XK_Return);
    Result(selected);
    Require(std::strcmp(selected.data, "slot-39") == 0, "scroll to last slot");
    window = Open(display, param);
    Key(display, window, XK_Page_Down);
    Key(display, window, XK_Home);
    Key(display, window, XK_Return);
    Result(selected);
    Require(std::strcmp(selected.data, "slot-00") == 0, "scroll back to first slot");
    items.dir_names = names;
    items.dir_names_num = 0;
    window = Open(display, param);
    Click(display, window, 360, 400);
    Key(display, window, XK_Escape);
    Require(Result(selected).result == 1, "empty list accepted");
    items.dir_names_num = 1;
    window = Open(display, param);
    Require(sceSaveDataDialogClose(nullptr) == 0, "caller close list");
    Require(Result(selected).result == 1, "caller close accepted list");
    param.mode = SAVE_DATA_DIALOG_MODE_PROGRESS_BAR;
    SaveDataDialogProgressBarParam bar{};
    bar.message = "Restoring saved data. Please wait.";
    param.prog_bar_param = &bar;
    window = Open(display, param);
    Require(sceSaveDataDialogProgressBarSetValue(1, 0) == SAVE_DATA_DIALOG_ERROR_INVALID_PARAMETER, "bad target accepted");
    Require(sceSaveDataDialogProgressBarSetValue(0, 101) == SAVE_DATA_DIALOG_ERROR_INVALID_PARAMETER, "bad percent accepted");
    Require(sceSaveDataDialogProgressBarSetValue(0, 40) == 0, "set progress");
    Require(sceSaveDataDialogProgressBarInc(0, std::numeric_limits<std::uint32_t>::max()) == 0, "saturate progress");
    WindowClose(display, window);
    Key(display, window, XK_Escape);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    Require(sceSaveDataDialogGetStatus() == SAVE_DATA_DIALOG_STATUS_RUNNING, "progress finished without caller");
    Require(sceSaveDataDialogClose(nullptr) == 0, "close progress");
    Require(Result(selected).result == 0, "progress close failed");
    param.mode = SAVE_DATA_DIALOG_MODE_LIST;
    param.disp_type = 1;
    param.prog_bar_param = nullptr;
    window = Open(display, param);
    Key(display, window, XK_Return);
    AwaitTitle(display, window, "AnyPS5 - Confirm save");
    Require(sceSaveDataDialogTerminate() == 0, "terminate confirmation");
    Require(FindWindow(display) == 0, "terminate left a window");
    Require(sceSaveDataDialogInitialize() == 0, "reinitialize");
    param.disp_type = 2;
    window = Open(display, param);
    Key(display, window, XK_Return);
    Require(Result(selected).result == 0, "reopen after terminate");
    Require(sceSaveDataDialogTerminate() == 0, "terminate");
    Require(sceSaveDataDialogInitialize() == 0, "initialize for invalid UTF-8");
    names[0].data[0] = static_cast<char>(0xc0);
    names[0].data[1] = 0;
    bool threw = false;
    try { sceSaveDataDialogOpen(&param); }
    catch (const std::runtime_error& error) { threw = std::strstr(error.what(), "not UTF-8") != nullptr; }
    Require(threw && sceSaveDataDialogGetStatus() == SAVE_DATA_DIALOG_STATUS_INITIALIZED, "invalid UTF-8 was accepted or corrupted state");
    Require(sceSaveDataDialogTerminate() == 0, "terminate invalid UTF-8");
    XCloseDisplay(display);
    return 0;
}
