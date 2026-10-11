from pathlib import Path
import contextlib
import io
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import stubgen


def parse(data):
    with tempfile.TemporaryDirectory() as d:
        p = Path(d) / "nids.txt"
        p.write_bytes(data)
        with contextlib.redirect_stderr(io.StringIO()):
            return stubgen.parse_nids(str(p))


class NidValidationTests(unittest.TestCase):
    def test_valid_nids_parse(self):
        got = parse(b"AC1FtjqMCL0\n# note\nab+-0123456 # tail\nAC1FtjqMCL0\n")
        self.assertEqual(got, ["AC1FtjqMCL0", "ab+-0123456"])

    def test_hex_token_rejected(self):
        with self.assertRaises(SystemExit) as cm:
            parse(b"AC1FtjqMCL0\n0x1234\n")
        self.assertEqual(cm.exception.code, 2)

    def test_word_token_rejected(self):
        with self.assertRaises(SystemExit) as cm:
            parse(b"sceFoo\n")
        self.assertEqual(cm.exception.code, 2)

    def test_utf16_file_rejected(self):
        with self.assertRaises(SystemExit) as cm:
            parse("AC1FtjqMCL0\n".encode("utf-16"))
        self.assertEqual(cm.exception.code, 2)


if __name__ == "__main__":
    unittest.main()
