import hashlib
import struct
import unittest

import test_compiler
from compiler import Image, Rejected, compile_image
from toolchain import build_native_object


class PortableTests(unittest.TestCase):
    setUp = test_compiler.CompilerTests.setUp

    def lower(self, raw, contracts):
        source = self.directory / 'input.elf'
        source.write_bytes(raw)
        return compile_image(Image(source, contracts))

    def test_cross_compilation_produces_apple_arm64_machine_code(self):
        raw, contracts = test_compiler.fixture(bytes.fromhex('4889f84801f0c3'), ['u64', 'u64'], result='u64')
        ir, report = self.lower(raw, contracts)
        output = self.directory / 'native.o'
        result = build_native_object(ir, output)
        self.assertEqual(struct.unpack_from('<III', output.read_bytes())[:2], (0xfeedfacf, 0x100000c))
        self.assertEqual(result['machine_state_allocas'], 0)
        optimized = output.with_suffix('.optimized.ll').read_text()
        self.assertIn('add i64', optimized)
        self.assertNotIn('alloca', optimized)
        self.assertEqual(report['input_sha256'], hashlib.sha256(raw).hexdigest())

    def test_changed_binary_rejects_before_codegen(self):
        raw, contracts = test_compiler.fixture(bytes.fromhex('31c0c3'))
        altered = bytearray(raw)
        altered[0x1001] ^= 1
        with self.assertRaises(Rejected):
            self.lower(bytes(altered), contracts)

    def test_unproved_native_pointer_rejects(self):
        raw, contracts = test_compiler.fixture(bytes.fromhex('b800200000488b00c3'))
        with self.assertRaises(Rejected):
            self.lower(raw, contracts)

    def test_unimplemented_instruction_rejects(self):
        raw, contracts = test_compiler.fixture(bytes.fromhex('0fa2c3'))
        with self.assertRaises(Rejected):
            self.lower(raw, contracts)


if __name__ == '__main__':
    unittest.main()
