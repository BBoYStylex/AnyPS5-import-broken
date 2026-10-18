import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from check_test_registration import built, commands, registered


def make_build(files):
    directory = tempfile.TemporaryDirectory()
    root = Path(directory.name)
    for relative, content in files.items():
        path = root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
    return directory, root


class CommandsTests(unittest.TestCase):
    def test_add_test_plain_arguments(self):
        self.assertEqual(list(commands("add_test(foo /bin/foo --verbose)")),
                         [("add_test", ["foo", "/bin/foo", "--verbose"])])

    def test_command_name_is_case_insensitive(self):
        self.assertEqual(list(commands("ADD_TEST(foo bar)")), [("add_test", ["foo", "bar"])])

    def test_quoted_argument_unescapes(self):
        self.assertEqual(list(commands('add_test(t "a\\nb\\rc\\td" "e\\"f" "g\\\\h")')),
                         [("add_test", ["t", "a\nb\rc\td", 'e"f', "g\\h"])])

    def test_bracket_arguments(self):
        self.assertEqual(list(commands("add_test(t [[plain]] [==[wide]==])")),
                         [("add_test", ["t", "plain", "wide"])])

    def test_subdirs(self):
        self.assertEqual(list(commands("subdirs(one two)")), [("subdirs", ["one", "two"])])

    def test_set_tests_properties(self):
        content = "add_test(foo /bin/foo)\nset_tests_properties(foo PROPERTIES DISABLED TRUE)"
        self.assertEqual(list(commands(content)),
                         [("add_test", ["foo", "/bin/foo"]),
                          ("set_tests_properties", ["foo", "PROPERTIES", "DISABLED", "TRUE"])])

    def test_comments_are_blanked(self):
        content = "# leading comment\nadd_test(foo bar) # trailing\n#[[bracket\ncomment]]\nsubdirs(x)"
        self.assertEqual(list(commands(content)),
                         [("add_test", ["foo", "bar"]), ("subdirs", ["x"])])

    def test_multiple_commands(self):
        content = "add_test(a /x/a)\nadd_test(b /x/b)"
        self.assertEqual([name for name, _ in commands(content)], ["add_test", "add_test"])

    def test_invalid_argument_raises(self):
        with self.assertRaises(ValueError):
            list(commands("add_test(t ()"))

    def test_paren_inside_quotes_is_not_a_terminator(self):
        self.assertEqual(list(commands('add_test(t "a)b")')), [("add_test", ["t", "a)b"])])


class RegisteredTests(unittest.TestCase):
    def test_names_from_add_test(self):
        directory, root = make_build({"CTestTestfile.cmake": "add_test(foo /bin/foo)\nadd_test(bar /opt/bar)\n"})
        try:
            self.assertEqual(registered(root), {"foo", "bar"})
        finally:
            directory.cleanup()

    def test_exe_suffix_stripped(self):
        directory, root = make_build({"CTestTestfile.cmake": "add_test(bar C:/x/tests/bar.exe)\n"})
        try:
            self.assertEqual(registered(root), {"bar"})
        finally:
            directory.cleanup()

    def test_flags_and_assignments_skipped(self):
        directory, root = make_build({"CTestTestfile.cmake": "add_test(mytest /usr/bin/mytest --verbose MODE=fast)\n"})
        try:
            self.assertEqual(registered(root), {"mytest"})
        finally:
            directory.cleanup()

    def test_disabled_tests_excluded(self):
        content = ("add_test(foo /bin/foo)\nadd_test(bar /bin/bar)\n"
                   "set_tests_properties(foo PROPERTIES DISABLED ON)\n"
                   "set_tests_properties(bar PROPERTIES DISABLED OFF)\n")
        directory, root = make_build({"CTestTestfile.cmake": content})
        try:
            self.assertEqual(registered(root), {"bar"})
        finally:
            directory.cleanup()

    def test_disabled_true_values(self):
        for value in ("1", "ON", "YES", "TRUE", "Y", "on", "true"):
            content = f"add_test(foo /bin/foo)\nset_tests_properties(foo PROPERTIES DISABLED {value})\n"
            directory, root = make_build({"CTestTestfile.cmake": content})
            try:
                self.assertEqual(registered(root), set(), value)
            finally:
                directory.cleanup()

    def test_subdirs_recursed(self):
        files = {"CTestTestfile.cmake": "subdirs(sub)\nadd_test(top /bin/top)\n",
                 "sub/CTestTestfile.cmake": "add_test(inner /bin/inner)\n"}
        directory, root = make_build(files)
        try:
            self.assertEqual(registered(root), {"top", "inner"})
        finally:
            directory.cleanup()

    def test_missing_testfile_returns_empty(self):
        directory, root = make_build({})
        try:
            self.assertEqual(registered(root), set())
        finally:
            directory.cleanup()


class BuiltTests(unittest.TestCase):
    def test_names_from_ninja_targets(self):
        files = {"CMakeCache.txt": "CMAKE_MAKE_PROGRAM:=/usr/bin/ninja\nCMAKE_GENERATOR:=Ninja\n"}
        directory, root = make_build(files)
        output = "tests/foo: EXECUTABLE\ntests/helper: phony\ntests/bar.exe: EXECUTABLE\nother/baz: EXECUTABLE\n"
        try:
            with mock.patch("subprocess.run") as run:
                run.return_value = subprocess.CompletedProcess([], 0, stdout=output)
                self.assertEqual(built(root), {"foo", "bar"})
        finally:
            directory.cleanup()

    def test_non_ninja_generator_raises(self):
        files = {"CMakeCache.txt": "CMAKE_MAKE_PROGRAM:=/usr/bin/make\nCMAKE_GENERATOR:=Unix Makefiles\n"}
        directory, root = make_build(files)
        try:
            with self.assertRaises(ValueError):
                built(root)
        finally:
            directory.cleanup()

    def test_missing_program_raises(self):
        files = {"CMakeCache.txt": "CMAKE_MAKE_PROGRAM:=-NOTFOUND\nCMAKE_GENERATOR:=Ninja\n"}
        directory, root = make_build(files)
        try:
            with self.assertRaises(FileNotFoundError):
                built(root)
        finally:
            directory.cleanup()

    def test_win32_uses_codepage_encoding(self):
        files = {"CMakeCache.txt": "CMAKE_MAKE_PROGRAM:=C:/ninja/ninja.exe\nCMAKE_GENERATOR:=Ninja\n"}
        directory, root = make_build(files)
        codepage = subprocess.CompletedProcess([], 0, stdout="Build file encoding: ANSI")
        targets = subprocess.CompletedProcess([], 0, stdout="tests/foo: EXECUTABLE\n")
        try:
            with mock.patch.object(sys, "platform", "win32"), \
                    mock.patch("subprocess.run", side_effect=[codepage, targets]) as run:
                self.assertEqual(built(root), {"foo"})
                self.assertEqual(run.call_args_list[1].kwargs["encoding"], "mbcs")
        finally:
            directory.cleanup()

    def test_win32_unsupported_codepage_raises(self):
        files = {"CMakeCache.txt": "CMAKE_MAKE_PROGRAM:=C:/ninja/ninja.exe\nCMAKE_GENERATOR:=Ninja\n"}
        directory, root = make_build(files)
        codepage = subprocess.CompletedProcess([], 0, stdout="Build file encoding: Latin-1")
        try:
            with mock.patch.object(sys, "platform", "win32"), \
                    mock.patch("subprocess.run", return_value=codepage):
                with self.assertRaises(ValueError):
                    built(root)
        finally:
            directory.cleanup()


if __name__ == "__main__":
    unittest.main()
