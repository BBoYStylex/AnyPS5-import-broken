import random
import unittest
from pathlib import Path

import guest_lab


class VexIntegerTests(unittest.TestCase):
    def execute(self, cases, models):
        with guest_lab.lab_directory() as directory:
            lab = guest_lab.Lab(directory, memory_epochs=getattr(self, 'memory_epochs', False))
            binary, roots = lab.lifted_binary(cases)
            rng = random.Random(72191)
            records, expected = [], []
            for root, model in zip(roots, models):
                for sample in range(16):
                    data = bytearray([0xa5] * guest_lab.BUFFER_SIZE)
                    a = rng.getrandbits(256) if sample > 3 else (0, (1 << 256) - 1, int.from_bytes(b'\x80' * 32, 'little'), 1)[sample]
                    b = rng.getrandbits(256) if sample > 3 else a
                    if sample in (4, 5):
                        width = 16 if sample == 4 else 32
                        values = [0, 1, 127, 128, 255, 256, 32767, 32768, 65535, 65536, (1 << (width - 1)) - 1, 1 << (width - 1)]
                        a = sum((values[n % len(values)] & ((1 << width) - 1)) << (width * n) for n in range(256 // width))
                        b = sum((values[-1 - n % len(values)] & ((1 << width) - 1)) << (width * n) for n in range(256 // width))
                    data[:32] = a.to_bytes(32, 'little')
                    data[32:64] = b.to_bytes(32, 'little')
                    gpr = [0] * 16
                    gpr[0] = 0xfedcba9876543210
                    gpr[6], gpr[15] = guest_lab.BUFFER_MARK, guest_lab.SCRATCH_MARK
                    records.append((root - guest_lab.CODE, gpr, bytes(256), guest_lab.ALL_FLAGS | 2, bytes(data)))
                    model(data, a, b, gpr[0])
                    expected.append(bytes(data))
            source, output = Path(directory) / 'inputs.bin', Path(directory) / 'outputs.bin'
            lab.write_inputs(records, source)
            guest_lab.run([str(binary), str(source), str(output)])
            for index, (actual, wanted) in enumerate(zip(lab.read_outputs(output, len(records)), expected)):
                self.assertEqual(actual[3], wanted, f'case {index // 16}, sample {index % 16}')
                self.assertEqual(actual[2] & guest_lab.ALL_FLAGS, guest_lab.ALL_FLAGS)

    def binary_cases(self, operations):
        cases, models = [], []
        for operation in operations:
            bits = {'b': 8, 'w': 16, 'd': 32, 'q': 64}[operation[-1]]
            for width in (128, 256):
                register = 'xmm' if width == 128 else 'ymm'
                for memory in (False, True):
                    for alias in (0, 1):
                        right = ('xmmword' if width == 128 else 'ymmword') + ' ptr [rsi + 32]' if memory else register + '1'
                        cases.append({'asm': ['vmovups ymm0, ymmword ptr [rsi]', 'vmovups ymm1, ymmword ptr [rsi + 32]',
                                              f'{operation} {register}{alias}, {register}0, {right}',
                                              'vmovups ymmword ptr [rsi + 128], ymm0', 'vmovups ymmword ptr [rsi + 160], ymm1']})
                        def model(data, a, b, scalar, operation=operation, bits=bits, width=width, alias=alias):
                            mask = (1 << bits) - 1
                            result = 0
                            for shift in range(0, width, bits):
                                x, y = (a >> shift) & mask, (b >> shift) & mask
                                sx, sy = (x ^ (1 << (bits - 1))) - (1 << (bits - 1)), (y ^ (1 << (bits - 1))) - (1 << (bits - 1))
                                if operation.startswith('vpadd'):
                                    value = x + y
                                elif operation.startswith('vpsub'):
                                    value = x - y
                                elif operation.startswith('vpmull'):
                                    value = x * y
                                elif operation.startswith('vpcmpeq'):
                                    value = mask if x == y else 0
                                elif operation.startswith('vpcmpgt'):
                                    value = mask if sx > sy else 0
                                else:
                                    left, right = (sx, sy) if operation[-2] == 's' else (x, y)
                                    value = min(left, right) if operation.startswith('vpmin') else max(left, right)
                                result |= (value & mask) << shift
                            data[128:160] = (result if alias == 0 else a).to_bytes(32, 'little')
                            data[160:192] = (result if alias == 1 else b).to_bytes(32, 'little')
                        models.append(model)
        self.execute(cases, models)

    def test_add_subtract_and_multiply(self):
        self.binary_cases(['vpadd' + x for x in 'bwdq'] + ['vpsub' + x for x in 'bwdq'] + ['vpmullw', 'vpmulld'])

    def test_equal_and_signed_greater(self):
        self.binary_cases(['vpcmpeq' + x for x in 'bwdq'] + ['vpcmpgt' + x for x in 'bwdq'])

    def test_signed_unsigned_extrema(self):
        self.binary_cases([prefix + sign + size for prefix in ('vpmin', 'vpmax') for sign in 'su' for size in 'bwd'])

    def test_epoch_state_preserves_aliases_and_upper_lanes(self):
        self.memory_epochs = True
        self.binary_cases(['vpaddd', 'vpcmpgtq', 'vpminsd'])

    def test_evex_masks_are_rejected(self):
        for instruction in ('vpaddd xmm0 {k1}, xmm1, xmm2', 'vpaddd ymm0 {k1}{z}, ymm1, ymm2',
                            'vmovups xmm0 {k1}, xmm1', 'vmovups ymm0 {k1}{z}, ymm1'):
            with self.subTest(instruction=instruction), guest_lab.lab_directory() as directory:
                with self.assertRaisesRegex(ValueError, 'only unmasked VEX encodings'):
                    guest_lab.Lab(directory).lifted_binary([{'asm': [instruction]}])

    def test_extract_insert_masks_aliases_and_narrow_stores(self):
        cases, models = [], []
        for suffix, bits in [('b', 8), ('w', 16), ('d', 32), ('q', 64)]:
            for immediate in (0, 128 // bits - 1, 255):
                for memory in (False, True):
                    offset = (immediate & (128 // bits - 1)) * bits
                    size = {8: 'byte', 16: 'word', 32: 'dword', 64: 'qword'}[bits]
                    destination = f'{size} ptr [rsi + 193]' if memory else ('rax' if bits == 64 else 'eax')
                    middle = [f'vpextr{suffix} {destination}, xmm0, {immediate}']
                    if not memory:
                        middle.append('mov qword ptr [rsi + 193], rax')
                    cases.append({'asm': ['vmovups ymm0, ymmword ptr [rsi]', *middle, 'vmovups ymmword ptr [rsi + 128], ymm0']})
                    def extraction(data, a, b, scalar, offset=offset, bits=bits, memory=memory):
                        value = (a >> offset) & ((1 << bits) - 1)
                        length = bits // 8 if memory else 8
                        data[193:193 + length] = value.to_bytes(length, 'little')
                        data[128:160] = a.to_bytes(32, 'little')
                    models.append(extraction)
                    for alias in (0, 1):
                        source = f'{size} ptr [rsi + 32]' if memory else ('rax' if bits == 64 else 'eax')
                        cases.append({'asm': ['vmovups ymm0, ymmword ptr [rsi]', 'vmovups ymm1, ymmword ptr [rsi + 32]',
                                              f'vpinsr{suffix} xmm{alias}, xmm0, {source}, {immediate}',
                                              'vmovups ymmword ptr [rsi + 128], ymm0', 'vmovups ymmword ptr [rsi + 160], ymm1']})
                        def insertion(data, a, b, scalar, offset=offset, bits=bits, memory=memory, alias=alias):
                            mask = (1 << bits) - 1
                            value = (b if memory else scalar) & mask
                            result = ((a & ((1 << 128) - 1)) & ~(mask << offset)) | (value << offset)
                            data[128:160] = (result if alias == 0 else a).to_bytes(32, 'little')
                            data[160:192] = (result if alias == 1 else b).to_bytes(32, 'little')
                        models.append(insertion)
        self.execute(cases, models)


if __name__ == '__main__':
    unittest.main()
