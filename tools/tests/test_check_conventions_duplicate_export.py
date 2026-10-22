import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import check_conventions
from check_conventions import Check

STUB = """#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceShared(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
"""

IMPLEMENTED = """#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceShared(void) {
    return 0;
}

}
"""


def library_source(name, body):
    return (f'#include "prx/libc/include/General.hpp"\n\n'
            f"extern \"C\" {{\n\n"
            f"int APS5_VABI {name}(void) {{\n"
            f"{body}"
            f"    return 0;\n"
            f"}}\n\n"
            f"}}\n")


class DuplicateExportTests(unittest.TestCase):
    def check(self, first, second, expected):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)

            def git(*args):
                return subprocess.run(["git", "-c", f"safe.directory={root}", "-c", "user.name=Conventions Test",
                                       "-c", "user.email=test@example.com", "-c", "commit.gpgsign=false", *args],
                                      cwd=root, capture_output=True, check=True, encoding="utf-8").stdout

            def write(relative, text):
                path = root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text, encoding="utf-8")

            git("init", "-q")
            git("commit", "--allow-empty", "-qm", "chore: start")
            base = git("rev-parse", "HEAD").strip()
            write("core/libs/prx/libSceFirst/Export.cpp", first)
            write("core/libs/prx/libSceSecond/Export.cpp", second)
            git("add", ".")
            git("commit", "-qm", "feat: add the exports")
            head = git("rev-parse", "HEAD").strip()
            previous = Path.cwd()
            try:
                os.chdir(root)
                check = Check(base, head)
                check.code()
            finally:
                os.chdir(previous)
            return [finding for finding in check.findings if finding[0] == "duplicate-export"]

    def test_the_same_pending_export_in_two_libraries_is_allowed(self):
        first = library_source("sceShared", "    NotImplemented_nid_no_patch(__func__);\n")
        second = library_source("sceShared", "    NotImplemented_nid_no_patch(__func__);\n")
        self.assertEqual(self.check(first, second, None), [])

    def test_two_libraries_with_a_different_body_are_reported(self):
        first = library_source("sceShared", "    NotImplemented_nid_no_patch(__func__);\n")
        second = library_source("sceShared", "    return 0;\n")
        self.assertEqual(len(self.check(first, second, None)), 1)

    def test_the_order_of_the_libraries_does_not_matter(self):
        first = library_source("sceShared", "    NotImplemented_nid_no_patch(__func__);\n")
        second = library_source("sceShared", "    return 0;\n")
        self.assertEqual(len(self.check(second, first, None)), 1)


if __name__ == "__main__":
    unittest.main()
