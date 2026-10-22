import os
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import nid_names

# Every export the SDK's stub tables mark `@object` and that AnyPS5 exports.
# The SDK's tables are ps5-payload-dev/sdk's sce_stubs/*.c at
# a7685ce8e97ce1f8a4f38253a41d54241c9124f1, and the names are listed here
# because the tables are not vendored.
OBJECT_EXPORTS = {
    "libSceAmpr.prx": [
        ("_ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE", "-mLzBSk-VGs"),
        ("_ZTVSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE", "KfcTPbeaOqg"),
        ("_ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE", "a54t8+k7KpY"),
    ],
    "libSceLibcInternal.prx": [
        ("optreset", "CZNm+oNmB-I"),
        ("optopt", "FwzVaZ8Vnus"),
        ("sys_signame", "Y16fu+FC+3Q"),
        ("sys_nsig", "ZD+Dp+-LsGg"),
        ("Need_sceLibcInternal", "ZT4ODD2Ts9o"),
        ("sys_siglist", "yCdGspbNHZ8"),
        ("opterr", "yaFXXViLWPw"),
        ("optind", "zCnSJWp-Qj8"),
        ("optarg", "zG0BNJOZdm4"),
    ],
    "libSceNet.prx": [
        ("sce_net_in6addr_linklocal_allnodes", "84MgU4MMTLQ"),
        ("sce_net_in6addr_any", "GAtITrgxKDE"),
        ("in6addr_loopback", "XCuA-GqjA-k"),
        ("in6addr_any", "ZRAJo-A-ukc"),
    ],
    "libc.prx": [
        ("_Flt", "+SeQg8c1WC0"),
        ("_ZTVSt13runtime_error", "-L+-8F0+gBc"),
        ("timezone", "-Oet9AHzwtU"),
        ("_Stdin", "1TDo-ImqkJc"),
        ("_ZTVSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE", "1kZFcktOm+s"),
        ("_ZSt3cin", "2WVBaSdGIds"),
        ("_ZSt4wcin", "2bASh0rEeXI"),
        ("_LInf", "2h3jY75zMkI"),
        ("_Stdout", "2sWzhYqFH4E"),
        ("_Ldbl", "43u-nm1hQc8"),
        ("_ZTISt12domain_error", "5BIbzIuDxTQ"),
        ("_ZSt4cout", "5PfqUBaQf4g"),
        ("signgam", "5gOOC0kzW0c"),
        ("__mb_cur_max", "77pL1FoD4I4"),
        ("_ZNSt7collateIcE2idE", "7brRfHVVAlI"),
        ("_Nan", "8e2KBTO08Po"),
        ("_ZTISt18bad_variant_access", "8rd5FvOFk+w"),
        ("_ZTISt11regex_error", "9-TRy4p-YTM"),
        ("_ZTVN10__cxxabiv121__vmi_class_type_infoE", "9ByRMdo7ywg"),
        ("_ZTVSt18bad_variant_access", "AZGKZIVok6U"),
        ("_ZTVSt14overflow_error", "Azw9C8cy7FY"),
        ("tzname", "BAYjQubSZT8"),
        ("_ZTISt8ios_base", "BJCgW9-OxLA"),
        ("_ZTVSt12system_error", "Bq8m04PN1zw"),
        ("_ZTISt11range_error", "C0IYaaVSC1w"),
        ("_Denorm", "COSADmn1ROg"),
        ("_ZTVN10__cxxabiv120__function_type_infoE", "CSEjkTYt5dw"),
        ("_ZNSt5ctypeIcE2idE", "Cv+zC4EjGMA"),
        ("_ZSt5wcerr", "CvJ3HUPlMIY"),
        ("_ZTISt9bad_alloc", "DwH3gdbYfZo"),
        ("_ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE", "E14mW8pVpoE"),
        ("_ZTVSt9bad_alloc", "EMNG6cHitlQ"),
        ("_ZSt7_BADOFF", "FQ9NFbBHb5Y"),
        ("_ZNSt7codecvtIwc9_MbstatetE2idE", "FjZCPmK0SbA"),
        ("_ZTISt10bad_typeid", "H0aqk25W6BI"),
        ("_ZNSt6locale2id7_Id_cntE", "H4fcpQOpc08"),
        ("_Stderr", "H8AprKeZtNg"),
        ("_Inf", "HIhqigNaOns"),
        ("_FDenorm", "JG1MkIFKnT8"),
        ("_ZSt7nothrow", "NLwJ3q+64bY"),
        ("_LDenorm", "O7zxyNnSDDA"),
        ("_FInf", "QWCTbYI14dA"),
        ("_ZSt21_sceLibcClassicLocale", "Qoo175Ig+-k"),
        ("_ZTVSt11range_error", "RbzWN8X21hY"),
        ("_ZSt4cerr", "TVfbf1sXt0A"),
        ("_FSnan", "UCjpTas5O3E"),
        ("_FNan", "VVgqI3B2bfk"),
        ("_ZNSt5ctypeIwE2idE", "VmqsS6auJzo"),
        ("_ZTISt16invalid_argument", "XZzWt0ygWdw"),
        ("_ZTISt12bad_weak_ptr", "XtP9KKwyK9Q"),
        ("_ZTVSt20bad_array_new_length", "Z+vcX3rnECg"),
        ("_ZTVSt7codecvtIcc9_MbstatetE", "aK1Ymf-NhAs"),
        ("_Hugeval", "ac102y6Rjjc"),
        ("_ZTVN10__cxxabiv119__pointer_type_infoE", "aeHxLWwq0gQ"),
        ("_ZTVSt12bad_weak_ptr", "apHKv46QaCw"),
        ("__stderrp", "as8Od-tH1BI"),
        ("_ZTISt13runtime_error", "bLPn1gfqSW8"),
        ("__stdinp", "bgAcsbcEznc"),
        ("_ZTVN10__cxxabiv117__class_type_infoE", "byV+FWlAnB4"),
        ("_ZTVSt11regex_error", "c-EfVOIbo8M"),
        ("_ZTISt17bad_function_call", "c33GAGjd7Is"),
        ("_CurrentRuneLocale", "chlN6g6UbGo"),
        ("_ZTVSt12length_error", "cqvea9uWpvQ"),
        ("_ZTISt12length_error", "cxqzgvGm1GI"),
        ("_ZSt5wcout", "d-YRIvO0jXI"),
        ("_ZTVSt9exception", "dCzeFfg9WWI"),
        ("_ZTISt12out_of_range", "dKjhNUf9FBc"),
        ("_Dbl", "e+hi-tOrDZU"),
        ("_ZTISt12system_error", "eDciML+moZs"),
        ("_ZNSt7codecvtIcc9_MbstatetE2idE", "eVFYZnYNDo0"),
        ("__stack_chk_guard", "f7uOxY9mM1U"),
        ("__mb_sb_limit", "fGYLBr2COwA"),
        ("_Snan", "fmHLr9P3dOk"),
        ("_ZNSt7collateIwE2idE", "irGo1yaJ-vM"),
        ("_ZTVSt10bad_typeid", "jRLwj8TLcQY"),
        ("_ZTVSt16invalid_argument", "keXoyW-rV-0"),
        ("_DefaultRuneLocale", "lWGF8NHv880"),
        ("_ZTISt20bad_array_new_length", "lbLEAN+Y9iI"),
        ("_ZTISt14overflow_error", "lt0mLhNwjs0"),
        ("_ZTVSt12out_of_range", "n+aUKkC-3sI"),
        ("_ZTISt9exception", "n2kx+OmFUis"),
        ("_ZTVSt12domain_error", "oAidKrxuUv0"),
        ("_ZTVN10__cxxabiv120__si_class_type_infoE", "pZ9WXcClPO8"),
        ("__isthreaded", "q1OvUam0BJU"),
        ("_ZTISt8bad_cast", "qOD-ksTkE08"),
        ("_LSnan", "rQ-70s51wrc"),
        ("_ZTINSt8ios_base7failureE", "sBCTjFk7Gi4"),
        ("daylight", "tOicWgmk4ZI"),
        ("_ZTVSt8bad_cast", "tVHE+C8vGXk"),
        ("_ZTVSt11logic_error", "udTM6Nxx-Ng"),
        ("_ZSt4_Fpz", "wiR+rIcbnlc"),
        ("_ZTVSt17bad_function_call", "wuOrunkpIrU"),
        ("_ZTISt11logic_error", "x8LHSvl5N6I"),
        ("_ZTVNSt8ios_base7failureE", "yLE5H3058Ao"),
        ("_LNan", "zqASRvZg6d0"),
        ("__stdoutp", "zqJhBxAKfsc"),
    ],
    "libkernel.prx": [
        ("environ", "+2thxYZ4syk"),
        ("__progname", "djxxOmW6-aw"),
    ],
}


class Export:
    def __init__(self, name, rva, kind):
        self.name = name
        self.rva = rva
        self.kind = kind

    def __repr__(self):
        return f"{self.name} at 0x{self.rva:x} ({self.kind})"


def elf_exports(path, data):
    """Every exported symbol of an ELF library, with its address and kind."""
    header = struct.unpack_from("<16sHHIQQQIHHHHHH", data, 0)
    section_offset, section_size, section_count = header[6], header[11], header[12]
    if not section_offset:
        return []
    sections = [struct.unpack_from("<IIQQQQIIQQ", data, section_offset + index * section_size)
                for index in range(section_count)]
    names = []
    for section in sections:
        if section[1] != 11 or section[9] != 24:
            continue
        strings = sections[section[6]]
        for position in range(0, section[5], 24):
            name, info, _, value, _, _ = struct.unpack_from("<IBBHQQ", data, section[4] + position)
            if not name or not value:
                continue
            end = data.find(b"\0", strings[4] + name)
            if end < 0:
                continue
            names.append(Export(data[strings[4] + name:end].decode("utf-8", "replace"), value,
                                "code" if (info & 0xf) == 2 else "data"))
    return names


def pe_sections(data):
    offset = struct.unpack_from("<I", data, 0x3c)[0]
    count = struct.unpack_from("<H", data, offset + 6)[0]
    size_of_optional = struct.unpack_from("<H", data, offset + 20)[0]
    table = offset + 24 + size_of_optional
    sections = []
    for index in range(count):
        entry = table + index * 40
        name = data[entry:entry + 8].rstrip(b"\0").decode("ascii", "replace")
        virtual_size = struct.unpack_from("<I", data, entry + 8)[0]
        virtual_address = struct.unpack_from("<I", data, entry + 12)[0]
        raw_size = struct.unpack_from("<I", data, entry + 16)[0]
        raw_offset = struct.unpack_from("<I", data, entry + 20)[0]
        sections.append((name, max(virtual_size, raw_size), virtual_address, raw_offset))
    return sections


def pe_exports(path, data):
    """Every exported symbol of a PE library, with its RVA and kind."""
    offset = struct.unpack_from("<I", data, 0x3c)[0]
    directory = offset + 24 + 112
    rva, size = struct.unpack_from("<II", data, directory)
    if not rva or size < 40:
        return []
    sections = pe_sections(data)

    def to_offset(target):
        for name, extent, virtual_address, raw_offset in sections:
            if virtual_address <= target < virtual_address + extent:
                return raw_offset + target - virtual_address
        raise AssertionError(f"{path}: the RVA 0x{target:x} lies outside every section")

    table = struct.unpack_from("<IIHHIIIIIII", data, to_offset(rva))
    function_count, name_count = table[6], table[7]
    functions_rva, names_rva, ordinals_rva = table[8], table[9], table[10]
    if not name_count:
        return []
    functions = struct.unpack_from(f"<{function_count}I", data, to_offset(functions_rva))
    names = struct.unpack_from(f"<{name_count}I", data, to_offset(names_rva))
    ordinals = struct.unpack_from(f"<{name_count}H", data, to_offset(ordinals_rva))
    result = []
    for index in range(name_count):
        start = to_offset(names[index])
        end = data.find(b"\0", start)
        name = data[start:end].decode("ascii", "replace")
        address = functions[ordinals[index]]
        kind = "data"
        for section, extent, virtual_address, _ in sections:
            if virtual_address <= address < virtual_address + extent and section == ".text":
                kind = "code"
        result.append(Export(name, address, kind))
    return result


def read_exports(path):
    data = path.read_bytes()
    if data[:4] == b"\x7fELF":
        return elf_exports(path, data)
    if data[:2] == b"MZ":
        return pe_exports(path, data)
    raise AssertionError(f"{path}: neither ELF nor PE")


class DataObjectExportTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(os.environ.get("ANYPS5_SOURCE_ROOT", Path.cwd())).resolve()
        self.patched = self.root / "build" / "core" / "libs" / "libs"

    def exports(self, library):
        path = self.patched / library
        if not path.exists():
            self.skipTest(f"{path} is not built")
        return {export.name: export for export in read_exports(path)}

    def test_every_object_export_resolves_by_its_nid(self):
        for library, entries in OBJECT_EXPORTS.items():
            if not (self.patched / library).exists():
                continue
            exports = self.exports(library)
            for name, nid in entries:
                with self.subTest(library=library, export=name):
                    self.assertIn(nid, exports)

    def test_an_object_export_is_data_and_not_a_function(self):
        for library, entries in OBJECT_EXPORTS.items():
            if not (self.patched / library).exists():
                continue
            exports = self.exports(library)
            for name, nid in entries:
                with self.subTest(library=library, export=name):
                    export = exports.get(nid)
                    self.assertIsNotNone(export, f"{nid} is not exported")
                    self.assertEqual(export.kind, "data",
                                     f"{nid} resolves to {export}, so a consumer reads code where the object is")

    def test_no_export_of_an_object_is_a_function(self):
        """No export of a library may be code when the SDK declares it an object.

        The export may carry the NID or the plain name, so every export is
        matched against the NID of every SDK object.
        """
        nids = {nid for entries in OBJECT_EXPORTS.values() for _, nid in entries}
        for library in OBJECT_EXPORTS:
            if not (self.patched / library).exists():
                continue
            for name, export in self.exports(library).items():
                if name in nids:
                    with self.subTest(library=library, export=name):
                        self.assertEqual(export.kind, "data",
                                         f"{name} resolves to {export}, so a consumer reads code where the object is")

    def test_the_nid_of_every_object_export_is_the_one_the_sdk_names(self):
        for library, entries in OBJECT_EXPORTS.items():
            for name, nid in entries:
                with self.subTest(library=library, export=name):
                    self.assertEqual(nid_names.compute_nid(name), nid)


if __name__ == "__main__":
    unittest.main()
