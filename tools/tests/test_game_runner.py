import io
import json
from pathlib import Path
import sys
import tempfile
import time
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import game_runner


class GameRunnerTests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp_dir.cleanup)
        self.workdir = Path(self.temp_dir.name)
        self.state_file = self.workdir / ".game_runner_state.json"

    def test_state_persistence(self):
        data = {"pid": 1234, "exe": "dummy.exe"}
        game_runner.save_state(self.state_file, data)
        loaded = game_runner.load_state(self.state_file)
        self.assertEqual(loaded, data)

        game_runner.remove_state(self.state_file)
        self.assertIsNone(game_runner.load_state(self.state_file))

    def test_read_log_tail(self):
        log_file = self.workdir / "log.txt"
        lines = [f"line {i}" for i in range(100)]
        log_file.write_text("\n".join(lines), encoding="utf-8")

        tail = game_runner.read_log_tail(log_file, lines=5)
        self.assertEqual(tail, "line 95\nline 96\nline 97\nline 98\nline 99")
        self.assertEqual(game_runner.read_log_tail(self.workdir / "absent.txt"), "")

    def test_launch_game_requires_existing_exe(self):
        with self.assertRaises(FileNotFoundError):
            game_runner.launch_game(
                self.workdir / "nonexistent.exe",
                workdir=self.workdir,
                state_file=self.state_file,
            )

    def test_launch_game_prevents_duplicate_active_process(self):
        fake_exe = self.workdir / "app.exe"
        fake_exe.write_text("dummy", encoding="utf-8")
        game_runner.save_state(self.state_file, {"pid": 99999})

        with mock.patch.object(game_runner, "is_pid_running", return_value=True):
            with self.assertRaises(RuntimeError):
                game_runner.launch_game(
                    fake_exe,
                    workdir=self.workdir,
                    state_file=self.state_file,
                )

    def test_launch_game_success(self):
        fake_exe = self.workdir / "app.exe"
        fake_exe.write_text("dummy", encoding="utf-8")

        mock_proc = mock.Mock(pid=54321)
        with mock.patch.object(game_runner.subprocess, "Popen", return_value=mock_proc):
            res = game_runner.launch_game(
                fake_exe,
                workdir=self.workdir,
                args=["--headless"],
                env_vars=["APS5_TEST=1"],
                state_file=self.state_file,
            )

        self.assertEqual(res["pid"], 54321)
        self.assertEqual(res["cmd"], [str(fake_exe.resolve()), "--headless"])
        self.assertTrue(self.state_file.is_file())

        loaded = game_runner.load_state(self.state_file)
        self.assertEqual(loaded["pid"], 54321)

    def test_get_game_status_no_session(self):
        status = game_runner.get_game_status(state_file=self.state_file)
        self.assertFalse(status["running"])
        self.assertIn("No active game session", status["message"])

    def test_get_game_status_running_and_terminated(self):
        game_runner.save_state(
            self.state_file,
            {
                "pid": 1111,
                "exe": "app.exe",
                "workdir": str(self.workdir),
                "start_time": time.time() - 10,
                "stderr_log": str(self.workdir / "stderr.log"),
            },
        )

        with mock.patch.object(game_runner, "is_pid_running", return_value=True):
            with mock.patch.object(game_runner, "find_windows_for_pid", return_value=[(101, "Game Window")]):
                status = game_runner.get_game_status(state_file=self.state_file)
                self.assertTrue(status["running"])
                self.assertEqual(status["pid"], 1111)
                self.assertGreaterEqual(status["elapsed_seconds"], 9.0)

        with mock.patch.object(game_runner, "is_pid_running", return_value=False):
            with mock.patch.object(game_runner, "get_pid_exit_code", return_value=0):
                status = game_runner.get_game_status(state_file=self.state_file)
                self.assertFalse(status["running"])
                self.assertEqual(status["exit_code"], 0)

    def test_send_pad_input_formats_file_correctly(self):
        pad_file = self.workdir / "pad_input.cmd"
        res = game_runner.send_pad_input("cross", hold_ms=500, pad_file=pad_file)
        self.assertEqual(res["button"], "cross")
        self.assertEqual(res["hold_ms"], 500)
        self.assertTrue(pad_file.is_file())

        tokens = pad_file.read_text(encoding="utf-8").strip().split()
        self.assertEqual(len(tokens), 3)
        self.assertEqual(tokens[1], "16384")
        self.assertEqual(tokens[2], "500")

    def test_send_pad_input_rejects_unknown_button(self):
        with self.assertRaises(ValueError):
            game_runner.send_pad_input("super_button", pad_file=self.workdir / "pad.cmd")

    def test_kill_game_already_stopped(self):
        game_runner.save_state(self.state_file, {"pid": 2222})
        with mock.patch.object(game_runner, "is_pid_running", return_value=False):
            res = game_runner.kill_game(state_file=self.state_file)
            self.assertTrue(res["stopped"])
            self.assertFalse(self.state_file.exists())

    def test_mcp_tools_list_and_ping(self):
        tools = game_runner.MCP_TOOLS
        tool_names = {tool["name"] for tool in tools}
        expected = {
            "game_runner_launch",
            "game_runner_status",
            "game_runner_screenshot",
            "game_runner_logs",
            "game_runner_input",
            "game_runner_kill",
        }
        self.assertTrue(expected.issubset(tool_names))

    def test_handle_mcp_call_status(self):
        with mock.patch.object(game_runner, "get_game_status", return_value={"running": False}):
            res = game_runner.handle_mcp_call("game_runner_status", {"workdir": str(self.workdir)})
            self.assertIn("content", res)
            self.assertEqual(res["content"][0]["type"], "text")
            parsed = json.loads(res["content"][0]["text"])
            self.assertFalse(parsed["running"])

    def test_handle_mcp_call_unknown(self):
        with self.assertRaises(ValueError):
            game_runner.handle_mcp_call("nonexistent_tool", {})


class McpProtocolTests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp_dir.cleanup)
        self.workdir = Path(self.temp_dir.name)

    def exchange(self, requests):
        input_data = []
        for req in requests:
            if isinstance(req, str):
                input_data.append(req)
            else:
                input_data.append(json.dumps(req))
        input_stream = io.StringIO("\n".join(input_data) + "\n")
        output_stream = io.StringIO()
        with mock.patch.object(sys, "stdin", input_stream), mock.patch.object(sys, "stdout", output_stream):
            game_runner.run_mcp_server()
        return [json.loads(line) for line in output_stream.getvalue().splitlines() if line.strip()]

    def test_initialize_and_ping(self):
        responses = self.exchange([
            {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {}},
            {"jsonrpc": "2.0", "method": "notifications/initialized"},
            {"jsonrpc": "2.0", "id": 2, "method": "ping"},
        ])
        self.assertEqual(len(responses), 2)
        self.assertEqual(responses[0]["id"], 1)
        self.assertEqual(responses[0]["result"]["serverInfo"]["name"], "anyps5-game-runner")
        self.assertEqual(responses[0]["result"]["protocolVersion"], "2024-11-05")
        self.assertEqual(responses[1]["id"], 2)
        self.assertEqual(responses[1]["result"], {})

    def test_tools_list_schema(self):
        responses = self.exchange([
            {"jsonrpc": "2.0", "id": 10, "method": "tools/list", "params": {}}
        ])
        self.assertEqual(len(responses), 1)
        tools = responses[0]["result"]["tools"]
        names = {t["name"] for t in tools}
        for expected in ("game_runner_launch", "game_runner_status", "game_runner_screenshot",
                         "game_runner_logs", "game_runner_input", "game_runner_kill"):
            self.assertIn(expected, names)

    def test_unknown_method_returns_rpc_error(self):
        responses = self.exchange([
            {"jsonrpc": "2.0", "id": 99, "method": "unknown/method", "params": {}}
        ])
        self.assertEqual(len(responses), 1)
        self.assertEqual(responses[0]["id"], 99)
        self.assertEqual(responses[0]["error"]["code"], -32601)

    def test_malformed_json_lines_are_ignored(self):
        responses = self.exchange([
            "not json at all",
            "",
            {"jsonrpc": "2.0", "id": 1, "method": "ping"},
        ])
        self.assertEqual(len(responses), 1)
        self.assertEqual(responses[0]["id"], 1)

    def test_tool_call_status(self):
        state_file = self.workdir / ".game_runner_state.json"
        game_runner.save_state(state_file, {"pid": 4321, "exe": "test.exe"})
        with mock.patch.object(game_runner, "is_pid_running", return_value=True):
            responses = self.exchange([
                {
                    "jsonrpc": "2.0",
                    "id": 5,
                    "method": "tools/call",
                    "params": {
                        "name": "game_runner_status",
                        "arguments": {"workdir": str(self.workdir)},
                    },
                }
            ])
        self.assertEqual(len(responses), 1)
        content = responses[0]["result"]["content"]
        self.assertEqual(content[0]["type"], "text")
        status = json.loads(content[0]["text"])
        self.assertTrue(status["running"])
        self.assertEqual(status["pid"], 4321)

    def test_tool_call_logs(self):
        state_file = self.workdir / ".game_runner_state.json"
        err_file = self.workdir / "err.log"
        err_file.write_text("critical error message\n", encoding="utf-8")
        game_runner.save_state(state_file, {"pid": 4321, "stderr_log": str(err_file)})

        responses = self.exchange([
            {
                "jsonrpc": "2.0",
                "id": 6,
                "method": "tools/call",
                "params": {
                    "name": "game_runner_logs",
                    "arguments": {
                        "stream": "stderr",
                        "lines": 10,
                        "workdir": str(self.workdir),
                    },
                },
            }
        ])
        self.assertEqual(len(responses), 1)
        text = responses[0]["result"]["content"][0]["text"]
        self.assertIn("critical error message", text)

    def test_tool_call_launch_and_kill(self):
        mock_launch = {"pid": 7777, "cmd": ["dummy.exe"]}
        mock_kill = {"stopped": True, "pid": 7777}
        with mock.patch.object(game_runner, "launch_game", return_value=mock_launch), \
                mock.patch.object(game_runner, "kill_game", return_value=mock_kill):
            responses = self.exchange([
                {
                    "jsonrpc": "2.0",
                    "id": 7,
                    "method": "tools/call",
                    "params": {
                        "name": "game_runner_launch",
                        "arguments": {"exe_path": "dummy.exe"},
                    },
                },
                {
                    "jsonrpc": "2.0",
                    "id": 8,
                    "method": "tools/call",
                    "params": {
                        "name": "game_runner_kill",
                        "arguments": {"force": True},
                    },
                },
            ])
        self.assertEqual(len(responses), 2)
        launch_res = json.loads(responses[0]["result"]["content"][0]["text"])
        self.assertEqual(launch_res["pid"], 7777)
        kill_res = json.loads(responses[1]["result"]["content"][0]["text"])
        self.assertTrue(kill_res["stopped"])

    def test_tool_call_screenshot(self):
        shot_path = self.workdir / "screen.png"
        shot_path.write_bytes(b"\x89PNG\r\n\x1a\nfakeimagebytes")
        mock_res = {"path": str(shot_path), "size": [100, 100], "bounds": None}

        with mock.patch.object(game_runner, "capture_screenshot", return_value=mock_res):
            responses = self.exchange([
                {
                    "jsonrpc": "2.0",
                    "id": 9,
                    "method": "tools/call",
                    "params": {
                        "name": "game_runner_screenshot",
                        "arguments": {"output_path": str(shot_path)},
                    },
                }
            ])
        self.assertEqual(len(responses), 1)
        content = responses[0]["result"]["content"]
        self.assertEqual(content[0]["type"], "text")
        self.assertEqual(content[1]["type"], "image")
        self.assertEqual(content[1]["mimeType"], "image/png")

    def test_tool_call_input(self):
        pad_file = self.workdir / "pad.cmd"
        responses = self.exchange([
            {
                "jsonrpc": "2.0",
                "id": 11,
                "method": "tools/call",
                "params": {
                    "name": "game_runner_input",
                    "arguments": {
                        "button": "options",
                        "hold_ms": 200,
                        "pad_file": str(pad_file),
                    },
                },
            }
        ])
        self.assertEqual(len(responses), 1)
        res = json.loads(responses[0]["result"]["content"][0]["text"])
        self.assertEqual(res["button"], "options")
        self.assertEqual(res["hold_ms"], 200)

    def test_tool_call_failure_returns_is_error(self):
        responses = self.exchange([
            {
                "jsonrpc": "2.0",
                "id": 12,
                "method": "tools/call",
                "params": {
                    "name": "nonexistent_tool_name",
                    "arguments": {},
                },
            }
        ])
        self.assertEqual(len(responses), 1)
        self.assertTrue(responses[0]["result"]["isError"])
        self.assertIn("Error:", responses[0]["result"]["content"][0]["text"])


if __name__ == "__main__":
    unittest.main()
