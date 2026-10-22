import contextlib
import io
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import audit_dump
import unwrap_self

SELF = b"\x4f\x15\x3d\x1d" + b"\0" * 60
ELF = b"\x7fELF" + b"\0" * 60


def fake_unwrap(source, destination, verbose=False):
    data = Path(source).read_bytes()
    if not data.startswith(SELF[:4]):
        raise unwrap_self.Refused("not a SELF container")
    Path(destination).parent.mkdir(parents=True, exist_ok=True)
    Path(destination).write_bytes(ELF + Path(source).name.encode())
    return len(data)


class AuditDumpTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name)
        self.dump = self.root / "app0"
        self.dump.mkdir()
        (self.dump / "eboot.bin").write_bytes(SELF)
        self.relinker = self.root / "relinker.exe"
        self.relinker.write_bytes(b"")
        patcher = mock.patch.object(unwrap_self, "unwrap", side_effect=fake_unwrap)
        patcher.start()
        self.addCleanup(patcher.stop)

    def tearDown(self):
        self.directory.cleanup()

    def module(self, relative, data=SELF):
        path = self.dump / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)

    def test_every_module_location_is_staged_and_the_extra_ones_become_module_dirs(self):
        self.module("sce_module/libc.prx")
        self.module("Media/Modules/Il2CppUserAssemblies.prx")
        self.module("Media/Plugins/PSNCore.prx", ELF)
        self.module("libfmod.prx")
        self.module("Media/Plugins/readme.txt", b"text")
        (self.dump / "sce_modules").mkdir()
        source = self.root / "work" / "source"
        directories, extra = audit_dump.prepare(self.dump, source, log=lambda text: None)
        self.assertTrue((source / "eboot.elf").read_bytes().startswith(ELF))
        self.assertTrue((source / "sce_module" / "libc.prx").read_bytes().endswith(b"libc.prx"))
        self.assertEqual((source / "Media/Plugins/PSNCore.prx").read_bytes(), ELF)
        self.assertTrue((source / "libfmod.prx").is_file())
        self.assertFalse((source / "Media/Plugins/readme.txt").exists())
        self.assertEqual(extra, ["Media/Modules", "Media/Plugins", "."])
        self.assertEqual(directories, [source / "sce_module", source / "Media/Modules", source / "Media/Plugins", source])

    def test_a_module_that_is_neither_a_self_nor_an_elf_is_refused(self):
        self.module("sce_module/broken.prx", b"MZ" + b"\0" * 62)
        with self.assertRaises(unwrap_self.Refused):
            audit_dump.prepare(self.dump, self.root / "source", log=lambda text: None)

    def test_the_relinker_gets_the_registry_windows_and_module_dir_options(self):
        with mock.patch.object(subprocess, "run", return_value=subprocess.CompletedProcess([], 0, "", "")) as run:
            audit_dump.convert(self.relinker, self.root / "source", self.root / "out" / "app.exe", ["Media/Modules", "."], True)
        command = run.call_args.args[0]
        self.assertEqual(command[:3], [str(self.relinker), "--registry", "--windows"])
        self.assertEqual(command[3:7], ["--module-dir", "Media/Modules", "--module-dir", "."])
        self.assertEqual(command[7:], [str(self.root / "source" / "eboot.elf"), str(self.root / "out" / "app.exe")])

    def run_main(self, *arguments):
        errors = io.StringIO()
        printed = io.StringIO()
        with contextlib.redirect_stderr(errors), contextlib.redirect_stdout(printed):
            code = audit_dump.main([str(argument) for argument in arguments])
        return code, errors.getvalue()

    def test_the_audit_gets_every_registry_library_and_module_directory(self):
        self.module("sce_module/libc.prx")
        work = self.root / "work"

        def relink(command, capture_output, text):
            output = Path(command[-1]).parent
            (output / "app.registry.json").write_text("[]")
            (output / "app.libc.prx.guest.prx.registry.json").write_text("[]")
            (output / "app.libc.prx.guest.prx.needed.json").write_text("{}")
            return subprocess.CompletedProcess(command, 0, "", "")

        with mock.patch.object(subprocess, "run", side_effect=relink), mock.patch.object(audit_dump.import_audit, "main", return_value=1) as audit:
            code, _ = self.run_main(self.dump, "--relinker", self.relinker, "--libs", "libs-a", "--libs", "libs-b", "--work", work, "--json", "report.json")
        self.assertEqual(code, 1)
        argv = audit.call_args.args[0]
        output = work / "output"
        self.assertEqual(argv[:2], [str(output / "app.libc.prx.guest.prx.registry.json"), str(output / "app.registry.json")])
        self.assertEqual(argv[2:], ["--libs", "libs-a", "--libs", "libs-b", "--modules", str(work / "source" / "sce_module"), "--json", "report.json"])

    def test_a_relinker_failure_returns_2_with_its_message(self):
        with mock.patch.object(subprocess, "run", return_value=subprocess.CompletedProcess([], 3, "", "bad input")), mock.patch.object(audit_dump.import_audit, "main") as audit:
            code, errors = self.run_main(self.dump, "--relinker", self.relinker, "--libs", "libs")
        self.assertEqual(code, 2)
        self.assertIn("relinker exited with 3", errors)
        self.assertIn("bad input", errors)
        audit.assert_not_called()

    def test_a_dump_without_eboot_or_a_missing_relinker_is_an_argument_error(self):
        (self.dump / "eboot.bin").unlink()
        with self.assertRaises(SystemExit):
            self.run_main(self.dump, "--relinker", self.relinker, "--libs", "libs")
        (self.dump / "eboot.bin").write_bytes(SELF)
        with self.assertRaises(SystemExit):
            self.run_main(self.dump, "--relinker", self.root / "missing.exe", "--libs", "libs")


if __name__ == "__main__":
    unittest.main()
