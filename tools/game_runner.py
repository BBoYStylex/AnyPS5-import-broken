import argparse
import base64
import contextlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

STATE_FILE_NAME = ".game_runner_state.json"
DEFAULT_PAD_BUTTONS = {
    "l3": 0x2, "r3": 0x4, "options": 0x8,
    "up": 0x10, "right": 0x20, "down": 0x40, "left": 0x80,
    "l2": 0x100, "r2": 0x200, "l1": 0x400, "r1": 0x800,
    "triangle": 0x1000, "circle": 0x2000, "cross": 0x4000,
    "square": 0x8000, "touchpad": 0x100000, "release": 0,
}


def global_state_fallback():
    import tempfile
    return Path(tempfile.gettempdir()) / f"anyps5_{STATE_FILE_NAME}"


def default_state_path(workdir=None):
    if workdir:
        return Path(workdir) / STATE_FILE_NAME
    local = Path.cwd() / STATE_FILE_NAME
    if local.is_file():
        return local
    sibling_run = Path.cwd().parent / "run" / STATE_FILE_NAME
    if sibling_run.is_file():
        return sibling_run
    child_run = Path.cwd() / "run" / STATE_FILE_NAME
    if child_run.is_file():
        return child_run
    fallback = global_state_fallback()
    if fallback.is_file():
        return fallback
    return local


def init_dpi_awareness():
    if os.name != "nt":
        return
    try:
        import ctypes
        ctypes.windll.user32.SetProcessDpiAwarenessContext(ctypes.c_void_p(-4))
    except Exception:
        pass


def attach_input_desktop():
    if os.name != "nt":
        return False
    init_dpi_awareness()
    try:
        import ctypes
        user32 = ctypes.windll.user32
        hdesk = user32.OpenInputDesktop(0, False, 0x01FF)
        if hdesk:
            user32.SetThreadDesktop(hdesk)
            return True
    except Exception:
        pass
    return False


def get_all_desktops():
    if os.name != "nt":
        return []
    try:
        import ctypes
        from ctypes import wintypes
        user32 = ctypes.windll.user32
        desktops = []
        DESKTOPENUMPROC = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.LPCWSTR, wintypes.LPARAM)

        def desk_cb(name, _):
            desktops.append(name)
            return True

        winsta0 = user32.OpenWindowStationW("WinSta0", False, 0x037F)
        if winsta0:
            user32.EnumDesktopsW(winsta0, DESKTOPENUMPROC(desk_cb), 0)
            user32.CloseWindowStation(winsta0)
        if not desktops:
            desktops = ["Default"]
        return desktops
    except Exception:
        return ["Default"]


def find_windows_for_pid(pid):
    if os.name != "nt":
        return []
    init_dpi_awareness()
    try:
        import ctypes
        from ctypes import wintypes
        user32 = ctypes.windll.user32
        WNDENUMPROC = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        desktops = get_all_desktops()
        found = []
        for desk_name in desktops:
            hdesk = user32.OpenDesktopW(desk_name, 0, False, 0x01FF)
            if not hdesk:
                continue

            def enum_cb(hwnd, _):
                if user32.IsWindowVisible(hwnd):
                    win_pid = wintypes.DWORD()
                    user32.GetWindowThreadProcessId(hwnd, ctypes.byref(win_pid))
                    if win_pid.value == pid:
                        length = user32.GetWindowTextLengthW(hwnd)
                        title = ""
                        if length > 0:
                            buf = ctypes.create_unicode_buffer(length + 1)
                            user32.GetWindowTextW(hwnd, buf, length + 1)
                            title = buf.value
                        found.append((hwnd, title))
                return True

            user32.EnumDesktopWindows(hdesk, WNDENUMPROC(enum_cb), 0)
            user32.CloseDesktop(hdesk)
        return found
    except Exception:
        return []


def get_window_bounds(hwnd):
    if os.name != "nt":
        return None
    try:
        import ctypes
        from ctypes import wintypes
        user32 = ctypes.windll.user32
        rect = wintypes.RECT()

        try:
            dwmapi = ctypes.windll.dwmapi
            DWMWA_EXTENDED_FRAME_BOUNDS = 9
            hr = dwmapi.DwmGetWindowAttribute(
                hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, ctypes.byref(rect), ctypes.sizeof(rect)
            )
            if hr == 0:
                width = rect.right - rect.left
                height = rect.bottom - rect.top
                if width > 0 and height > 0:
                    return (rect.left, rect.top, rect.right, rect.bottom)
        except Exception:
            pass

        if user32.GetWindowRect(hwnd, ctypes.byref(rect)):
            width = rect.right - rect.left
            height = rect.bottom - rect.top
            if width > 0 and height > 0:
                return (rect.left, rect.top, rect.right, rect.bottom)
    except Exception:
        pass
    return None


def bring_window_to_foreground(hwnd):
    if os.name != "nt":
        return False
    try:
        import ctypes
        user32 = ctypes.windll.user32
        user32.ShowWindow(hwnd, 9)
        user32.SetForegroundWindow(hwnd)
        user32.BringWindowToTop(hwnd)
        return True
    except Exception:
        return False


def capture_window_direct(hwnd):
    if os.name != "nt":
        return None
    init_dpi_awareness()
    try:
        import ctypes
        from ctypes import wintypes
        from PIL import Image
        user32 = ctypes.windll.user32
        gdi32 = ctypes.windll.gdi32

        desktops = get_all_desktops()
        matched_desk = None
        for desk_name in desktops:
            hdesk = user32.OpenDesktopW(desk_name, 0, False, 0x01FF)
            if not hdesk:
                continue
            found = [False]
            WNDENUMPROC = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

            def w_cb(h, _):
                if h == hwnd:
                    found[0] = True
                    return False
                return True

            user32.EnumDesktopWindows(hdesk, WNDENUMPROC(w_cb), 0)
            user32.CloseDesktop(hdesk)
            if found[0]:
                matched_desk = desk_name
                break

        hdesk_switch = None
        if matched_desk:
            hdesk_switch = user32.OpenDesktopW(matched_desk, 0, False, 0x01FF)
            if hdesk_switch:
                user32.SetThreadDesktop(hdesk_switch)

        rect = wintypes.RECT()
        if not user32.GetWindowRect(hwnd, ctypes.byref(rect)):
            if hdesk_switch:
                user32.CloseDesktop(hdesk_switch)
            return None

        w = rect.right - rect.left
        h = rect.bottom - rect.top
        if w <= 0 or h <= 0:
            if hdesk_switch:
                user32.CloseDesktop(hdesk_switch)
            return None

        hdc_window = user32.GetWindowDC(hwnd)
        hdc_mem = gdi32.CreateCompatibleDC(hdc_window)
        hbm_mem = gdi32.CreateCompatibleBitmap(hdc_window, w, h)
        hold = gdi32.SelectObject(hdc_mem, hbm_mem)

        pw_res = user32.PrintWindow(hwnd, hdc_mem, 2)
        if not pw_res:
            pw_res = user32.PrintWindow(hwnd, hdc_mem, 0)

        image = None
        if pw_res:
            class BITMAPINFOHEADER(ctypes.Structure):
                _fields_ = [
                    ("biSize", wintypes.DWORD),
                    ("biWidth", wintypes.LONG),
                    ("biHeight", wintypes.LONG),
                    ("biPlanes", wintypes.WORD),
                    ("biBitCount", wintypes.WORD),
                    ("biCompression", wintypes.DWORD),
                    ("biSizeImage", wintypes.DWORD),
                    ("biXPelsPerMeter", wintypes.LONG),
                    ("biYPelsPerMeter", wintypes.LONG),
                    ("biClrUsed", wintypes.DWORD),
                    ("biClrImportant", wintypes.DWORD),
                ]

            bmi = BITMAPINFOHEADER()
            bmi.biSize = ctypes.sizeof(BITMAPINFOHEADER)
            bmi.biWidth = w
            bmi.biHeight = -h
            bmi.biPlanes = 1
            bmi.biBitCount = 32
            bmi.biCompression = 0
            buf_size = w * h * 4
            buf = (ctypes.c_ubyte * buf_size)()
            gdi32.GetDIBits(hdc_mem, hbm_mem, 0, h, ctypes.byref(buf), ctypes.byref(bmi), 0)
            image = Image.frombuffer("RGBA", (w, h), bytes(buf), "raw", "BGRA", 0, 1).convert("RGB")

        gdi32.SelectObject(hdc_mem, hold)
        gdi32.DeleteObject(hbm_mem)
        gdi32.DeleteDC(hdc_mem)
        user32.ReleaseDC(hwnd, hdc_window)
        if hdesk_switch:
            user32.CloseDesktop(hdesk_switch)
        return image
    except Exception:
        return None


def capture_screenshot(output_path, pid=None, window_only=True, focus=True):
    attach_input_desktop()
    target_bounds = None
    target_hwnd = None
    direct_image = None

    if pid and window_only and os.name == "nt":
        windows = find_windows_for_pid(pid)
        if windows:
            windows.sort(key=lambda item: len(item[1]), reverse=True)
            target_hwnd, _ = windows[0]
            if focus:
                bring_window_to_foreground(target_hwnd)
                time.sleep(0.05)
            target_bounds = get_window_bounds(target_hwnd)
            direct_image = capture_window_direct(target_hwnd)

    output = Path(output_path).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)

    try:
        if direct_image is not None:
            direct_image.save(output, format="PNG")
            return {
                "path": str(output),
                "size": direct_image.size,
                "window_captured": True,
                "bounds": target_bounds,
            }
        import PIL.ImageGrab
        image = PIL.ImageGrab.grab(bbox=target_bounds)
        image.save(output, format="PNG")
        return {
            "path": str(output),
            "size": image.size,
            "window_captured": target_hwnd is not None and target_bounds is not None,
            "bounds": target_bounds,
        }
    except Exception as exc:
        raise RuntimeError(f"Screenshot capture failed: {exc}") from exc


def read_log_tail(log_path, lines=50):
    path = Path(log_path)
    if not path.is_file():
        return ""
    try:
        content = path.read_text(encoding="utf-8", errors="replace")
    except Exception:
        return ""
    all_lines = content.splitlines()
    if lines <= 0 or len(all_lines) <= lines:
        return "\n".join(all_lines)
    return "\n".join(all_lines[-lines:])


def is_pid_running(pid):
    if pid <= 0:
        return False
    if os.name == "nt":
        try:
            import ctypes
            PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
            kernel32 = ctypes.windll.kernel32
            handle = kernel32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, pid)
            if not handle:
                return False
            exit_code = ctypes.c_uint32()
            kernel32.GetExitCodeProcess(handle, ctypes.byref(exit_code))
            kernel32.CloseHandle(handle)
            STILL_ACTIVE = 259
            return exit_code.value == STILL_ACTIVE
        except Exception:
            pass
    try:
        os.kill(pid, 0)
        return True
    except OSError:
        return False


def get_pid_exit_code(pid):
    if os.name == "nt":
        try:
            import ctypes
            PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
            kernel32 = ctypes.windll.kernel32
            handle = kernel32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, pid)
            if not handle:
                return None
            exit_code = ctypes.c_uint32()
            kernel32.GetExitCodeProcess(handle, ctypes.byref(exit_code))
            kernel32.CloseHandle(handle)
            return exit_code.value
        except Exception:
            return None
    return None


def load_state(state_path):
    path = Path(state_path)
    if not path.is_file():
        return None
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return None


def save_state(state_path, data):
    path = Path(state_path)
    path.parent.mkdir(parents=True, exist_ok=True)
    text = json.dumps(data, indent=2)
    path.write_text(text, encoding="utf-8")
    try:
        global_state_fallback().write_text(text, encoding="utf-8")
    except Exception:
        pass


def remove_state(state_path):
    Path(state_path).unlink(missing_ok=True)
    try:
        global_state_fallback().unlink(missing_ok=True)
    except Exception:
        pass


def launch_game(
    exe_path,
    workdir=None,
    args=None,
    env_vars=None,
    stdout_file=None,
    stderr_file=None,
    pad_file=None,
    state_file=None,
):
    exe = Path(exe_path).resolve()
    if not exe.is_file():
        raise FileNotFoundError(f"Executable not found: {exe}")

    working_directory = Path(workdir).resolve() if workdir else exe.parent
    working_directory.mkdir(parents=True, exist_ok=True)

    state_path = Path(state_file).resolve() if state_file else default_state_path(working_directory)
    existing_state = load_state(state_path)
    if existing_state and is_pid_running(existing_state.get("pid", 0)):
        raise RuntimeError(
            f"A game process is already running with PID {existing_state['pid']}. Stop it first."
        )

    stdout_path = Path(stdout_file).resolve() if stdout_file else working_directory / "game_stdout.log"
    stderr_path = Path(stderr_file).resolve() if stderr_file else working_directory / "game_stderr.log"

    env = os.environ.copy()
    if pad_file:
        pad_path = Path(pad_file).resolve()
        pad_path.parent.mkdir(parents=True, exist_ok=True)
        env["APS5_PAD_INPUT_FILE"] = str(pad_path)
    else:
        pad_path = working_directory / "pad_input.cmd"
        env["APS5_PAD_INPUT_FILE"] = str(pad_path)

    if env_vars:
        for item in env_vars:
            key, sep, val = item.partition("=")
            if sep:
                env[key.strip()] = val.strip()

    cmd = [str(exe)] + (args or [])

    creation_flags = 0
    if os.name == "nt":
        creation_flags = subprocess.CREATE_NEW_PROCESS_GROUP

    with open(stdout_path, "w", encoding="utf-8", errors="replace") as stdout_handle, \
         open(stderr_path, "w", encoding="utf-8", errors="replace") as stderr_handle:
        process = subprocess.Popen(
            cmd,
            cwd=str(working_directory),
            env=env,
            stdout=stdout_handle,
            stderr=stderr_handle,
            creationflags=creation_flags,
        )

    state_data = {
        "pid": process.pid,
        "exe": str(exe),
        "workdir": str(working_directory),
        "cmd": cmd,
        "stdout_log": str(stdout_path),
        "stderr_log": str(stderr_path),
        "pad_input_file": str(pad_path),
        "start_time": time.time(),
    }
    save_state(state_path, state_data)

    return state_data


def get_game_status(state_file=None, workdir=None):
    state_path = Path(state_file).resolve() if state_file else default_state_path(workdir)
    state = load_state(state_path)
    if not state:
        return {"running": False, "message": "No active game session recorded"}

    pid = state.get("pid", 0)
    running = is_pid_running(pid)
    elapsed = round(time.time() - state.get("start_time", time.time()), 2)

    status_info = {
        "running": running,
        "pid": pid,
        "elapsed_seconds": elapsed,
        "exe": state.get("exe"),
        "workdir": state.get("workdir"),
        "stdout_log": state.get("stdout_log"),
        "stderr_log": state.get("stderr_log"),
        "pad_input_file": state.get("pad_input_file"),
    }

    if running:
        windows = find_windows_for_pid(pid) if os.name == "nt" else []
        status_info["windows"] = [
            {"hwnd": hwnd, "title": title, "bounds": get_window_bounds(hwnd)}
            for hwnd, title in windows
        ]
    else:
        status_info["exit_code"] = get_pid_exit_code(pid)
        status_info["stderr_tail"] = read_log_tail(state.get("stderr_log"), lines=10)

    return status_info


def kill_game(state_file=None, workdir=None, force=False, timeout=3.0):
    state_path = Path(state_file).resolve() if state_file else default_state_path(workdir)
    state = load_state(state_path)
    if not state:
        return {"stopped": False, "message": "No active game process recorded"}

    pid = state.get("pid", 0)
    if not is_pid_running(pid):
        remove_state(state_path)
        return {"stopped": True, "pid": pid, "message": "Process already terminated"}

    if not force and os.name == "nt":
        windows = find_windows_for_pid(pid)
        if windows:
            import ctypes
            user32 = ctypes.windll.user32
            WM_CLOSE = 0x0010
            for hwnd, _ in windows:
                user32.PostMessageW(hwnd, WM_CLOSE, 0, 0)

            deadline = time.time() + timeout
            while time.time() < deadline:
                if not is_pid_running(pid):
                    remove_state(state_path)
                    return {"stopped": True, "pid": pid, "method": "graceful_wm_close"}
                time.sleep(0.1)

    if os.name == "nt":
        subprocess.run(["taskkill", "/F", "/T", "/PID", str(pid)], capture_output=True, check=False)
    else:
        with contextlib.suppress(OSError):
            os.kill(pid, 9)

    remove_state(state_path)
    return {"stopped": True, "pid": pid, "method": "force_terminated"}


def send_pad_input(button, hold_ms=300, sticks=None, pad_file=None, state_file=None, workdir=None):
    if button not in DEFAULT_PAD_BUTTONS:
        raise ValueError(f"Unknown button: {button}. Choices: {list(DEFAULT_PAD_BUTTONS.keys())}")

    if pad_file:
        pad_path = Path(pad_file)
    else:
        state_path = Path(state_file).resolve() if state_file else default_state_path(workdir)
        state = load_state(state_path)
        if state and state.get("pad_input_file"):
            pad_path = Path(state["pad_input_file"])
        else:
            raise ValueError("No pad_input_file specified and no active session found")

    sequence = time.monotonic_ns()
    pad_path.parent.mkdir(parents=True, exist_ok=True)

    stick_payload = ""
    if sticks:
        stick_payload = " " + " ".join(str(axis) for axis in sticks)

    content = f"{sequence} {DEFAULT_PAD_BUTTONS[button]} {hold_ms}{stick_payload}\n"
    temp_path = pad_path.parent / f"{pad_path.name}.tmp"
    temp_path.write_text(content, encoding="utf-8")
    os.replace(temp_path, pad_path)

    return {
        "status": "queued",
        "button": button,
        "hold_ms": hold_ms,
        "sequence": sequence,
        "file": str(pad_path),
    }


MCP_TOOLS = [
    {
        "name": "game_runner_launch",
        "description": "Launches a game executable in the background, redirects output to logs, and sets up pad input.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "exe_path": {
                    "type": "string",
                    "description": "Path to executable (e.g. C:/AnyPS5/run/app.exe).",
                },
                "workdir": {
                    "type": "string",
                    "description": "Working directory for execution (defaults to executable directory).",
                },
                "args": {
                    "type": "array",
                    "items": {"type": "string"},
                    "description": "Command-line arguments to pass to the executable.",
                },
                "env_vars": {
                    "type": "array",
                    "items": {"type": "string"},
                    "description": "Environment variables in KEY=VALUE format.",
                },
            },
            "required": ["exe_path"],
        },
    },
    {
        "name": "game_runner_status",
        "description": "Gets current status of the game (running, exit code, elapsed time, window info).",
        "inputSchema": {
            "type": "object",
            "properties": {
                "workdir": {
                    "type": "string",
                    "description": "Optional working directory to locate the runner state file.",
                }
            },
        },
    },
    {
        "name": "game_runner_screenshot",
        "description": "Captures a screenshot of the running game window or full desktop and returns the file path.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "output_path": {
                    "type": "string",
                    "description": "Path where the PNG screenshot should be saved.",
                },
                "window_only": {
                    "type": "boolean",
                    "description": "Capture only the game window (true, default) or entire screen (false).",
                    "default": True,
                },
            },
            "required": ["output_path"],
        },
    },
    {
        "name": "game_runner_logs",
        "description": "Retrieves the recent stdout or stderr output from the running or finished game.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "stream": {
                    "type": "string",
                    "enum": ["stderr", "stdout", "both"],
                    "description": "Which log stream to read (default: stderr).",
                    "default": "stderr",
                },
                "lines": {
                    "type": "integer",
                    "description": "Number of trailing lines to read (default: 50).",
                    "default": 50,
                },
                "workdir": {
                    "type": "string",
                    "description": "Optional working directory to locate the runner state.",
                },
            },
        },
    },
    {
        "name": "game_runner_input",
        "description": "Sends a controller button pulse or stick movement via APS5_PAD_INPUT_FILE.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "button": {
                    "type": "string",
                    "description": "Controller button name (cross, circle, triangle, square, options, up, down, left, right, l1, r1, l2, r2, release).",
                },
                "hold_ms": {
                    "type": "integer",
                    "description": "Duration to hold the button in milliseconds (default: 300).",
                    "default": 300,
                },
            },
            "required": ["button"],
        },
    },
    {
        "name": "game_runner_kill",
        "description": "Terminates the running game process (gracefully with WM_CLOSE first, or forced).",
        "inputSchema": {
            "type": "object",
            "properties": {
                "force": {
                    "type": "boolean",
                    "description": "Force kill immediately without attempting graceful close.",
                    "default": False,
                },
                "timeout": {
                    "type": "number",
                    "description": "Seconds to wait before force kill (default: 3.0).",
                    "default": 3.0,
                },
            },
        },
    },
]


def handle_mcp_call(tool_name, arguments):
    if tool_name == "game_runner_launch":
        result = launch_game(
            arguments["exe_path"],
            workdir=arguments.get("workdir"),
            args=arguments.get("args"),
            env_vars=arguments.get("env_vars"),
        )
        return {"content": [{"type": "text", "text": json.dumps(result, indent=2)}]}

    if tool_name == "game_runner_status":
        result = get_game_status(workdir=arguments.get("workdir"))
        return {"content": [{"type": "text", "text": json.dumps(result, indent=2)}]}

    if tool_name == "game_runner_screenshot":
        output_path = arguments["output_path"]
        window_only = arguments.get("window_only", True)
        state = load_state(default_state_path(arguments.get("workdir")))
        pid = state.get("pid") if state else None
        res = capture_screenshot(output_path, pid=pid, window_only=window_only)
        content = [{"type": "text", "text": json.dumps(res, indent=2)}]
        if Path(output_path).is_file():
            try:
                raw_bytes = Path(output_path).read_bytes()
                b64 = base64.b64encode(raw_bytes).decode("ascii")
                content.append({"type": "image", "data": b64, "mimeType": "image/png"})
            except Exception:
                pass
        return {"content": content}

    if tool_name == "game_runner_logs":
        stream = arguments.get("stream", "stderr")
        lines = arguments.get("lines", 50)
        state = load_state(default_state_path(arguments.get("workdir")))
        if not state:
            return {"content": [{"type": "text", "text": "No game session found."}], "isError": True}

        output = []
        if stream in ("stderr", "both"):
            tail = read_log_tail(state.get("stderr_log"), lines=lines)
            output.append(f"--- STDERR (last {lines} lines) ---\n{tail}")
        if stream in ("stdout", "both"):
            tail = read_log_tail(state.get("stdout_log"), lines=lines)
            output.append(f"--- STDOUT (last {lines} lines) ---\n{tail}")

        return {"content": [{"type": "text", "text": "\n\n".join(output)}]}

    if tool_name == "game_runner_input":
        result = send_pad_input(
            arguments["button"],
            hold_ms=arguments.get("hold_ms", 300),
            pad_file=arguments.get("pad_file"),
            workdir=arguments.get("workdir"),
        )
        return {"content": [{"type": "text", "text": json.dumps(result, indent=2)}]}

    if tool_name == "game_runner_kill":
        result = kill_game(
            workdir=arguments.get("workdir"),
            force=arguments.get("force", False),
            timeout=arguments.get("timeout", 3.0),
        )
        return {"content": [{"type": "text", "text": json.dumps(result, indent=2)}]}

    raise ValueError(f"Unknown MCP tool: {tool_name}")


def run_mcp_server():
    if sys.stdin.isatty():
        sys.stderr.write(
            "AnyPS5 MCP server started in stdio mode.\n"
            "Waiting for JSON-RPC messages from MCP client via standard input...\n"
        )
        sys.stderr.flush()
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            req = json.loads(line)
        except Exception:
            continue

        req_id = req.get("id")
        method = req.get("method")

        if method == "initialize":
            resp = {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "protocolVersion": "2024-11-05",
                    "capabilities": {"tools": {}},
                    "serverInfo": {"name": "anyps5-game-runner", "version": "1.0.0"},
                },
            }
        elif method == "notifications/initialized":
            continue
        elif method == "ping":
            resp = {"jsonrpc": "2.0", "id": req_id, "result": {}}
        elif method == "tools/list":
            resp = {"jsonrpc": "2.0", "id": req_id, "result": {"tools": MCP_TOOLS}}
        elif method == "tools/call":
            params = req.get("params", {})
            tool_name = params.get("name")
            tool_args = params.get("arguments", {})
            try:
                call_res = handle_mcp_call(tool_name, tool_args)
                resp = {"jsonrpc": "2.0", "id": req_id, "result": call_res}
            except Exception as exc:
                resp = {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "result": {
                        "content": [{"type": "text", "text": f"Error: {exc}"}],
                        "isError": True,
                    },
                }
        else:
            resp = {
                "jsonrpc": "2.0",
                "id": req_id,
                "error": {"code": -32601, "message": f"Method {method} not found"},
            }

        sys.stdout.write(json.dumps(resp) + "\n")
        sys.stdout.flush()


def main():
    parser = argparse.ArgumentParser(
        description="AnyPS5 game runner, debugger and screenshot utility with MCP server mode."
    )
    subparsers = parser.add_subparsers(dest="command")

    launch_parser = subparsers.add_parser("launch", help="Launch game executable in background")
    launch_parser.add_argument("--exe", required=True, help="Path to executable")
    launch_parser.add_argument("--workdir", help="Working directory")
    launch_parser.add_argument("--arg", action="append", dest="args", help="Arguments to pass")
    launch_parser.add_argument("--env", action="append", dest="env_vars", help="KEY=VALUE environment variable")
    launch_parser.add_argument("--pad-file", help="Override pad input file path")
    launch_parser.add_argument("--state-file", help="State file path")

    status_parser = subparsers.add_parser("status", help="Check status of running game")
    status_parser.add_argument("--workdir", help="Working directory to find state file")
    status_parser.add_argument("--state-file", help="State file path")

    shot_parser = subparsers.add_parser("screenshot", help="Capture game window or screen")
    shot_parser.add_argument("--output", "-o", default="screenshot.png", help="Output PNG file path")
    shot_parser.add_argument("--full-screen", action="store_true", help="Capture full screen instead of window")
    shot_parser.add_argument("--no-focus", action="store_true", help="Do not restore/focus window")
    shot_parser.add_argument("--state-file", help="State file path")
    shot_parser.add_argument("--workdir", help="Working directory")

    log_parser = subparsers.add_parser("logs", help="View recent stdout/stderr output")
    log_parser.add_argument("--stream", choices=["stderr", "stdout", "both"], default="stderr")
    log_parser.add_argument("--lines", "-n", type=int, default=50)
    log_parser.add_argument("--workdir", help="Working directory")
    log_parser.add_argument("--state-file", help="State file path")

    in_parser = subparsers.add_parser("input", help="Send controller pulse")
    in_parser.add_argument("button", choices=list(DEFAULT_PAD_BUTTONS.keys()))
    in_parser.add_argument("--hold-ms", type=int, default=300)
    in_parser.add_argument("--left-stick", type=int, nargs=2, metavar=("X", "Y"))
    in_parser.add_argument("--right-stick", type=int, nargs=2, metavar=("X", "Y"))
    in_parser.add_argument("--pad-file", help="Pad input file path")
    in_parser.add_argument("--workdir", help="Working directory")
    in_parser.add_argument("--state-file", help="State file path")

    kill_parser = subparsers.add_parser("kill", help="Stop running game")
    kill_parser.add_argument("--force", "-f", action="store_true", help="Force terminate immediately")
    kill_parser.add_argument("--timeout", type=float, default=3.0, help="Graceful shutdown timeout")
    kill_parser.add_argument("--workdir", help="Working directory")
    kill_parser.add_argument("--state-file", help="State file path")

    subparsers.add_parser("mcp", help="Run as MCP (Model Context Protocol) stdio server")

    args = parser.parse_args()

    if args.command == "launch":
        res = launch_game(
            args.exe,
            workdir=args.workdir,
            args=args.args,
            env_vars=args.env_vars,
            pad_file=args.pad_file,
            state_file=args.state_file,
        )
        print(json.dumps(res, indent=2))

    elif args.command == "status":
        res = get_game_status(state_file=args.state_file, workdir=args.workdir)
        print(json.dumps(res, indent=2))

    elif args.command == "screenshot":
        state = load_state(args.state_file or default_state_path(args.workdir))
        pid = state.get("pid") if state else None
        res = capture_screenshot(
            args.output,
            pid=pid,
            window_only=not args.full_screen,
            focus=not args.no_focus,
        )
        print(json.dumps(res, indent=2))

    elif args.command == "logs":
        state = load_state(args.state_file or default_state_path(args.workdir))
        if not state:
            sys.exit("No game state found.")
        if args.stream in ("stderr", "both"):
            print(f"=== STDERR ({state.get('stderr_log')}) ===")
            print(read_log_tail(state.get("stderr_log"), lines=args.lines))
        if args.stream in ("stdout", "both"):
            print(f"=== STDOUT ({state.get('stdout_log')}) ===")
            print(read_log_tail(state.get("stdout_log"), lines=args.lines))

    elif args.command == "input":
        sticks = None
        if args.left_stick or args.right_stick:
            sticks = (args.left_stick or [128, 128]) + (args.right_stick or [128, 128])
        res = send_pad_input(
            args.button,
            hold_ms=args.hold_ms,
            sticks=sticks,
            pad_file=args.pad_file,
            state_file=args.state_file,
            workdir=args.workdir,
        )
        print(json.dumps(res, indent=2))

    elif args.command == "kill":
        res = kill_game(
            state_file=args.state_file,
            workdir=args.workdir,
            force=args.force,
            timeout=args.timeout,
        )
        print(json.dumps(res, indent=2))

    elif args.command == "mcp":
        run_mcp_server()

    else:
        parser.print_help()


if __name__ == "__main__":
    main()
