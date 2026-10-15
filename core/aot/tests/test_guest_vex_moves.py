import unittest
from unittest.mock import patch

import guest_lab

import test_guest_vex_integer as integer


class VexMoveTests(unittest.TestCase):
    execute = integer.VexIntegerTests.execute

    def test_narrow_memory_sources_at_guard_page(self):
        cases, models = [], []
        for operation, width, count in [('vpmovzxbq', 128, 2), ('vpmovsxbd', 128, 4),
                                        ('vpmovzxbw', 256, 16), ('vpbroadcastb', 128, 1),
                                        ('vpbroadcastw', 256, 2), ('vpbroadcastq', 256, 8),
                                        ('vmovss', 128, 4), ('vmovsd', 128, 8)]:
            register = 'xmm' if width == 128 else 'ymm'
            size = {1: 'byte', 2: 'word', 4: 'dword', 8: 'qword', 16: 'xmmword'}[count]
            cases.append({'asm': [f'{operation} {register}0, {size} ptr [rsi + {guest_lab.BUFFER_SIZE - count}]',
                                  'vmovups ymmword ptr [rsi + 128], ymm0']})
            def model(data, a, b, scalar, operation=operation, width=width, count=count):
                if operation.startswith('vpmov'):
                    dstbits = {'q': 64, 'd': 32, 'w': 16}[operation[-1]]
                    value = 0xa5 - 256 if operation.startswith('vpmovsx') else 0xa5
                    result = sum((value & ((1 << dstbits) - 1)) << i for i in range(0, width, dstbits))
                elif operation.startswith('vpbroadcast'):
                    result = int.from_bytes(bytes([0xa5]) * (width // 8), 'little')
                else:
                    result = int.from_bytes(bytes([0xa5]) * count, 'little')
                data[128:160] = result.to_bytes(32, 'little')
            models.append(model)
        allocation = '''static std::uint8_t* buffer = [] {
        const auto page = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
        auto* mapping = static_cast<std::uint8_t*>(::mmap(nullptr, page * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0));
        if (mapping == MAP_FAILED || ::mprotect(mapping + page, page, PROT_NONE) != 0) std::abort();
        return mapping + page - %(buffer)d;
    }();'''
        harness = '#include <sys/mman.h>\n#include <unistd.h>\n' + guest_lab.HARNESS.replace('alignas(64) static std::uint8_t buffer[%(buffer)d + 64];', allocation)
        self.assertIn(allocation, harness)
        with patch.object(guest_lab, 'HARNESS', harness):
            self.execute(cases, models)

    def test_scalar_moves_preserve_merge_source_and_store_width(self):
        cases, models = [], []
        for bits, operation in ((32, 'vmovss'), (64, 'vmovsd')):
            for form in ('load', 'store', 'merge-left', 'merge-right'):
                middle = {'load': f'{operation} xmm0, ' + ('dword' if bits == 32 else 'qword') + ' ptr [rsi + 33]',
                          'store': f'{operation} ' + ('dword' if bits == 32 else 'qword') + ' ptr [rsi + 193], xmm0',
                          'merge-left': f'{operation} xmm0, xmm0, xmm1',
                          'merge-right': f'{operation} xmm1, xmm0, xmm1'}[form]
                cases.append({'asm': ['vmovups ymm0, ymmword ptr [rsi]', 'vmovups ymm1, ymmword ptr [rsi + 32]', middle,
                                      'vmovups ymmword ptr [rsi + 128], ymm0', 'vmovups ymmword ptr [rsi + 160], ymm1']})
                def model(data, a, b, scalar, bits=bits, form=form):
                    mask = (1 << bits) - 1
                    left, right = a, b
                    if form == 'load':
                        left = (b >> 8) & mask
                    elif form == 'store':
                        data[193:193 + bits // 8] = (a & mask).to_bytes(bits // 8, 'little')
                    else:
                        result = ((a & ((1 << 128) - 1)) & ~mask) | (b & mask)
                        if form == 'merge-left':
                            left = result
                        else:
                            right = result
                    data[128:160], data[160:192] = left.to_bytes(32, 'little'), right.to_bytes(32, 'little')
                models.append(model)
        self.execute(cases, models)

    def test_broadcast_widths_and_upper_zeroing(self):
        cases, models = [], []
        for operation, bits in [('vpbroadcastb', 8), ('vpbroadcastw', 16), ('vpbroadcastq', 64), ('vbroadcastss', 32), ('vbroadcastsd', 64), ('vbroadcasti128', 128)]:
            for width in (128, 256):
                if width == 128 and operation in ('vbroadcastsd', 'vbroadcasti128'):
                    continue
                for memory in (False, True):
                    if not memory and operation == 'vbroadcasti128':
                        continue
                    source = {8: 'byte', 16: 'word', 32: 'dword', 64: 'qword', 128: 'xmmword'}[bits] + ' ptr [rsi + 33]' if memory else 'xmm0'
                    register = 'xmm' if width == 128 else 'ymm'
                    cases.append({'asm': ['vmovups ymm0, ymmword ptr [rsi]', f'{operation} {register}0, {source}',
                                          'vmovups ymmword ptr [rsi + 128], ymm0']})
                    def model(data, a, b, scalar, bits=bits, width=width, memory=memory):
                        value = ((b >> 8) if memory else a) & ((1 << bits) - 1)
                        result = sum(value << n for n in range(0, width, bits))
                        data[128:160] = result.to_bytes(32, 'little')
                    models.append(model)
        self.execute(cases, models)

    def test_widening_uses_only_declared_source_bytes(self):
        cases, models = [], []
        for signed in (False, True):
            for suffix, srcbits, dstbits in [('bw', 8, 16), ('bd', 8, 32), ('bq', 8, 64), ('wd', 16, 32), ('wq', 16, 64), ('dq', 32, 64)]:
                operation = ('vpmovsx' if signed else 'vpmovzx') + suffix
                for width in (128, 256):
                    nbytes = width // dstbits * srcbits // 8
                    for memory in (False, True):
                        source = {2: 'word', 4: 'dword', 8: 'qword', 16: 'xmmword'}[nbytes] + ' ptr [rsi + 33]' if memory else 'xmm0'
                        register = 'xmm' if width == 128 else 'ymm'
                        cases.append({'asm': ['vmovups ymm0, ymmword ptr [rsi]', f'{operation} {register}0, {source}',
                                              'vmovups ymmword ptr [rsi + 128], ymm0']})
                        def model(data, a, b, scalar, signed=signed, srcbits=srcbits, dstbits=dstbits, width=width, memory=memory):
                            value = b >> 8 if memory else a
                            result = 0
                            for lane in range(width // dstbits):
                                part = (value >> (lane * srcbits)) & ((1 << srcbits) - 1)
                                if signed and part & (1 << (srcbits - 1)):
                                    part -= 1 << srcbits
                                result |= (part & ((1 << dstbits) - 1)) << (lane * dstbits)
                            data[128:160] = result.to_bytes(32, 'little')
                        models.append(model)
        self.execute(cases, models)


if __name__ == '__main__':
    unittest.main()
