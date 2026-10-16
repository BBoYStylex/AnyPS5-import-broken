import random
import unittest
from pathlib import Path

import guest_lab


class ExtractEndianTests(unittest.TestCase):
    def test_bextr_boundaries_aliases_and_defined_flags(self):
        cases, descriptions = [], []
        for bits in (32, 64):
            for memory in (False, True):
                for alias in (False, True):
                    a, c = ('eax', 'ecx') if bits == 32 else ('rax', 'rcx')
                    source = ('dword' if bits == 32 else 'qword') + ' ptr [rsi + 1]' if memory else a
                    cases.append({'asm': [f'bextr {c if alias else a}, {source}, {c}']})
                    descriptions.append((bits, memory, alias))
        with guest_lab.lab_directory() as directory:
            lab = guest_lab.Lab(directory)
            binary, roots = lab.lifted_binary(cases)
            rng = random.Random(916)
            records, expected = [], []
            for root, (bits, memory, alias) in zip(roots, descriptions):
                for start in (0, 1, bits - 1, bits, 255):
                    for length in (0, 1, bits - 1, bits, 255):
                        for sample in range(8):
                            value = rng.getrandbits(64)
                            control = (rng.getrandbits(48) << 16) | (length << 8) | start
                            data = bytearray([0xa5] * guest_lab.BUFFER_SIZE)
                            data[1:9] = value.to_bytes(8, 'little')
                            gpr = [0] * 16
                            gpr[0], gpr[1] = value, control
                            gpr[6], gpr[15] = guest_lab.BUFFER_MARK, guest_lab.SCRATCH_MARK
                            records.append((root - guest_lab.CODE, gpr, bytes(256), guest_lab.ALL_FLAGS | 2, bytes(data)))
                            result = ((value & ((1 << bits) - 1)) >> start) & ((1 << length) - 1)
                            expected.append((result, int(result == 0) << 6, bytes(data), int(alias)))
            source, output = Path(directory) / 'input', Path(directory) / 'output'
            lab.write_inputs(records, source)
            guest_lab.run([str(binary), str(source), str(output)])
            for actual, (value, flags, memory, register) in zip(lab.read_outputs(output, len(records)), expected):
                self.assertEqual(actual[0][register], value)
                self.assertEqual(actual[2] & 0x841, flags)
                self.assertEqual(actual[3], memory)

    def test_movbe_widths_loads_stores_and_flags(self):
        cases = []
        for register, size in [('ax', 'word'), ('eax', 'dword'), ('rax', 'qword')]:
            cases += [{'asm': [f'movbe {register}, {size} ptr [rsi + 1]'], 'pointers': {'rsi': 32}},
                      {'asm': [f'movbe {size} ptr [rsi + 1], {register}'], 'pointers': {'rsi': 32}}]
        with guest_lab.lab_directory() as directory:
            failures = guest_lab.Lab(directory).compare(cases, samples=64, seed=115)
        self.assertEqual(failures, [])


if __name__ == '__main__':
    unittest.main()
