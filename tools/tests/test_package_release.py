import os
import sys
import tarfile
import tempfile
import unittest
import zipfile
from contextlib import contextmanager
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import package_release


@contextmanager
def working_directory(path):
    previous = Path.cwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(previous)


class ReleasePackageTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.source = self.root / "source"
        self.build = self.root / "build"
        self.output = self.root / "output"
        (self.source / "core/libs/prx/libc").mkdir(parents=True)
        (self.source / "core/libs/prx/libc/Export.cpp").write_text("", encoding="utf-8")
        libraries = self.build / "core/libs/libs"
        libraries.mkdir(parents=True)
        self.library = libraries / "libc.prx"
        self.library.write_bytes(b"library bytes")
        binary = self.build / "core/relinker/relinker"
        binary.parent.mkdir(parents=True)
        binary.write_bytes(b"relinker bytes")
        self.addCleanup(patch.stopall)
        patch.object(package_release, "ROOT", self.source, create=True).start()

    def test_source_discovery_does_not_depend_on_working_directory(self):
        with working_directory(self.root):
            package_release.package("linux", self.build, self.output, "v1.0")
        self.assertEqual((self.output / "relinker-v1.0").read_bytes(), b"relinker bytes")
        with zipfile.ZipFile(self.output / "prx-linux-v1.0.zip") as archive:
            self.assertEqual(archive.read("libs/libc.prx"), b"library bytes")

    def test_hidden_cache_directory_is_not_a_required_library(self):
        (self.source / "core/libs/prx/.cache").mkdir()
        with working_directory(self.source):
            package_release.package("linux", self.build, self.output, "v1.0")
        with zipfile.ZipFile(self.output / "prx-linux-v1.0.zip") as archive:
            self.assertEqual(archive.namelist(), ["libs/libc.prx"])

    def test_tar_and_zip_include_the_same_symlink_contents(self):
        target = self.library.with_name("libc.target")
        self.library.rename(target)
        try:
            self.library.symlink_to(target.name)
        except (OSError, NotImplementedError) as error:
            self.skipTest(str(error))
        with working_directory(self.source):
            package_release.package("linux", self.build, self.output, "v1.0")
        with tarfile.open(self.output / "prx-linux-v1.0.tar.gz") as archive:
            member = archive.getmember("libs/libc.prx")
            self.assertTrue(member.isfile())
            self.assertEqual(archive.extractfile(member).read(), b"library bytes")
        with zipfile.ZipFile(self.output / "prx-linux-v1.0.zip") as archive:
            self.assertEqual(archive.read("libs/libc.prx"), b"library bytes")

    def test_missing_real_library_still_fails(self):
        self.library.unlink()
        with working_directory(self.source), self.assertRaisesRegex(RuntimeError, "Missing patched libraries"):
            package_release.package("linux", self.build, self.output, "v1.0")

    def test_invalid_tag_creates_no_assets(self):
        with self.assertRaises(ValueError):
            package_release.package("linux", self.build, self.output, "../v1.0")
        self.assertFalse(self.output.exists())


if __name__ == "__main__":
    unittest.main()
