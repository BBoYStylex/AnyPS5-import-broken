import struct
import sys
import unittest
from pathlib import Path

from test_symbol_versions import build_image, symbols, patch, SHOFF


class DynstrRangeTests(unittest.TestCase):
    def reject(self, image, message):
        result, after = patch(PATCHER, bytes(image), "dynstr.so")
        self.assertEqual(result.returncode, 2, (result.returncode, result.stderr))
        self.assertIn(message, result.stderr)
        self.assertEqual(after, image)

    def test_dynstr_past_end_of_file_is_rejected(self):
        for size in (0xd40, 0x2000, 0x100000, 2**64 - 1):
            with self.subTest(size=size):
                image = build_image(symbols(False), False)
                struct.pack_into("<Q", image, SHOFF + 2 * 64 + 32, size)
                self.reject(image, b".dynstr extends past the end of the file")

    def test_dynstr_offset_past_end_of_file_is_rejected(self):
        image = build_image(symbols(False), False)
        struct.pack_into("<Q", image, SHOFF + 2 * 64 + 24, len(image) + 1)
        self.reject(image, b".dynstr extends past the end of the file")

    def test_dynsym_link_outside_section_table_is_rejected(self):
        image = build_image(symbols(False), False)
        struct.pack_into("<I", image, SHOFF + 1 * 64 + 40, 100)
        self.reject(image, b"outside the section header table")

    def test_dynsym_link_to_non_string_table_is_rejected(self):
        image = build_image(symbols(False), False)
        struct.pack_into("<I", image, SHOFF + 1 * 64 + 40, 3)
        self.reject(image, b"not a string table")


if __name__ == "__main__":
    PATCHER = Path(sys.argv.pop(1)).resolve()
    unittest.main()
