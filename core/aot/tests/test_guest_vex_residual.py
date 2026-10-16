import unittest

import guest_lab


class VexResidualTests(unittest.TestCase):
    def compare(self, templates, widths=(128, 256)):
        cases = []
        for width in widths:
            reg = 'xmm' if width == 128 else 'ymm'
            for template in templates:
                for alias in (0, 1, 2):
                    cases.append({'asm': ['vinserti128 ymm0, ymm0, xmm4, 1', 'vinserti128 ymm1, ymm1, xmm5, 1',
                                          'vinserti128 ymm2, ymm2, xmm6, 1',
                                          template.format(d=reg + str(alias), a=reg + '0', b=reg + '1', c=reg + '2'),
                                          'vmovups ymmword ptr [rsi], ymm0', 'vmovups ymmword ptr [rsi + 32], ymm1',
                                          'vmovups ymmword ptr [rsi + 64], ymm2'], 'pointers': {'rsi': 0}})
        with guest_lab.lab_directory() as directory:
            failures = guest_lab.Lab(directory).compare(cases, samples=32, seed=561)
            self.assertEqual(failures, [], str(failures[:4]))

    def test_blends(self):
        self.compare(['vblendvps {d}, {a}, {b}, {c}', 'vblendvpd {d}, {a}, {b}, {c}',
                      'vpblendw {d}, {a}, {b}, 165', 'vpblendd {d}, {a}, {b}, 165'])

    def test_permutations(self):
        self.compare(['vpermilps {d}, {a}, 27', 'vpermilpd {d}, {a}, 6', 'vshufps {d}, {a}, {b}, 27',
                      'vshufpd {d}, {a}, {b}, 9', 'vpunpcklbw {d}, {a}, {b}', 'vpunpcklqdq {d}, {a}, {b}',
                      'vunpcklpd {d}, {a}, {b}', 'vmovshdup {d}, {a}', 'vmovddup {d}, {a}'])

    def test_integer_operations(self):
        self.compare(['vpandn {d}, {a}, {b}', 'vpmuldq {d}, {a}, {b}', 'vpmuludq {d}, {a}, {b}',
                      'vphaddd {d}, {a}, {b}', 'vpavgb {d}, {a}, {b}', 'vpabsd {d}, {a}',
                      'vpsrlvd {d}, {a}, {b}', 'vmovmskps eax, {a}'])

    def test_narrow_transfers(self):
        self.compare(['vinsertps {d}, {a}, {b}, 183', 'vinsertps {d}, {a}, dword ptr [rsi + 33], 247',
                      'vmovlhps {d}, {a}, {b}', 'vmovlps {d}, {a}, qword ptr [rsi + 33]',
                      'vmovhpd {d}, {a}, qword ptr [rsi + 33]', 'vmovlpd qword ptr [rsi + 193], {a}',
                      'vmovhpd qword ptr [rsi + 193], {a}'], widths=(128,))

    def test_128bit_permute_and_zero_control(self):
        self.compare([f'vperm2i128 {{d}}, {{a}}, {{b}}, {imm}' for imm in (0, 3, 49, 128, 136, 255)], widths=(256,))


if __name__ == '__main__':
    unittest.main()
