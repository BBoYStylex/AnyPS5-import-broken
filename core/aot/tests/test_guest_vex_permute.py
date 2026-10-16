import unittest

import test_guest_vex_integer as integer


class VexPermuteTests(unittest.TestCase):
    execute = integer.VexIntegerTests.execute

    def test_byte_shuffle_and_signed_saturating_packs(self):
        cases, models = [], []
        for operation in ('vpshufb', 'vpacksswb', 'vpackssdw', 'vpackuswb', 'vpackusdw'):
            for width in (128, 256):
                register = 'xmm' if width == 128 else 'ymm'
                for memory in (False, True):
                    for alias in (0, 1):
                        right = ('xmmword' if width == 128 else 'ymmword') + ' ptr [rsi + 32]' if memory else register + '1'
                        cases.append({'asm': ['vmovups ymm0, ymmword ptr [rsi]', 'vmovups ymm1, ymmword ptr [rsi + 32]',
                                              f'{operation} {register}{alias}, {register}0, {right}',
                                              'vmovups ymmword ptr [rsi + 128], ymm0', 'vmovups ymmword ptr [rsi + 160], ymm1']})
                        def model(data, a, b, scalar, operation=operation, width=width, alias=alias):
                            left, right = a.to_bytes(32, 'little'), b.to_bytes(32, 'little')
                            result = bytearray()
                            for block in range(width // 128):
                                if operation == 'vpshufb':
                                    result.extend(0 if c & 128 else left[block * 16 + (c & 15)] for c in right[block * 16:block * 16 + 16])
                                else:
                                    size = 2 if operation.endswith('wb') else 4
                                    target_bits = size * 4
                                    low, high = (-(1 << (target_bits - 1)), (1 << (target_bits - 1)) - 1) if operation.startswith('vpackss') else (0, (1 << target_bits) - 1)
                                    for source in (left, right):
                                        for index in range(block * 16, block * 16 + 16, size):
                                            value = int.from_bytes(source[index:index + size], 'little', signed=True)
                                            value = max(low, min(high, value)) & ((1 << target_bits) - 1)
                                            result.extend(value.to_bytes(size // 2, 'little'))
                            result.extend(bytes(32 - len(result)))
                            data[128:160] = result if alias == 0 else left
                            data[160:192] = result if alias == 1 else right
                        models.append(model)
        self.execute(cases, models)

    def test_shifts_use_low_64_bit_count_and_saturate(self):
        cases, models = [], []
        for operation in ('vpsllw', 'vpslld', 'vpsllq', 'vpsrlw', 'vpsrld', 'vpsrlq', 'vpsraw', 'vpsrad'):
            bits = {'w': 16, 'd': 32, 'q': 64}[operation[-1]]
            for width in (128, 256):
                register = 'xmm' if width == 128 else 'ymm'
                for count in (0, bits - 1, bits, 255, 'register', 'memory'):
                    right = 'xmm1' if count == 'register' else 'xmmword ptr [rsi + 32]' if count == 'memory' else str(count)
                    cases.append({'asm': ['vmovups ymm0, ymmword ptr [rsi]', 'vmovups ymm1, ymmword ptr [rsi + 32]',
                                          f'{operation} {register}1, {register}0, {right}', 'vmovups ymmword ptr [rsi + 128], ymm1',
                                          'vmovups ymmword ptr [rsi + 160], ymm0']})
                    def model(data, a, b, scalar, operation=operation, bits=bits, width=width, count=count):
                        amount = count if isinstance(count, int) else b & ((1 << 64) - 1)
                        result, mask = 0, (1 << bits) - 1
                        for shift in range(0, width, bits):
                            value = (a >> shift) & mask
                            if operation.startswith('vpsra'):
                                value = ((value ^ (1 << (bits - 1))) - (1 << (bits - 1))) >> min(amount, bits - 1)
                            elif amount >= bits:
                                value = 0
                            elif operation.startswith('vpsll'):
                                value <<= amount
                            else:
                                value >>= amount
                            result |= (value & mask) << shift
                        data[128:160], data[160:192] = result.to_bytes(32, 'little'), a.to_bytes(32, 'little')
                    models.append(model)
        self.execute(cases, models)

    def test_cross_lane_permutations_and_128_bit_transfers(self):
        cases, models = [], []
        for operation in ('vpermq', 'vextracti128', 'vextractf128', 'vinserti128', 'vinsertf128'):
            for immediate in (0, 1, 27, 255):
                for memory in (False, True):
                    source = 'ymmword ptr [rsi + 32]' if memory else 'ymm1'
                    if operation == 'vpermq':
                        body = f'vpermq ymm0, {source}, {immediate}'
                    elif operation.startswith('vextract'):
                        body = f'{operation} ' + ('xmmword ptr [rsi + 193]' if memory else 'xmm0') + f', ymm0, {immediate}'
                    else:
                        body = f'{operation} ymm0, ymm0, ' + ('xmmword ptr [rsi + 32]' if memory else 'xmm1') + f', {immediate}'
                    cases.append({'asm': ['vmovups ymm0, ymmword ptr [rsi]', 'vmovups ymm1, ymmword ptr [rsi + 32]', body,
                                          'vmovups ymmword ptr [rsi + 128], ymm0', 'vmovups ymmword ptr [rsi + 160], ymm1']})
                    def model(data, a, b, scalar, operation=operation, immediate=immediate, memory=memory):
                        if operation == 'vpermq':
                            result = sum(((b >> (64 * ((immediate >> (2 * n)) & 3))) & ((1 << 64) - 1)) << (n * 64) for n in range(4))
                        elif operation.startswith('vextract'):
                            result = (a >> ((immediate & 1) * 128)) & ((1 << 128) - 1)
                            if memory:
                                data[193:209] = result.to_bytes(16, 'little')
                                result = a
                        else:
                            offset = (immediate & 1) * 128
                            result = (a & ~(((1 << 128) - 1) << offset)) | ((b & ((1 << 128) - 1)) << offset)
                        data[128:160], data[160:192] = result.to_bytes(32, 'little'), b.to_bytes(32, 'little')
                    models.append(model)
        self.execute(cases, models)


if __name__ == '__main__':
    unittest.main()
