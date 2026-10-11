import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import shader_corpus


FIXTURE = """
import os
from pathlib import Path
import struct
import sys
import time
mode = sys.argv[1]
request = Path(sys.argv[-1])
text = request.read_text(encoding="utf-8")
if mode == "logs":
    sys.stdout.write("x" * 200000 + "TAIL")
    sys.stderr.write("y" * 200000 + "ERRTAIL")
if mode == "timeout":
    print("before timeout", flush=True)
    time.sleep(10)
if mode == "crash":
    os._exit(7)
if text == "fail" or mode == "regress" and text == "good":
    print("resource analysis failed: fixture", flush=True)
    sys.exit(1)
if mode == "missing":
    sys.exit(0)
if mode == "invalid":
    Path("request.req.spv").write_bytes(b"bad")
    sys.exit(0)
generator = 2 if mode == "change" else os.getpid() if mode == "unstable" else 1
Path("request.req.spv").write_bytes(struct.pack("<5I", 0x07230203, 0x10000, generator, 1, 0))
"""


class CorpusTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.fixture = self.root / "fixture.py"
        self.fixture.write_text(FIXTURE, encoding="utf-8")
        self.request = self.root / "shader.req"
        self.request.write_text("good", encoding="utf-8")
        self.cases = shader_corpus.collect_cases([self.request])

    def build(self, mode="normal"):
        return shader_corpus.resolve_command(sys.executable, [str(self.fixture), mode])

    def run_report(self, mode="normal", baseline=None, **options):
        return shader_corpus.run_corpus(self.cases, self.build(mode), self.build(baseline) if baseline else None,
                                       repeats=options.pop("repeats", 1), timeout=options.pop("timeout", 3), **options)

    def test_discovery_deduplicates_content_and_aliases(self):
        directory = self.root / "captures"
        directory.mkdir()
        other = directory / "copy.req"
        other.write_bytes(self.request.read_bytes())
        cases = shader_corpus.collect_cases([self.request, self.request, directory])
        self.assertEqual(len(cases), 1)
        self.assertEqual(len(cases[0]["paths"]), 2)
        report = shader_corpus.run_corpus(cases, self.build(), repeats=1)
        self.assertEqual(report["summary"]["captures"], 2)

    def test_empty_or_wrong_input(self):
        directory = self.root / "empty"
        directory.mkdir()
        for paths in [[], [directory], [self.fixture]]:
            with self.subTest(paths=paths):
                with self.assertRaises(ValueError):
                    shader_corpus.collect_cases(paths)

    def test_native_process_roundtrip_and_source_preservation(self):
        report = self.run_report(repeats=2)
        result = report["results"][0]["candidate"]
        self.assertEqual(result["status"], "passed")
        self.assertTrue(result["stable_output"])
        self.assertEqual(result["samples"][0]["artifact"]["words"], 5)
        self.assertEqual(shader_corpus.exit_code(report), 0)
        self.assertEqual(self.request.read_text(encoding="utf-8"), "good")
        self.assertFalse((self.root / "request.req.spv").exists())

    def test_regression(self):
        report = self.run_report("regress", "normal")
        self.assertEqual(report["summary"]["regressions"], 1)
        self.assertEqual(shader_corpus.exit_code(report), 1)

    def test_resolution_and_unchanged_failure(self):
        resolved = self.run_report("normal", "regress")
        self.assertEqual(resolved["summary"]["resolved"], 1)
        self.assertEqual(shader_corpus.exit_code(resolved), 0)
        failed = self.run_report("regress", "regress")
        self.assertEqual(failed["results"][0]["comparison"]["classification"], "unchanged_failure")
        self.assertEqual(shader_corpus.exit_code(failed), 0)

    def test_output_changes_are_opt_in_gate(self):
        report = self.run_report("change", "normal", minimum_ms=10000)
        self.assertEqual(report["summary"]["output_changes"], 1)
        self.assertEqual(shader_corpus.exit_code(report), 0)
        self.assertEqual(shader_corpus.exit_code(report, True), 1)

    def test_unstable_output(self):
        report = self.run_report("unstable", repeats=2)
        self.assertEqual(report["summary"]["unstable_outputs"], 1)
        self.assertEqual(shader_corpus.exit_code(report, True), 1)

    def test_timeout(self):
        report = self.run_report("timeout", timeout=0.1)
        sample = report["results"][0]["candidate"]["samples"][0]
        self.assertEqual(sample["status"], "timeout")
        self.assertLess(sample["seconds"], 3)
        self.assertEqual(shader_corpus.exit_code(report), 1)

    def test_crash_and_missing_or_invalid_artifacts(self):
        for mode, status in [("crash", "failed"), ("missing", "invalid_artifact"), ("invalid", "invalid_artifact")]:
            with self.subTest(mode=mode):
                sample = self.run_report(mode)["results"][0]["candidate"]["samples"][0]
                self.assertEqual(sample["status"], status)

    def test_bounded_log_tails(self):
        sample = self.run_report("logs")["results"][0]["candidate"]["samples"][0]
        for name, suffix in [("stdout", "TAIL"), ("stderr", "ERRTAIL")]:
            self.assertTrue(sample[name]["truncated"])
            self.assertTrue(sample[name]["text"].endswith(suffix))
            self.assertLessEqual(len(sample[name]["text"]), shader_corpus.LOG_LIMIT)
            self.assertGreater(sample[name]["bytes"], shader_corpus.LOG_LIMIT)

    def test_launch_failure(self):
        with mock.patch("shader_corpus.subprocess.Popen", side_effect=OSError("denied")):
            self.assertEqual(shader_corpus.replay_once(self.cases[0], self.build(), 1)["status"], "launch_error")

    def test_input_mutation(self):
        self.request.write_text("changed", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "capture changed"):
            self.run_report()

    def test_executable_mutation(self):
        build = self.build()
        build["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "executable changed"):
            shader_corpus.run_corpus(self.cases, build, repeats=1)

    def test_performance_thresholds(self):
        old = {"status": "passed", "median_seconds": 0.1, "artifact_hashes": ["same"]}
        new = dict(old, median_seconds=0.14)
        self.assertTrue(shader_corpus.compare_runs(old, new, 25, 25)["slowdown"])
        self.assertFalse(shader_corpus.compare_runs(old, new, 50, 25)["slowdown"])
        self.assertFalse(shader_corpus.compare_runs(old, new, 25, 50)["slowdown"])
        self.assertAlmostEqual(shader_corpus.compare_runs(old, new, 25, 25)["ratio"], 1.4)

    def test_parallel_result_identity_and_order(self):
        second = self.root / "other.req"
        second.write_text("fail", encoding="utf-8")
        cases = shader_corpus.collect_cases([second, self.request])
        report = shader_corpus.run_corpus(cases, self.build(), jobs=2, repeats=1)
        self.assertEqual([result["id"] for result in report["results"]], [case["id"] for case in cases])
        self.assertEqual(report["summary"]["passed"], 1)

    def test_parameter_bounds(self):
        for options in [{"jobs": 0}, {"jobs": 33}, {"jobs": True}, {"repeats": 0},
                        {"repeats": 10}, {"timeout": 0}, {"timeout": float("nan")},
                        {"slowdown_percent": -1}, {"minimum_ms": float("inf")}]:
            with self.subTest(options=options):
                with self.assertRaises(ValueError):
                    shader_corpus.run_corpus(self.cases, self.build(), **options)

    def test_artifact_header_and_bounds(self):
        path = self.root / "test.spv"
        for words in [(0, 0x10000, 1, 1, 0), (0x07230203, 0x10000, 1, 0, 0),
                      (0x07230203, 0x10000, 1, 1, 1), (0x07230203, 0, 1, 1, 0),
                      (0x07230203, 0x10700, 1, 1, 0), (0x07230203, 0x10001, 1, 1, 0)]:
            path.write_bytes(struct.pack("<5I", *words))
            with self.assertRaises(ValueError):
                shader_corpus.artifact_info(path)
        path.write_bytes(b"z" * 21)
        with self.assertRaises(ValueError):
            shader_corpus.artifact_info(path)
        with path.open("wb") as output:
            output.truncate(shader_corpus.ARTIFACT_LIMIT + 4)
        with self.assertRaises(ValueError):
            shader_corpus.artifact_info(path)

    def test_atomic_json_cleanup(self):
        path = self.root / "report.json"
        shader_corpus.write_report(path, {"version": 1})
        with mock.patch("shader_corpus.os.replace", side_effect=OSError("denied")):
            with self.assertRaises(OSError):
                shader_corpus.write_report(path, {"version": 2})
        self.assertEqual(json.loads(path.read_text(encoding="utf-8")), {"version": 1})
        self.assertEqual(len(list(self.root.iterdir())), 3)

    def cli(self, output, mode="normal", extra=()):
        return subprocess.run([sys.executable, str(Path(shader_corpus.__file__)), str(self.request),
                               "--replay", sys.executable, "--replay-arg", str(self.fixture),
                               "--replay-arg", mode, "--repeats", "1", "--json", str(output), *extra],
                              capture_output=True, text=True, timeout=5)

    def test_cli_report_and_exit_status(self):
        output = self.root / "report.json"
        self.assertEqual(self.cli(output).returncode, 0)
        self.assertEqual(json.loads(output.read_text(encoding="utf-8"))["summary"]["passed"], 1)
        self.assertEqual(self.cli(output, "crash").returncode, 1)
        self.assertEqual(self.cli(output, extra=["--timeout", "nan"]).returncode, 2)

    def test_cli_protects_inputs_and_executables(self):
        linked = self.root / "hardlink"
        os.link(self.request, linked)
        for output in [self.request, linked, Path(sys.executable)]:
            with self.subTest(output=output):
                self.assertEqual(self.cli(output).returncode, 2)
        self.assertEqual(self.request.read_text(encoding="utf-8"), "good")


if __name__ == "__main__":
    unittest.main()
