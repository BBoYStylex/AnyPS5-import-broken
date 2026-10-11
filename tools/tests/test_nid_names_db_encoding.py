import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import nid_names


class DatabaseEncodingTests(unittest.TestCase):
    def load(self, data):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "aerolib.csv"
            path.write_bytes(data)
            return nid_names.load_db(path)

    def test_utf8_bom_is_not_part_of_the_first_nid(self):
        db = self.load(b"\xef\xbb\xbf" + "nid1 sceFirst\nnid2 sceSecond\n".encode("utf-8"))
        self.assertEqual(db.get("nid1"), "sceFirst")
        self.assertFalse(any("\ufeff" in key for key in db))

    def test_bytes_cp1252_cannot_decode_do_not_raise(self):
        db = self.load("nid1 sceFirst \u2010 note\nnid2 sceSecond\n".encode("utf-8") + b"\x81\x8d\n")
        self.assertEqual(db.get("nid1"), "sceFirst")
        self.assertEqual(db.get("nid2"), "sceSecond")


if __name__ == "__main__":
    unittest.main()
