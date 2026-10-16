import unittest

import guest_lab


class VexFloatTests(unittest.TestCase):
    def compare(self, cases, samples=64):
        with guest_lab.lab_directory() as directory:
            failures = guest_lab.Lab(directory).compare(cases, samples=samples, seed=7194)
            self.assertEqual(failures, [], str(failures[:4]))

    def binary(self, suffix):
        cases = []
        for operation in ('add', 'sub', 'mul', 'div', 'min', 'max', 'cmpeq', 'cmplt', 'cmpnle'):
            for alias in (0, 1, 2):
                cases.append({'asm': [f'vinserti128 ymm{alias}, ymm{alias}, xmm7, 1',
                                      f'v{operation}{suffix} xmm{alias}, xmm0, xmm1',
                                      f'vmovups ymmword ptr [rsi], ymm{alias}'],
                              'pointers': {'rsi': 0}, 'vectors': 'double' if suffix[-1] == 'd' else 'float'})
        self.compare(cases)

    def test_scalar_double_aliases_nan_and_upper_zeroing(self):
        self.binary('sd')

    def test_scalar_float_aliases_nan_and_upper_zeroing(self):
        self.binary('ss')

    def test_packed_double(self):
        self.binary('pd')

    def test_packed_float(self):
        self.binary('ps')

    def test_scalar_conversions(self):
        cases = []
        for suffix in ('sd', 'ss'):
            for source in ('rax', 'eax'):
                for alias in (0, 1):
                    cases.append({'asm': [f'vcvtsi2{suffix} xmm{alias}, xmm0, {source}',
                                          f'vmovups ymmword ptr [rsi], ymm{alias}'], 'pointers': {'rsi': 0}})
            for destination in ('rax', 'eax'):
                cases.append({'asm': [f'vcvtt{suffix}2si {destination}, xmm0'],
                              'vectors': 'double' if suffix == 'sd' else 'float'})
        for source, target in (('ss', 'sd'), ('sd', 'ss')):
            for alias in (0, 1, 2):
                cases.append({'asm': [f'vcvt{source}2{target} xmm{alias}, xmm0, xmm1',
                                      f'vmovups ymmword ptr [rsi], ymm{alias}'], 'pointers': {'rsi': 0},
                              'vectors': 'double' if source == 'sd' else 'float'})
        self.compare(cases, 128)

    def test_packed_conversions(self):
        cases = []
        for operation in ('vcvtpd2ps', 'vcvtdq2pd', 'vcvtdq2ps'):
            for alias in (0, 1):
                cases.append({'asm': [f'{operation} xmm{alias}, xmm0', f'vmovups ymmword ptr [rsi], ymm{alias}'],
                              'pointers': {'rsi': 0}, 'vectors': 'double' if operation == 'vcvtpd2ps' else 'bits'})
        self.compare(cases, 128)


    def test_256bit_arithmetic_and_conversions(self):
        cases = []
        for suffix in ('ps', 'pd'):
            for operation in ('add', 'sub', 'mul', 'div', 'min', 'max'):
                cases.append({'asm': ['vinserti128 ymm0, ymm0, xmm2, 1', 'vinserti128 ymm1, ymm1, xmm3, 1',
                                      f'v{operation}{suffix} ymm1, ymm0, ymm1', 'vmovups ymmword ptr [rsi], ymm1'],
                              'pointers': {'rsi': 0}, 'vectors': 'double' if suffix == 'pd' else 'float'})
        for operation, source, destination in [('vcvtpd2ps', 'ymm0', 'xmm0'), ('vcvtdq2pd', 'xmm0', 'ymm0'), ('vcvtdq2ps', 'ymm0', 'ymm0')]:
            cases.append({'asm': ['vinserti128 ymm0, ymm0, xmm2, 1', f'{operation} {destination}, {source}',
                                  'vmovups ymmword ptr [rsi], ymm0'], 'pointers': {'rsi': 0},
                          'vectors': 'double' if operation == 'vcvtpd2ps' else 'bits'})
        self.compare(cases, 128)


if __name__ == '__main__':
    unittest.main()
