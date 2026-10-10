import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import stubgen


class StubGeneratorTests(unittest.TestCase):
    def test_malformed_nid_is_rejected_before_generating_cpp(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "nids.txt"
            path.write_text('AAAAAAAAAAA\ninvalid"nid\n', encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "invalid NID"):
                stubgen.parse_nids(path)

    def test_code_and_debt_outputs_cannot_overwrite_each_other(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            nids = root / "nids.txt"
            nids.write_text("AAAAAAAAAAA\n", encoding="utf-8")
            output = root / "Export.cpp"
            arguments = ["--module", "libSceExample", "--nids", str(nids), "--out", str(output), "--td-out", str(output)]
            with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as error:
                stubgen.main(arguments)
            self.assertEqual(error.exception.code, 2)
            self.assertFalse(output.exists())

    def test_valid_annotations_comments_and_duplicates_are_preserved(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "nids.txt"
            path.write_text("# imports\nAAAAAAAAAAA#A#B\nAAAAAAAAAAA\nBBBBBBBBBBB\n", encoding="utf-8-sig")
            self.assertEqual(stubgen.parse_nids(path), ["AAAAAAAAAAA", "BBBBBBBBBBB"])

    def test_valid_generation_keeps_the_code_and_debt(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            nids = root / "nids.txt"
            nids.write_text("AAAAAAAAAAA\n", encoding="utf-8")
            code, debt = root / "Export.cpp", root / "debt.txt"
            arguments = ["--module", "libSceExample", "--nids", str(nids), "--out", str(code), "--td-out", str(debt)]
            self.assertEqual(stubgen.main(arguments), 0)
            self.assertIn('APS5_EXPORT("AAAAAAAAAAA",', code.read_text(encoding="utf-8"))
            self.assertIn("unknown name, signature", debt.read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
