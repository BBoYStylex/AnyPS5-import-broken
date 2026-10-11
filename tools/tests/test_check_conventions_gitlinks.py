import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

CHECKER = Path(__file__).resolve().parents[1] / "check_conventions.py"
SHA = "1234567890123456789012345678901234567890"


def run_with_gitlink(path):
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)

        def git(*args):
            return subprocess.run(
                ["git", "-c", "user.name=Conventions Test", "-c", "user.email=test@example.com",
                 "-c", "commit.gpgsign=false", *args],
                cwd=root, capture_output=True, text=True, check=True,
            )

        git("init", "-q")
        (root / "README.md").write_text("# Example\n", encoding="utf-8")
        git("add", ".")
        git("commit", "-qm", "docs: add readme")
        git("update-index", "--add", "--cacheinfo", f"160000,{SHA},{path}")
        git("commit", "-qm", "build: add submodule")
        return subprocess.run(
            [sys.executable, "-I", str(CHECKER), "--base", "HEAD~1"],
            cwd=root, capture_output=True, text=True, encoding="utf-8",
            env={key: value for key, value in os.environ.items()
                 if key not in ("GITHUB_EVENT_PATH", "GITHUB_STEP_SUMMARY", "GITHUB_ACTIONS")},
        )


class GitlinkTests(unittest.TestCase):
    def test_gitlink_outside_3rdparty_is_reported(self):
        result = run_with_gitlink("imgui")
        self.assertNotIn("Traceback", result.stderr)
        self.assertIn("[system-dependency]", result.stdout)
        self.assertIn("imgui", result.stdout)

    def test_gitlink_under_3rdparty_is_silent(self):
        result = run_with_gitlink("3rdparty/x")
        self.assertNotIn("Traceback", result.stderr)
        self.assertNotIn("[system-dependency]", result.stdout)


if __name__ == "__main__":
    unittest.main()
