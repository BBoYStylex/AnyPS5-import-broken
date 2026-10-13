# AnyPS5 Game Runner and MCP Server

`tools/game_runner.py` provides an automated process runner, debugger, and screenshot capture tool designed for both interactive developer use and autonomous AI agent workflows via the [Model Context Protocol (MCP)](https://modelcontextprotocol.io/).

## Features

- **Background Process Lifecycle**: Launch games without blocking, track execution PID, elapsed time, exit codes, and resource status.
- **Log Management**: Redirects stdout/stderr to separate log files and provides tail inspection with crash diagnostic support.
- **Window-Aware Screenshots**: Automatically locates the running game window on Windows, brings it to focus, and captures pixel-perfect PNG frames (with full desktop fallback).
- **Controller Automation**: Seamlessly queues gamepad button and analog stick pulses through `APS5_PAD_INPUT_FILE` (matching the controller protocol defined in `tools/pad_input.py`).
- **Model Context Protocol (MCP)**: Native stdio JSON-RPC 2.0 server mode exposing game runner tools directly to AI coding agents (Antigravity, Claude, Cursor, Windsurf).

---

## Command-Line Usage

### 1. Launch a Game

```sh
python tools/game_runner.py launch --exe C:/AnyPS5/run/app.exe --workdir C:/AnyPS5/run
```

Options:
- `--exe <path>`: Path to the converted game executable.
- `--workdir <dir>`: Working directory for execution (defaults to the executable directory).
- `--arg <val>`: Additional command-line argument to pass to the game (repeatable).
- `--env <KEY=VAL>`: Environment variable to set (repeatable).
- `--pad-file <path>`: Custom location for the controller input pulse file.

### 2. Inspect Status

```sh
python tools/game_runner.py status
```

Returns a JSON object with:
- `running`: boolean indicating whether the process is alive.
- `pid`: Process identifier.
- `elapsed_seconds`: Execution time so far.
- `windows`: Enumerated HWNDs, window titles, and screen bounds.
- `exit_code` / `stderr_tail`: Exit diagnostics if the process has stopped or crashed.

### 3. Capture Screenshots

```sh
python tools/game_runner.py screenshot --output screenshot.png
```

Options:
- `--output <path>`: Destination path for the PNG image (default: `screenshot.png`).
- `--full-screen`: Captures the whole desktop instead of cropping to the game window.
- `--no-focus`: Do not bring the game window to the foreground before taking the screenshot.

### 4. Read Game Logs

```sh
python tools/game_runner.py logs --lines 50 --stream stderr
```

Options:
- `--lines <N>`: Number of trailing lines to display (default: 50).
- `--stream {stderr,stdout,both}`: Log stream to inspect (default: `stderr`).

### 5. Send Gamepad Input

```sh
python tools/game_runner.py input cross --hold-ms 300
```

Supported buttons:
- `cross`, `circle`, `triangle`, `square`
- `up`, `down`, `left`, `right`
- `l1`, `r1`, `l2`, `r2`, `l3`, `r3`
- `options`, `touchpad`, `release`

Analog sticks:
```sh
python tools/game_runner.py input cross --left-stick 255 128
```

### 6. Stop or Kill the Process

```sh
python tools/game_runner.py kill
```

Sends a graceful `WM_CLOSE` to the game window. If the process does not exit within the timeout, it forces termination.
Use `--force` to terminate immediately.

---

## Model Context Protocol (MCP) Integration

To enable AI agents to interact with AnyPS5 automatically, run `game_runner.py` in MCP mode or register it in your agent configuration.

### MCP Configuration Example

Add the server to your agent's MCP configuration (e.g. `~/.gemini/config/mcp_config.json` or equivalent agent config):

```json
{
  "mcpServers": {
    "anyps5-runner": {
      "command": "python",
      "args": ["C:/AnyPS5/AnyPS5/tools/game_runner.py", "mcp"],
      "env": {
        "PYTHONUNBUFFERED": "1"
      }
    }
  }
}
```

### Exposed MCP Tools

1. `game_runner_launch`: Starts the game executable with custom arguments and environment variables.
2. `game_runner_status`: Checks process health, running state, and window handles.
3. `game_runner_screenshot`: Captures a screenshot and returns both the file path and base64-encoded image for multimodal agent inspection.
4. `game_runner_logs`: Reads the latest stdout/stderr messages.
5. `game_runner_input`: Sends gamepad button pulses.
6. `game_runner_kill`: Stops or force-kills the game session.
