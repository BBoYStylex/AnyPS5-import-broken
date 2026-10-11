import importlib.util
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch


TOOL = Path(__file__).resolve().parents[1] / "relinker_build.py"
SPEC = importlib.util.spec_from_file_location("relinker_build", TOOL)
build = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(build)
RELINKER = None
if "--relinker" in sys.argv:
    index = sys.argv.index("--relinker")
    RELINKER = Path(sys.argv[index + 1]).resolve()
    del sys.argv[index:index + 2]


def fixture(size=0x1000):
    image = bytearray(size)
    image[:16] = b"\x7fELF\x02\x01\x01" + bytes(9)
    struct.pack_into("<HHIQQQIHHHHHH", image, 16,
                     3, 62, 1, 0x200, 64, 0, 0, 64, 56, 8, 64, 0, 0)
    image[0x200:0x206] = b"\xb8\x2a\x00\x00\x00\xc3"
    tags = [(5, 0x600), (10, 1), (6, 0x620), (11, 24), (7, 0x700), (8, 0), (9, 24), (0, 0)]
    struct.pack_into("<IIQQQQQQ", image, 64,
                     1, 7, 0, 0, 0, len(image), len(image), 0x1000)
    struct.pack_into("<IIQQQQQQ", image, 120,
                     2, 6, 0x400, 0x400, 0x400, len(tags) * 16, len(tags) * 16, 8)
    for index in range(2, 8):
        struct.pack_into("<IIQQQQQQ", image, 64 + index * 56, 0x6FFFFF01, 0, 0, 0, 0, 0, 0, 1)
    for index, tag in enumerate(tags):
        struct.pack_into("<qQ", image, 0x400 + index * 16, *tag)
    return image


FAKE = '''import pathlib, sys, time
source, output = map(pathlib.Path, sys.argv[-2:])
data = source.read_bytes()
(output.parent / "partial").write_bytes(b"partial")
if data == b"fail":
    sys.exit(3)
if data == b"slow":
    time.sleep(2)
if data == b"mutate":
    source.write_bytes(b"changed")
output.write_bytes(data + repr(sys.argv[1:-2]).encode())
(output.parent / "module.guest.prx").write_bytes(b"module")
if "--registry" in sys.argv:
    output.with_suffix(".registry.json").write_text("[]")
'''


class BuildTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.source = self.root / "title.elf"
        self.source.write_bytes(b"title")
        self.fake = self.root / "fake.py"
        self.fake.write_text(FAKE, encoding="utf-8")
        self.command = [sys.executable, str(self.fake)]
        self.cache = self.root / "cache"
        self.manifest = self.root / "manifest.json"
        self.entries = [{"id": "title-linux", "input": "title.elf", "output": "out/linux/title", "options": ["--skip-sce-module", "--registry"]}]
        self.save()

    def save(self):
        self.manifest.write_text(json.dumps({"version": 1, "builds": self.entries}), encoding="utf-8")

    def run_builds(self, **kwargs):
        return build.run_builds(self.manifest, self.cache, self.command, **kwargs)

    def status(self, **kwargs):
        return self.run_builds(**kwargs)["builds"][0]["status"]

    def test_warm_cache_does_not_execute_converter(self):
        self.assertEqual(self.status(), "built")
        with patch.object(build.subprocess, "run", side_effect=AssertionError("converter invoked")):
            self.assertEqual(self.status(), "cached")

    def test_missing_and_changed_outputs_are_restored(self):
        self.assertEqual(self.status(), "built")
        output = self.root / self.entries[0]["output"]
        original = output.read_bytes()
        output.unlink()
        (output.parent / "module.guest.prx").write_bytes(b"broken")
        self.assertEqual(self.status(), "cached")
        self.assertEqual(output.read_bytes(), original)
        self.assertEqual((output.parent / "module.guest.prx").read_bytes(), b"module")

    def test_same_timestamp_input_change_invalidates(self):
        self.assertEqual(self.status(), "built")
        original = self.source.stat()
        self.source.write_bytes(b"other")
        os.utime(self.source, ns=(original.st_atime_ns, original.st_mtime_ns))
        self.assertEqual(self.status(), "built")

    def test_output_permissions_are_restored(self):
        self.assertEqual(self.status(), "built")
        output = self.root / self.entries[0]["output"]
        original = output.stat().st_mode & 0o777
        output.chmod(0o444 if original != 0o444 else 0o666)
        self.assertEqual(self.status(), "cached")
        self.assertEqual(output.stat().st_mode & 0o777, original)

    def test_stale_generated_files_are_removed_after_option_change(self):
        self.assertEqual(self.status(), "built")
        registry = (self.root / self.entries[0]["output"]).with_suffix(".registry.json")
        self.assertTrue(registry.exists())
        self.entries[0]["options"].remove("--registry")
        self.save()
        self.assertEqual(self.status(), "built")
        self.assertFalse(registry.exists())

    def test_converter_and_options_invalidate(self):
        self.assertEqual(self.status(), "built")
        self.fake.write_text(FAKE + "\n", encoding="utf-8")
        self.assertEqual(self.status(), "built")
        self.entries[0]["options"].append("--to-intel")
        self.save()
        self.assertEqual(self.status(), "built")

    def test_module_and_metadata_changes_invalidate(self):
        self.entries[0]["options"] = ["--registry", "--module-dir", "extra"]
        self.save()
        self.assertEqual(self.status(), "built")
        module_dir = self.root / "sce_module"
        module_dir.mkdir()
        self.assertEqual(self.status(), "built")
        module = module_dir / "dependency.prx"
        module.write_bytes(b"first")
        self.assertEqual(self.status(), "built")
        module.write_bytes(b"second")
        self.assertEqual(self.status(), "built")
        extra = self.root / "extra"
        extra.mkdir()
        (extra / "custom.prx").write_bytes(b"custom")
        self.assertEqual(self.status(), "built")
        metadata = self.root / "sce_sys"
        metadata.mkdir()
        (metadata / "param.json").write_text('{}', encoding="utf-8")
        self.assertEqual(self.status(), "built")
        (metadata / "icon0.png").write_bytes(b"icon")
        self.assertEqual(self.status(), "built")

    def test_corrupt_cache_is_rebuilt(self):
        result = self.run_builds()["builds"][0]
        entry = self.cache / result["cache_key"]
        (entry / "files" / "title").write_bytes(b"bad")
        self.assertEqual(self.status(), "built")
        data = json.loads((entry / "entry.json").read_text(encoding="utf-8"))
        data["artifacts"]["../escape"] = data["artifacts"]["title"]
        (entry / "entry.json").write_text(json.dumps(data), encoding="utf-8")
        self.assertEqual(self.status(), "built")
        self.assertFalse((self.root / "escape").exists())

    def test_failures_are_not_cached_and_retry_is_supported(self):
        self.source.write_bytes(b"fail")
        result = self.run_builds()
        self.assertEqual(result["summary"]["failed"], 1)
        self.assertIn("exited with 3", result["builds"][0]["error"])
        self.assertFalse(self.cache.exists())
        self.source.write_bytes(b"success")
        self.assertEqual(self.status(), "built")

    def test_changed_input_during_build_is_not_cached(self):
        self.source.write_bytes(b"mutate")
        result = self.run_builds()["builds"][0]
        self.assertEqual(result["status"], "failed")
        self.assertIn("changed during conversion", result["error"])
        self.assertFalse(self.cache.exists())

    def test_timeout_can_retry(self):
        self.source.write_bytes(b"slow")
        self.assertEqual(self.status(timeout=0.05), "failed")
        self.assertFalse(self.cache.exists())
        self.source.write_bytes(b"fast")
        self.assertEqual(self.status(), "built")

    def test_unowned_files_are_preserved(self):
        output = self.root / "out" / "linux"
        output.mkdir(parents=True)
        resource = output / "valuable.txt"
        resource.write_bytes(b"preserve")
        self.assertEqual(self.status(), "failed")
        self.assertEqual(resource.read_bytes(), b"preserve")
        resource.unlink()
        self.assertEqual(self.status(), "built")
        resource.write_bytes(b"preserve")
        self.assertEqual(self.status(), "failed")
        self.assertEqual(resource.read_bytes(), b"preserve")

    def test_concurrent_output_lock_is_respected(self):
        output = self.root / "out" / "linux"
        output.parent.mkdir()
        lock = output.parent / "linux.anyps5-lock"
        lock.write_text("another process", encoding="utf-8")
        self.assertEqual(self.status(), "failed")
        self.assertEqual(lock.read_text(encoding="utf-8"), "another process")

    def test_parallel_jobs_overlap_and_results_keep_manifest_order(self):
        self.entries.append({"id": "title-windows", "input": "title.elf", "output": "out/windows/title.exe", "target": "windows", "options": ["--skip-sce-module"]})
        self.save()
        barrier = threading.Barrier(2)
        original = build.subprocess.run

        def simultaneous(*args, **kwargs):
            barrier.wait(timeout=5)
            return original(*args, **kwargs)

        with patch.object(build.subprocess, "run", side_effect=simultaneous):
            result = self.run_builds(workers=2)
        self.assertEqual(result["summary"]["built"], 2)
        self.assertEqual([item["id"] for item in result["builds"]], [entry["id"] for entry in self.entries])

    def test_one_failed_job_does_not_stop_others(self):
        (self.root / "broken.elf").write_bytes(b"fail")
        self.entries.append({"id": "broken", "input": "broken.elf", "output": "out/broken/title", "options": ["--skip-sce-module"]})
        self.save()
        self.assertEqual(self.run_builds(workers=2)["summary"], {"built": 1, "cached": 0, "failed": 1})

    def test_manifest_rejects_overlaps_and_unsupported_execution(self):
        for change in ({"output": "title.elf"}, {"options": ["--autorun"]}, {"options": ["--module-dir", "../escape"]}, {"target": "unknown"}):
            with self.subTest(change=change):
                entry = dict(self.entries[0])
                self.entries[0].update(change)
                self.save()
                with self.assertRaises(ValueError):
                    self.run_builds()
                self.entries[0] = entry
        self.entries.append({"id": "duplicate-root", "input": "title.elf", "output": "out/linux/other", "options": ["--skip-sce-module"]})
        self.save()
        with self.assertRaises(ValueError):
            self.run_builds()

    def test_invalid_worker_and_timeout_values(self):
        for kwargs in ({"workers": 0}, {"timeout": float("inf")}, {"timeout": -1}):
            with self.assertRaises(ValueError):
                self.run_builds(**kwargs)

    def test_manifest_relative_module_root(self):
        modules = self.root / "dependencies"
        modules.mkdir()
        (modules / "sce_module").mkdir()
        self.entries[0]["options"] = ["--sce-module-path", "dependencies"]
        self.save()
        jobs = build.load_jobs(self.manifest, self.cache, self.command)
        self.assertIn(str(modules), jobs[0]["options"])
        self.assertIn(modules / "sce_module", jobs[0]["dependencies"])

    def test_cli_help_and_failure_exit(self):
        result = subprocess.run([sys.executable, str(TOOL), "--help"], capture_output=True, timeout=5)
        self.assertEqual(result.returncode, 0)
        self.source.write_bytes(b"fail")
        result = subprocess.run([sys.executable, str(TOOL), str(self.manifest), "--relinker", str(self.root / "missing")], capture_output=True, timeout=5)
        self.assertEqual(result.returncode, 2)


@unittest.skipUnless(RELINKER, "pass --relinker to exercise native conversion")
class NativeBuildTests(unittest.TestCase):
    def test_real_guest_modules_are_cached_restored_and_invalidated(self):
        sys.path.insert(0, str(TOOL.parents[1] / "core/relinker/relinker/tests"))
        from test_guest_symbol_names import guest_module, importing_executable

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "homebrew.elf"
            source.write_bytes(importing_executable(["AAAAAAAAAAA#A#A"]))
            modules = root / "sce_module"
            modules.mkdir()
            module = modules / "libc.prx"
            module.write_bytes(guest_module([("AAAAAAAAAAA#A#A", True)]))
            manifest = root / "manifest.json"
            manifest.write_text(json.dumps({"version": 1, "builds": [{"id": "guest", "input": source.name,
                                "output": "output/homebrew", "options": ["--registry"]}]}), encoding="utf-8")
            command = [str(RELINKER)]
            first = build.run_builds(manifest, root / "cache", command, timeout=10)
            self.assertEqual(first["summary"]["built"], 1, first)
            artifact = root / "output/app0/sce_module/libc.prx.guest.prx"
            self.assertTrue(artifact.is_file())
            original = artifact.read_bytes()
            artifact.unlink()
            self.assertEqual(build.run_builds(manifest, root / "cache", command)["summary"]["cached"], 1)
            self.assertEqual(artifact.read_bytes(), original)
            data = bytearray(module.read_bytes())
            data[-1] ^= 1
            module.write_bytes(data)
            updated = build.run_builds(manifest, root / "cache", command)
            self.assertEqual(updated["summary"]["built"], 1, updated)
            self.assertNotEqual(first["builds"][0]["cache_key"], updated["builds"][0]["cache_key"])

    def test_native_formats_cache_restoration_and_execution(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "homebrew.elf"
            source.write_bytes(fixture())
            linux_source = root / "linux.elf"
            linux_image = fixture()
            linux_image[0x200:0x20C] = bytes.fromhex("bf 2a 00 00 00 b8 3c 00 00 00 0f 05")
            linux_source.write_bytes(linux_image)
            entries = [{"id": target, "input": source.name, "output": target + "/homebrew",
                        "target": target, "options": ["--skip-sce-module", "--registry"]} for target in build.TARGETS]
            entries[0]["input"] = linux_source.name
            entries[0]["options"].append("--skip-syscall-check")
            manifest = root / "manifest.json"
            manifest.write_text(json.dumps({"version": 1, "builds": entries}), encoding="utf-8")
            result = build.run_builds(manifest, root / "cache", [str(RELINKER)], workers=3, timeout=10)
            self.assertEqual(result["summary"], {"built": 3, "cached": 0, "failed": 0}, result)
            expected = {"linux": b"\x7fELF", "windows": b"MZ", "macos": b"\xcf\xfa\xed\xfe"}
            originals = {}
            for entry in entries:
                output = root / entry["output"]
                originals[entry["id"]] = output.read_bytes()
                self.assertTrue(originals[entry["id"]].startswith(expected[entry["id"]]))
                output.unlink()
            result = build.run_builds(manifest, root / "cache", [str(RELINKER)], workers=3, timeout=10)
            self.assertEqual(result["summary"]["cached"], 3, result)
            for entry in entries:
                self.assertEqual((root / entry["output"]).read_bytes(), originals[entry["id"]])
            command = [sys.executable, str(TOOL), str(manifest), "--relinker", str(RELINKER),
                       "--cache-dir", str(root / "cache"), "--jobs", "3"]
            cached = subprocess.run(command, capture_output=True, text=True, timeout=10)
            self.assertEqual(cached.returncode, 0, cached.stderr)
            self.assertEqual(json.loads(cached.stdout)["summary"]["cached"], 3)
            if sys.platform.startswith("linux"):
                (root / "linux/homebrew").chmod(0o755)
                self.assertEqual(subprocess.run([str(root / "linux/homebrew")], timeout=5).returncode, 42)
            source.write_bytes(bytes(0x1000))
            failed = subprocess.run(command, capture_output=True, text=True, timeout=10)
            self.assertEqual(failed.returncode, 1, failed.stderr)
            self.assertEqual(json.loads(failed.stdout)["summary"], {"built": 0, "cached": 1, "failed": 2})


if __name__ == "__main__":
    unittest.main()
