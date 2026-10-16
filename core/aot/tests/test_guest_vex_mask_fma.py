import struct
import subprocess
import unittest
from pathlib import Path
from unittest.mock import patch

import guest_lab
import test_guest_vex_integer as integer


class FiniteLab(guest_lab.Lab):
    def vector(self, generator, case):
        bits = 64 if case.get('vectors') == 'double' else 32
        values = [generator.getrandbits(bits) for _ in range(128 // bits)]
        exponent = 0x7ff0000000000000 if bits == 64 else 0x7f800000
        values = [value & ~exponent if value & exponent == exponent else value for value in values]
        return b''.join(value.to_bytes(bits // 8, 'little') for value in values)


class VexMaskFmaTests(unittest.TestCase):
    execute = integer.VexIntegerTests.execute

    def test_fused_finite_operands_round_once_with_aliases(self):
        cases = []
        for suffix in ('ss', 'sd'):
            for operation in ('vfmadd', 'vfnmadd'):
                for order in ('132', '213', '231'):
                    for right in ('xmm2', 'xmm0'):
                        cases.append({'asm': [f'{operation}{order}{suffix} xmm0, xmm1, {right}',
                                              'vmovups ymmword ptr [rsi], ymm0'], 'pointers': {'rsi': 0},
                                      'vectors': 'double' if suffix == 'sd' else 'float'})
        with guest_lab.lab_directory() as directory:
            failures = FiniteLab(directory).compare(cases, samples=128, seed=947)
            self.assertEqual(failures, [], str(failures[:4]))

    def test_nonfinite_fma_is_rejected_at_execution(self):
        with guest_lab.lab_directory() as directory:
            lab = guest_lab.Lab(directory)
            binary, roots = lab.lifted_binary([{'asm': ['vfmadd213sd xmm0, xmm1, xmm2']}])
            for value in (float('inf'), float('-inf'), float('nan')):
                gpr = [0] * 16
                gpr[15] = guest_lab.SCRATCH_MARK
                source = Path(directory) / 'input.bin'
                lab.write_inputs([(roots[0] - guest_lab.CODE, gpr, struct.pack('<d', value) + bytes(248), 2, bytes(512))], source)
                result = subprocess.run([str(binary), str(source), str(Path(directory) / 'out.bin')], capture_output=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(b'unsupported', result.stderr.lower())

    def test_masked_memory_does_not_touch_disabled_guard_page(self):
        cases, models = [], []
        for width in (128, 256):
            reg = 'xmm' if width == 128 else 'ymm'
            for bits in (32, 64):
                operation = 'vpmaskmovd' if bits == 32 else 'vpmaskmovq'
                for store in (False, True):
                    for empty in (False, True):
                        prefix = ['vpxor xmm0, xmm0, xmm0']
                        if not empty:
                            prefix += ['mov eax, -1', 'vmovd xmm0, eax'] if bits == 32 else ['mov rax, -1', 'vmovq xmm0, rax']
                        offset = guest_lab.BUFFER_SIZE if empty else guest_lab.BUFFER_SIZE - bits // 8
                        instruction = f'{operation} {reg}0, {reg}0, [{"rsi"} + {offset}]'
                        if store:
                            prefix += ['vmovups ymm1, ymmword ptr [rsi]']
                            instruction = f'{operation} [rsi + {offset}], {reg}0, {reg}1'
                        cases.append({'asm': prefix + [instruction, 'vmovups ymmword ptr [rsi + 128], ymm0']})
                        def model(data, a, b, scalar, bits=bits, store=store, empty=empty):
                            result = 0 if empty else (1 << bits) - 1 if store else int.from_bytes(bytes([0xa5]) * (bits // 8), 'little')
                            data[128:160] = result.to_bytes(32, 'little')
                            if store and not empty:
                                data[guest_lab.BUFFER_SIZE - bits // 8:] = (a & ((1 << bits) - 1)).to_bytes(bits // 8, 'little')
                        models.append(model)
        allocation = '''static std::uint8_t* buffer = [] {
        const auto page = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
        auto* mapping = static_cast<std::uint8_t*>(::mmap(nullptr, page * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0));
        if (mapping == MAP_FAILED || ::mprotect(mapping + page, page, PROT_NONE) != 0) std::abort();
        return mapping + page - %(buffer)d;
    }();'''
        harness = '#include <sys/mman.h>\n#include <unistd.h>\n' + guest_lab.HARNESS.replace('alignas(64) static std::uint8_t buffer[%(buffer)d + 64];', allocation)
        with patch.object(guest_lab, 'HARNESS', harness):
            self.execute(cases, models)

    def test_vtest_sign_bits_and_flags(self):
        cases = []
        for width in (128, 256):
            reg = 'xmm' if width == 128 else 'ymm'
            for suffix in ('ps', 'pd'):
                cases.append({'asm': ['vinserti128 ymm0, ymm0, xmm4, 1', 'vinserti128 ymm1, ymm1, xmm5, 1',
                                      f'vtest{suffix} {reg}0, {reg}1'], 'flags': 0x41})
        with guest_lab.lab_directory() as directory:
            failures = guest_lab.Lab(directory).compare(cases, samples=128, seed=97)
            self.assertEqual(failures, [], str(failures[:4]))


    def test_vtest_all_flags_against_integer_specification(self):
        cases = [{'asm': ['vinserti128 ymm0, ymm0, xmm4, 1', 'vinserti128 ymm1, ymm1, xmm5, 1',
                          f'vtest{suffix} {reg}0, {reg}1']}
                 for reg in ('xmm', 'ymm') for suffix in ('ps', 'pd')]
        with guest_lab.lab_directory() as directory:
            lab = guest_lab.Lab(directory)
            binary, roots = lab.lifted_binary(cases)
            records = lab.inputs(cases, roots, 128, 97)
            source, output = Path(directory) / 'inputs.bin', Path(directory) / 'outputs.bin'
            lab.write_inputs(records, source)
            guest_lab.run([str(binary), str(source), str(output)])
            for index, (record, result) in enumerate(zip(records, lab.read_outputs(output, len(records)))):
                width = 128 if index // 128 < 2 else 256
                bits = 32 if (index // 128) % 2 == 0 else 64
                xmm = record[2]
                a = int.from_bytes(xmm[0:16] + (xmm[64:80] if width == 256 else b''), 'little')
                b = int.from_bytes(xmm[16:32] + (xmm[80:96] if width == 256 else b''), 'little')
                signs = sum(1 << (lane * bits + bits - 1) for lane in range(width // bits))
                expected = (64 if a & b & signs == 0 else 0) | (1 if ~a & b & signs == 0 else 0)
                self.assertEqual(result[2] & guest_lab.ALL_FLAGS, expected)


if __name__ == '__main__':
    unittest.main()
