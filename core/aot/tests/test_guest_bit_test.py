import math
import random
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

import guest_lab


class GuestBitTestTests(unittest.TestCase):
    def test_vptest_flags_and_preserved_vectors(self):
        cases, widths = [], []
        for bits, register in ((128, 'xmm'), (256, 'ymm')):
            for memory in (False, True):
                source = f'{"xmmword" if bits == 128 else "ymmword"} ptr [rsi + 32]' if memory else register + '1'
                cases.append({'asm': ['vmovups ymm0, ymmword ptr [rsi]',
                                      'vmovups ymm1, ymmword ptr [rsi + 32]',
                                      f'vptest {register}0, {source}',
                                      'vmovups ymmword ptr [rsi + 128], ymm0',
                                      'vmovups ymmword ptr [rsi + 160], ymm1']})
                widths.append(bits)
        with guest_lab.lab_directory() as directory:
            lab = guest_lab.Lab(directory)
            binary, roots = lab.lifted_binary(cases)
            records, expected = [], []
            rng = random.Random(15517)
            for root, bits in zip(roots, widths):
                mask = (1 << bits) - 1
                pairs = [(0, 0), (mask, 0), (mask, mask), (0, mask), (1, 2), (1, 3),
                         (1 << (bits - 1), 1 << (bits - 1)), ((1 << 128) - 1, 1 << 200)]
                pairs += [(rng.getrandbits(256), rng.getrandbits(256)) for _ in range(64)]
                for a, b in pairs:
                    memory = bytearray([0xa5] * guest_lab.BUFFER_SIZE)
                    memory[:32] = a.to_bytes(32, 'little')
                    memory[32:64] = b.to_bytes(32, 'little')
                    gpr = [0] * 16
                    gpr[6] = guest_lab.BUFFER_MARK
                    gpr[15] = guest_lab.SCRATCH_MARK
                    records.append((root - guest_lab.CODE, gpr, bytes(256), guest_lab.ALL_FLAGS | 2, bytes(memory)))
                    memory[128:160] = memory[:32]
                    memory[160:192] = memory[32:64]
                    flags = (0x40 if a & b & mask == 0 else 0) | (1 if (~a & b & mask) == 0 else 0)
                    expected.append((bytes(memory), flags))
            source, output = Path(directory) / 'input.bin', Path(directory) / 'output.bin'
            lab.write_inputs(records, source)
            guest_lab.run([str(binary), str(source), str(output)])
            for actual, (memory, flags) in zip(lab.read_outputs(output, len(records)), expected):
                self.assertEqual(actual[3], memory)
                self.assertEqual(actual[2] & guest_lab.ALL_FLAGS, flags)

    def test_vex_unordered_comparison_flags_and_vectors(self):
        cases, kinds = [], []
        for bits, suffix, size in ((32, 'ss', 'dword'), (64, 'sd', 'qword')):
            for memory in (False, True):
                source = f'{size} ptr [rsi + 32]' if memory else 'xmm1'
                cases.append({'asm': ['vmovups ymm0, ymmword ptr [rsi]',
                                      'vmovups ymm1, ymmword ptr [rsi + 32]',
                                      f'vucomi{suffix} xmm0, {source}',
                                      'vmovups ymmword ptr [rsi + 128], ymm0',
                                      'vmovups ymmword ptr [rsi + 160], ymm1']})
                kinds.append(bits)
        with guest_lab.lab_directory() as directory:
            lab = guest_lab.Lab(directory)
            binary, roots = lab.lifted_binary(cases)
            records, expected = [], []
            values = [-math.inf, -12.5, -0.0, 0.0, 0.125, 17.5, math.inf, math.nan]
            for root, bits in zip(roots, kinds):
                fmt = '<f' if bits == 32 else '<d'
                for left in values:
                    for right in values:
                        memory = bytearray([0xa7] * guest_lab.BUFFER_SIZE)
                        memory[:bits // 8] = struct.pack(fmt, left)
                        memory[32:32 + bits // 8] = struct.pack(fmt, right)
                        gpr = [0] * 16
                        gpr[6] = guest_lab.BUFFER_MARK
                        gpr[15] = guest_lab.SCRATCH_MARK
                        records.append((root - guest_lab.CODE, gpr, bytes(256), guest_lab.ALL_FLAGS | 2, bytes(memory)))
                        memory[128:160] = memory[:32]
                        memory[160:192] = memory[32:64]
                        flags = 0x45 if math.isnan(left) or math.isnan(right) else 0x40 if left == right else 1 if left < right else 0
                        expected.append((bytes(memory), flags))
            source, output = Path(directory) / 'input.bin', Path(directory) / 'output.bin'
            lab.write_inputs(records, source)
            guest_lab.run([str(binary), str(source), str(output)])
            for actual, (memory, flags) in zip(lab.read_outputs(output, len(records)), expected):
                self.assertEqual(actual[3], memory)
                self.assertEqual(actual[2] & guest_lab.ALL_FLAGS, flags)

    def test_vex_tests_define_flags_after_a_call(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            source = """.globl _start
_start:
    and rsp, -16
    call helper
    vpxor ymm0, ymm0, ymm0
    vptest ymm0, ymm0
    jne fail
    jnc fail
    call helper
    vucomisd xmm0, xmm0
    jne fail
    jc fail
    xor edi, edi
    call qword ptr [rip + exit_slot]
fail:
    mov edi, 81
    call qword ptr [rip + exit_slot]
helper:
    xor eax, eax
    ret
.p2align 3
exit_slot: .quad 0
"""
            code, labels = guest_lab.assemble_program(source, directory)
            raw = guest_lab.program_image(code, labels, [('exit', 'exit_slot')])
            binary, _ = guest_lab.build_program(raw, directory)
            result = subprocess.run([str(binary)], capture_output=True, timeout=10)
            self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b'', b''))


if __name__ == '__main__':
    unittest.main()
