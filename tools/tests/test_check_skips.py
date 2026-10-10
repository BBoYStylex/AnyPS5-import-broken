import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import check_skips


class SkipReportTests(unittest.TestCase):
    def skipped(self, content):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "ctest.xml"
            path.write_text(content, encoding="utf-8")
            return list(check_skips.skips(path))

    def test_skip_message_preserves_the_device_reason(self):
        found = self.skipped('<testsuite><testcase name="gpu"><skipped message="the device has no required extension"/></testcase></testsuite>')
        self.assertEqual(len(found), 1)
        self.assertRegex(found[0][1], check_skips.DEVICE_LIMITATION)

    def test_stderr_preserves_the_device_reason(self):
        found = self.skipped('<testsuite><testcase name="gpu"><skipped/><system-err>skipped, no display</system-err></testcase></testsuite>')
        self.assertEqual(len(found), 1)
        self.assertRegex(found[0][1], check_skips.DEVICE_LIMITATION)

    def test_unnamed_skipped_test_is_an_error(self):
        with self.assertRaises(ValueError):
            self.skipped('<testsuite><testcase><skipped/></testcase></testsuite>')

    def test_unexpected_reason_stays_unexpected(self):
        found = self.skipped('<testsuite><testcase name="gpu"><skipped message="producer finished too early"/></testcase></testsuite>')
        self.assertIsNone(check_skips.DEVICE_LIMITATION.search(found[0][1]))

    def test_ctest_stdout_is_retained(self):
        found = self.skipped('<testsuite><testcase name="gpu"><skipped/><system-out>skipped, the device reports too few descriptors</system-out></testcase></testsuite>')
        self.assertRegex(found[0][1], check_skips.DEVICE_LIMITATION)

    def test_message_and_element_text_are_both_retained(self):
        found = self.skipped('<testsuite><testcase name="gpu"><skipped message="unsupported">the device has no required extension</skipped></testcase></testsuite>')
        self.assertRegex(found[0][1], check_skips.DEVICE_LIMITATION)


if __name__ == "__main__":
    unittest.main()
