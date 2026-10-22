#!/usr/bin/env python3
import re
import sys
import unittest
from pathlib import Path

SOURCE_EXPORTS = (
    "sceNpSessionSignalingActivate",
    "sceNpSessionSignalingManualUdpAbortConnection",
    "sceNpSessionSignalingManualUdpCreateJoin",
    "sceNpSessionSignalingManualUdpEstablishConnection",
    "sceNpSessionSignalingManualUdpJoin",
    "sceNpSessionSignalingRegisterInternalCallback",
)
PENDING = re.compile(
    r"^int APS5_VABI (?P<name>\w+)\(void\) \{\n"
    r"    NotImplemented_nid_no_patch\(__func__\);\n"
    r"    return 0;\n"
    r"\}$", re.MULTILINE)
EXPORT = re.compile(r"\bAPS5_VABI\s+(\w+)\s*\([^;{]*\)\s*\{")


def tools_directory(library):
    for parent in [library, *library.parents]:
        if (parent / "tools" / "import_audit.py").is_file():
            return parent / "tools"
    raise SystemExit("the tools directory is not reachable from " + str(library))


if len(sys.argv) != 3:
    raise SystemExit("usage: test_export_nids.py <library source directory> <patched library>")
LIBRARY = Path(sys.argv[1]).resolve()
PATCHED = Path(sys.argv[2]).resolve()
sys.path.insert(0, str(tools_directory(LIBRARY)))
import import_audit
import nid_names


class PendingExportNidTests(unittest.TestCase):
    def source(self):
        return (LIBRARY / "Export.cpp").read_text(encoding="utf-8", errors="replace")

    def declared(self):
        return {match.group("name") for match in PENDING.finditer(self.source())}

    def exports(self):
        if not PATCHED.is_file():
            self.skipTest(str(PATCHED) + " is not built")
        return import_audit.read_exports(PATCHED)

    def test_every_source_export_is_declared_as_a_pending_export(self):
        declared = self.declared()
        for name in SOURCE_EXPORTS:
            with self.subTest(export=name):
                self.assertIn(name, declared)

    def test_every_declared_export_resolves_by_its_nid(self):
        exports = self.exports()
        for name in EXPORT.findall(self.source()):
            with self.subTest(export=name):
                self.assertIn(nid_names.compute_nid(name), exports)

    def test_a_source_export_resolves_by_its_nid(self):
        exports = self.exports()
        for name in SOURCE_EXPORTS:
            with self.subTest(export=name):
                self.assertIn(nid_names.compute_nid(name), exports)

    def test_a_pending_export_does_not_keep_its_plain_name(self):
        exports = self.exports()
        for name in self.declared():
            with self.subTest(export=name):
                self.assertNotIn(name, exports)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
