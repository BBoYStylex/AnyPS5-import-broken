#include "DialogWindow.hpp"
#include "SaveDataDialog.hpp"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <future>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>
#include <unistd.h>

namespace SaveDialog {
namespace {

const bool threadsReady = XInitThreads() != 0;

std::vector<XChar2b> Glyphs(std::string_view text) {
    std::vector<XChar2b> result;
    for (std::size_t i = 0; i < text.size();) {
        const auto lead = static_cast<unsigned char>(text[i]);
        const unsigned length = lead < 0x80 ? 1 : lead >= 0xc2 && lead <= 0xdf ? 2
            : lead >= 0xe0 && lead <= 0xef ? 3 : lead >= 0xf0 && lead <= 0xf4 ? 4 : 0;
        if (length == 0 || text.size() - i < length) throw std::runtime_error("SaveDataDialog: text is not UTF-8");
        std::uint32_t code = lead & (length == 1 ? 0x7f : length == 2 ? 0x1f : length == 3 ? 0x0f : 0x07);
        for (unsigned j = 1; j < length; ++j) {
            const auto next = static_cast<unsigned char>(text[i + j]);
            if ((next & 0xc0) != 0x80) throw std::runtime_error("SaveDataDialog: text is not UTF-8");
            code = (code << 6) | (next & 0x3f);
        }
        if ((length == 2 && code < 0x80) || (length == 3 && code < 0x800) || (length == 4 && code < 0x10000)
            || (code >= 0xd800 && code <= 0xdfff) || code > 0x10ffff) throw std::runtime_error("SaveDataDialog: text is not UTF-8");
        if (code > 0xffff) code = 0xfffd;
        result.push_back({static_cast<unsigned char>(code >> 8), static_cast<unsigned char>(code & 0xff)});
        i += length;
    }
    return result;
}

struct WindowState {
    Display* display = nullptr;
    Window window = 0;
    GC pen = nullptr;
    XFontStruct* font = nullptr;
    Atom closeMessage = 0;
    View view;
    std::function<void(bool, std::size_t)> complete;
    int width = 560;
    int height = 440;
    int focus = 0;
    std::size_t selected = 0;
    std::size_t first = 0;
    bool confirming = false;
    bool finished = false;
    unsigned long ink = 0;
    unsigned long paper = 0;
    unsigned long accent = 0;
    unsigned long muted = 0;
    unsigned long selection = 0;

    ~WindowState() {
        if (!display) return;
        if (font) XFreeFont(display, font);
        if (pen) XFreeGC(display, pen);
        if (window) XDestroyWindow(display, window);
        XCloseDisplay(display);
    }

    bool Progress() const { return view.Mode == SAVE_DATA_DIALOG_MODE_PROGRESS_BAR; }
    const char* Action() const { return view.DisplayType == 1 ? "Save" : view.DisplayType == 3 ? "Delete" : "Load"; }
    int Rows() const { return std::max(1, (height - 176) / 32); }

    unsigned long Color(const char* name) const {
        XColor color{};
        if (!XParseColor(display, DefaultColormap(display, DefaultScreen(display)), name, &color)
            || !XAllocColor(display, DefaultColormap(display, DefaultScreen(display)), &color))
            throw std::runtime_error("SaveDataDialog: cannot allocate X11 colors");
        return color.pixel;
    }

    void Text(int x, int y, const std::string& text, unsigned long color) const {
        XSetForeground(display, pen, color);
        XRectangle clip{static_cast<short>(x), static_cast<short>(y - 20), static_cast<unsigned short>(width - x - 24), 32};
        XSetClipRectangles(display, pen, 0, 0, &clip, 1, Unsorted);
        const auto glyphs = Glyphs(text);
        XDrawString16(display, window, pen, x, y, glyphs.data(), static_cast<int>(glyphs.size()));
        XSetClipMask(display, pen, None);
    }

    void Box(int x, int y, int w, int h, unsigned long color) const {
        XSetForeground(display, pen, color);
        XFillRectangle(display, window, pen, x, y, static_cast<unsigned>(w), static_cast<unsigned>(h));
    }

    void Border(int x, int y, int w, int h, unsigned long color) const {
        XSetForeground(display, pen, color);
        XDrawRectangle(display, window, pen, x, y, static_cast<unsigned>(w), static_cast<unsigned>(h));
    }

    void Button(int x, const char* text, bool focused, bool enabled) const {
        Box(x, height - 56, 96, 32, paper);
        Border(x, height - 56, 96, 32, focused ? accent : muted);
        if (focused) Border(x + 2, height - 54, 92, 28, accent);
        Text(x + 12, height - 35, text, enabled ? ink : muted);
    }

    void Draw(std::uint32_t percent) {
        XStoreName(display, window, confirming ? (view.DisplayType == 3 ? "AnyPS5 - Confirm deletion" : "AnyPS5 - Confirm save") : "AnyPS5 - Save data");
        Box(0, 0, width, height, paper);
        Text(24, 40, Progress() ? (view.DisplayType == 2 ? "Loading game" : "Saving game")
            : confirming ? (view.DisplayType == 3 ? "Confirm deletion" : "Confirm save")
            : view.DisplayType == 1 ? "Save game" : view.DisplayType == 3 ? "Delete save" : "Load game", ink);
        if (Progress()) {
            Text(24, 78, view.Message.empty() ? "Please wait until the operation completes." : view.Message, ink);
            Border(24, 108, width - 48, 22, muted);
            if (percent != 0) Box(26, 110, std::max(1, (width - 52) * static_cast<int>(percent) / 100), 18, accent);
            Text(24, 164, std::to_string(percent) + "%", ink);
        } else if (confirming) {
            Text(24, 78, view.DisplayType == 3 ? "Delete the save in this slot?" : "Save to this slot?", ink);
            Text(24, 120, view.Names[selected], ink);
            Button(width - 232, "Yes", focus == 1, true);
            Button(width - 120, "No", focus == 2, true);
        } else {
            Text(24, 78, view.Names.empty() ? "No save slots are available. Cancel to return." : "Choose a save slot, then confirm your selection.", ink);
            Border(24, 100, width - 48, height - 176, focus == 0 ? accent : muted);
            const auto end = std::min(view.Names.size(), first + static_cast<std::size_t>(Rows()));
            for (std::size_t i = first; i < end; ++i) {
                const int y = 102 + static_cast<int>(i - first) * 32;
                if (i == selected) Box(26, y, width - 52, 32, selection);
                Text(36, y + 22, view.Names[i], ink);
            }
            Button(width - 232, Action(), focus == 1, !view.Names.empty());
            Button(width - 120, "Cancel", focus == 2, true);
        }
        XFlush(display);
    }

    void Select(std::size_t index) {
        if (view.Names.empty()) return;
        selected = std::min(index, view.Names.size() - 1);
        if (selected < first) first = selected;
        if (selected >= first + static_cast<std::size_t>(Rows())) first = selected - Rows() + 1;
    }

    void Finish(bool accepted) {
        if (finished) return;
        if (accepted && view.Names.empty()) return;
        if (accepted && view.DisplayType != 2 && !confirming) {
            confirming = true;
            focus = 2;
            return;
        }
        finished = true;
        complete(accepted, accepted ? selected : 0);
    }

    void Activate() {
        if (confirming) {
            if (focus == 1) Finish(true);
            else { confirming = false; focus = 0; }
        } else Finish(focus != 2);
    }

    void Key(KeySym key, bool backwards) {
        if (Progress()) return;
        if (key == XK_Escape) {
            if (confirming) { confirming = false; focus = 0; }
            else Finish(false);
        } else if (key == XK_Tab || key == XK_ISO_Left_Tab) {
            if (confirming) focus = focus == 1 ? 2 : 1;
            else {
                focus = (focus + (backwards || key == XK_ISO_Left_Tab ? 2 : 1)) % 3;
                if (view.Names.empty() && focus != 2) focus = 2;
            }
        } else if (key == XK_Return || key == XK_KP_Enter || (key == XK_space && focus != 0)) Activate();
        else if (!confirming && focus == 0 && !view.Names.empty()) {
            if (key == XK_Down) Select(selected + 1);
            else if (key == XK_Up) Select(selected == 0 ? 0 : selected - 1);
            else if (key == XK_Home) Select(0);
            else if (key == XK_End) Select(view.Names.size() - 1);
            else if (key == XK_Page_Down) Select(selected + Rows());
            else if (key == XK_Page_Up) Select(selected > static_cast<std::size_t>(Rows()) ? selected - Rows() : 0);
        }
    }

    void Mouse(const XButtonEvent& event) {
        if (Progress()) return;
        if (!confirming && (event.button == Button4 || event.button == Button5)) {
            if (event.button == Button4) Select(selected == 0 ? 0 : selected - 1);
            else Select(selected + 1);
            return;
        }
        if (event.button != Button1) return;
        if (event.y >= height - 56 && event.y <= height - 24) {
            if (event.x >= width - 232 && event.x <= width - 136) { focus = 1; Activate(); }
            else if (event.x >= width - 120 && event.x <= width - 24) { focus = 2; Activate(); }
        } else if (!confirming && event.x >= 24 && event.x < width - 24 && event.y >= 102 && event.y < height - 76) {
            const auto index = first + static_cast<std::size_t>((event.y - 102) / 32);
            if (index < view.Names.size()) { Select(index); focus = 0; }
        }
    }

    void Create() {
        for (const auto& name : view.Names) Glyphs(name);
        Glyphs(view.Message);
        display = XOpenDisplay(nullptr);
        if (!display) throw std::runtime_error("SaveDataDialog: cannot open X11 display; set DISPLAY or enable XWayland");
        const int screen = DefaultScreen(display);
        height = Progress() ? 240 : 440;
        paper = Color("#ffffff");
        ink = Color("#17212b");
        accent = Color("#155ab0");
        muted = Color("#707880");
        selection = Color("#dbeafe");
        window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, width, height, 0, ink, paper);
        if (!window) throw std::runtime_error("SaveDataDialog: cannot create X11 window");
        XStoreName(display, window, "AnyPS5 - Save data");
        XClassHint type{const_cast<char*>("anyps5-savedata"), const_cast<char*>("AnyPS5SaveDataDialog")};
        XSetClassHint(display, window, &type);
        const unsigned long process = static_cast<unsigned long>(getpid());
        XChangeProperty(display, window, XInternAtom(display, "_NET_WM_PID", False), XA_CARDINAL, 32, PropModeReplace,
            reinterpret_cast<const unsigned char*>(&process), 1);
        XSizeHints sizes{};
        sizes.flags = PMinSize;
        sizes.min_width = 440;
        sizes.min_height = Progress() ? 240 : 360;
        XSetWMNormalHints(display, window, &sizes);
        closeMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display, window, &closeMessage, 1);
        XSelectInput(display, window, ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask);
        font = XLoadQueryFont(display, "-misc-fixed-medium-r-normal--16-*-*-*-*-*-iso10646-1");
        if (!font) font = XLoadQueryFont(display, "fixed");
        if (!font) throw std::runtime_error("SaveDataDialog: no X11 font is available");
        pen = XCreateGC(display, window, 0, nullptr);
        if (!pen) throw std::runtime_error("SaveDataDialog: cannot create X11 graphics context");
        XSetFont(display, pen, font->fid);
        if (view.Names.empty()) focus = 2;
        XMapRaised(display, window);
        XSync(display, False);
    }
};

class DialogWindow {
public:
    ~DialogWindow() { Close(); }

    void Open(View view, std::function<void(bool, std::size_t)> complete) {
        Close();
        if (!threadsReady) throw std::runtime_error("SaveDataDialog: X11 thread initialization failed");
        closing.store(false);
        progress.store(0);
        std::promise<void> ready;
        auto initialized = ready.get_future();
        worker = std::thread([this, view = std::move(view), complete = std::move(complete), ready = std::move(ready)]() mutable {
            bool readySent = false;
            try {
                WindowState state;
                state.view = std::move(view);
                state.complete = std::move(complete);
                state.Create();
                state.Draw(0);
                ready.set_value();
                readySent = true;
                std::uint32_t drawnProgress = 0;
                while (!closing.load() && !state.finished) {
                    bool redraw = false;
                    while (!closing.load() && !state.finished && XPending(state.display)) {
                        XEvent event{};
                        XNextEvent(state.display, &event);
                        if (event.type == Expose) redraw = true;
                        else if (event.type == MapNotify) {
                            XWindowAttributes attributes{};
                            if (XGetWindowAttributes(state.display, state.window, &attributes) && attributes.map_state == IsViewable)
                                XSetInputFocus(state.display, state.window, RevertToParent, CurrentTime);
                        } else if (event.type == ConfigureNotify) {
                            state.width = std::max(440, event.xconfigure.width);
                            state.height = std::max(state.Progress() ? 240 : 360, event.xconfigure.height);
                            state.Select(state.selected);
                            redraw = true;
                        } else if (event.type == KeyPress) {
                            state.Key(XLookupKeysym(&event.xkey, 0), (event.xkey.state & ShiftMask) != 0);
                            redraw = true;
                        } else if (event.type == ButtonPress) { state.Mouse(event.xbutton); redraw = true; }
                        else if (event.type == ClientMessage && event.xclient.message_type == XInternAtom(state.display, "WM_PROTOCOLS", False)
                            && static_cast<Atom>(event.xclient.data.l[0]) == state.closeMessage && !state.Progress()) state.Finish(false);
                    }
                    const auto value = progress.load();
                    if (redraw || value != drawnProgress) { state.Draw(value); drawnProgress = value; }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            } catch (...) {
                if (!readySent) ready.set_exception(std::current_exception());
                else std::terminate();
            }
        });
        try { initialized.get(); }
        catch (...) { Close(); throw; }
    }

    void Close() {
        closing.store(true);
        if (worker.joinable()) worker.join();
    }

    void SetProgress(std::uint32_t value) { progress.store(value); }

private:
    std::thread worker;
    std::atomic<bool> closing{false};
    std::atomic<std::uint32_t> progress{0};
};

DialogWindow& NativeWindow() {
    static DialogWindow window;
    return window;
}

}

extern "C" bool SaveDataDialogUiEnabled_nid_no_patch() {
    const char* value = std::getenv("APS5_SAVEDATA_DIALOG_UI");
    return value && std::strcmp(value, "1") == 0;
}
extern "C" void SaveDataDialogUiOpen_nid_no_patch(View view, std::function<void(bool, std::size_t)> complete) {
    NativeWindow().Open(std::move(view), std::move(complete));
}
extern "C" void SaveDataDialogUiClose_nid_no_patch() { NativeWindow().Close(); }
extern "C" void SaveDataDialogUiProgress_nid_no_patch(std::uint32_t value) { NativeWindow().SetProgress(value); }

}
