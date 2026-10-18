import subprocess
import sys
import tempfile
import unittest
import urllib.parse
import urllib.request
from pathlib import Path
from unittest import mock

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "core/relinker/relinker/tests"))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import test_web

URL = "https://cdn.jsdelivr.net/npm/fake-zip@1.0/dist/zip.mjs"
ARCHIVE = 'import {x} from "https://cdn.jsdelivr.net/npm/fake-zip@1.0/dist/zip.mjs";\n'


class FakeResponse:
    def __init__(self, payload):
        self._payload = payload

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        return False

    def read(self):
        return self._payload


def make_build():
    directory = tempfile.TemporaryDirectory()
    build = Path(directory.name)
    web = build / "web"
    web.mkdir()
    (web / "archive.mjs").write_text(ARCHIVE, encoding="utf-8")
    return directory, build


class TestWebTests(unittest.TestCase):
    def test_main_runs_three_node_tests_in_order(self):
        directory, build = make_build()
        argv = ["test_web.py", "--build", str(build)]
        seen = []
        written = {}
        real_write_bytes = Path.write_bytes

        def spy_write(self, data):
            if self.name == "zip.mjs":
                written["zip.mjs"] = data
            return real_write_bytes(self, data)

        try:
            with mock.patch.object(sys, "argv", argv), \
                    mock.patch("urllib.request.urlopen", return_value=FakeResponse(b"ZIPDATA")), \
                    mock.patch.object(Path, "write_bytes", spy_write), \
                    mock.patch("subprocess.run") as run:
                run.side_effect = lambda *a, **k: seen.append((a, k)) or subprocess.CompletedProcess(a[0], 0)
                test_web.main()
            self.assertEqual(len(seen), 3)
            first, second, third = (call[0][0] for call in seen)
            self.assertEqual(first[:2], ["node", "--test"])
            self.assertTrue(first[2].endswith("output-directory.test.mjs"))
            self.assertEqual(second[0], "node")
            self.assertTrue(second[1].endswith("archive-root.test.mjs"))
            self.assertEqual(third[0], "node")
            self.assertTrue(third[1].endswith("archive.test.mjs"))
            environment = seen[1][1]["env"]
            self.assertEqual(environment["ANYPS5_TEST_ZIP_URL"], URL)
            self.assertTrue(environment["ANYPS5_TEST_ZIP_PATH"].startswith("file:"))
            self.assertEqual(written.get("zip.mjs"), b"ZIPDATA")
        finally:
            directory.cleanup()

    def test_missing_build_argument_exits_2(self):
        with mock.patch.object(sys, "argv", ["test_web.py"]), \
                self.assertRaises(SystemExit) as raised:
            test_web.main()
        self.assertEqual(raised.exception.code, 2)

    def test_missing_jsdelivr_url_raises(self):
        directory = tempfile.TemporaryDirectory()
        build = Path(directory.name)
        (build / "web").mkdir()
        (build / "web" / "archive.mjs").write_text("export const x = 1;\n", encoding="utf-8")
        try:
            with mock.patch.object(sys, "argv", ["test_web.py", "--build", str(build)]), \
                    mock.patch("subprocess.run"):
                with self.assertRaises(Exception):
                    test_web.main()
        finally:
            directory.cleanup()

    def test_node_failure_propagates(self):
        directory, build = make_build()
        argv = ["test_web.py", "--build", str(build)]
        try:
            with mock.patch.object(sys, "argv", argv), \
                    mock.patch("subprocess.run",
                               side_effect=subprocess.CalledProcessError(1, ["node"])):
                with self.assertRaises(subprocess.CalledProcessError):
                    test_web.main()
        finally:
            directory.cleanup()

    def test_fixture_elf_files_created(self):
        directory, build = make_build()
        argv = ["test_web.py", "--build", str(build)]
        written = {}
        real_write_bytes = Path.write_bytes
        def spy_write(self, data):
            if self.name in ("main.elf", "module.elf"):
                written[self.name] = data
            return real_write_bytes(self, data)
        try:
            with mock.patch.object(sys, "argv", argv), \
                    mock.patch("urllib.request.urlopen", return_value=FakeResponse(b"z")), \
                    mock.patch("subprocess.run"), \
                    mock.patch.object(Path, "write_bytes", spy_write):
                test_web.main()
            self.assertEqual(set(written), {"main.elf", "module.elf"})
            self.assertTrue(all(written[name].startswith(b"\x7fELF") for name in written))
        finally:
            directory.cleanup()


if __name__ == "__main__":
    unittest.main()
