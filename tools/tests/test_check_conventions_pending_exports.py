import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

CHECKER = Path(__file__).resolve().parents[1] / "check_conventions.py"
SOURCE = "core/libs/prx/libScePending/Export.cpp"
OTHER = "core/libs/prx/libScePending/Other.cpp"
FOREIGN = "core/libs/prx/libSceOther/Export.cpp"
DEBT = "docs/dev/debt/libScePending.md"
FOREIGN_DEBT = "docs/dev/debt/libSceOther.md"
PRESENT = "int APS5_VABI scePresent(int value) { return value; }\n"
PENDING = "int APS5_VABI scePending(void) {\n    NotImplemented_nid_no_patch(__func__);\n    return 0;\n}\n"
STUB = "int APS5_VABI sceStub(void) {\n    (void)sceStub;\n    return 0;\n}\n"
WITH_PARAMETER = "int APS5_VABI sceWithParameter(int value) {\n    (void)value;\n    NotImplemented_nid_no_patch(__func__);\n    return 0;\n}\n"
THROWS = "int APS5_VABI sceThrows(void) {\n    throw std::runtime_error(\"\");\n}\n"
NAMED = ("# libScePending technical debt\n\n### Unknown function info\n\n"
         "- [scePending](../../../core/libs/prx/libScePending/Other.cpp) (libScePending) - the export's signature is unknown, so it is declared without parameters and throws\n")
BY_FILE = ("# libScePending technical debt\n\n### Unknown function info\n\n"
           "- [libScePending pending exports](../../../core/libs/prx/libScePending/Export.cpp) - one named callable export is present in the PS5 3.20 export table; its signature and behaviour are unverified\n")
FOREIGN_NAMED = ("# libSceOther technical debt\n\n### Unknown function info\n\n"
                 "- [scePending](../../../core/libs/prx/libSceOther/Export.cpp) (libSceOther) - the export is unknown here too\n")


class PendingExportTests(unittest.TestCase):
    def check(self, export, debt=None, foreign=None):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)

            def git(*args):
                return subprocess.run(
                    ["git", "-c", f"safe.directory={root}", "-c", "user.name=Conventions Test",
                     "-c", "user.email=test@example.com", "-c", "commit.gpgsign=false", *args],
                    cwd=root, capture_output=True, text=True, check=True,
                )

            def write(name, text):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text, encoding="utf-8")

            git("init", "-q")
            write(SOURCE, PRESENT)
            write(OTHER, "")
            write(FOREIGN, "")
            git("add", ".")
            git("commit", "-qm", "feat: add the library")
            write(SOURCE, PRESENT + export)
            if debt is not None:
                write(DEBT, debt)
            if foreign is not None:
                write(FOREIGN_DEBT, foreign)
            git("add", ".")
            git("commit", "-qm", "feat: add the export")
            env = {key: value for key, value in os.environ.items()
                   if key not in ("GITHUB_EVENT_PATH", "GITHUB_STEP_SUMMARY", "GITHUB_ACTIONS")}
            env.update(GIT_CONFIG_COUNT="1", GIT_CONFIG_KEY_0="safe.directory", GIT_CONFIG_VALUE_0=str(root))
            return subprocess.run(
                [sys.executable, "-I", str(CHECKER), "--base", "HEAD~1", "--head", "HEAD"],
                cwd=root, capture_output=True, text=True, env=env, timeout=20,
            )

    def test_pending_export_without_a_debt_entry_is_reported(self):
        result = self.check(PENDING)
        self.assertIn("[unrecorded-pending-export]", result.stdout, result.stdout + result.stderr)
        self.assertIn("scePending", result.stdout, result.stdout + result.stderr)
        self.assertEqual(result.stdout.count("[unrecorded-pending-export]"), 1, result.stdout)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_pending_export_named_in_the_debt_file_is_accepted(self):
        result = self.check(PENDING, debt=NAMED)
        self.assertNotIn("[unrecorded-pending-export]", result.stdout, result.stdout + result.stderr)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_pending_export_recorded_by_its_file_is_accepted(self):
        result = self.check(PENDING, debt=BY_FILE)
        self.assertNotIn("[unrecorded-pending-export]", result.stdout, result.stdout + result.stderr)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_pending_export_recorded_in_another_library_is_reported(self):
        result = self.check(PENDING, foreign=FOREIGN_NAMED)
        self.assertIn("[unrecorded-pending-export]", result.stdout, result.stdout + result.stderr)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_pending_export_is_not_a_silent_stub(self):
        result = self.check(PENDING)
        self.assertNotIn("[silent-stub]", result.stdout, result.stdout + result.stderr)

    def test_silent_stub_is_still_reported_and_is_not_a_pending_export(self):
        result = self.check(STUB)
        self.assertIn("[silent-stub]", result.stdout, result.stdout + result.stderr)
        self.assertNotIn("[unrecorded-pending-export]", result.stdout, result.stdout + result.stderr)

    def test_export_with_parameters_is_not_a_pending_export(self):
        result = self.check(WITH_PARAMETER)
        self.assertNotIn("[unrecorded-pending-export]", result.stdout, result.stdout + result.stderr)

    def test_export_that_throws_without_the_sanctioned_call_is_not_a_pending_export(self):
        result = self.check(THROWS)
        self.assertNotIn("[unrecorded-pending-export]", result.stdout, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
