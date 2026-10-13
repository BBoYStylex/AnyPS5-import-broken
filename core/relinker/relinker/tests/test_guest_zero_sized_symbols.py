from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from test_guest_intel_trampolines import main_fixture, pe_sections
from test_guest_module_directories import module_with_symbol
from test_guest_symbol_names import dynamic_tags, file_offset, symbol_name
from test_macos_output import MachO


FIRST_ADDRESS = 0x1000
TARGETS = (0x2320, 0x2328)


def module(address, size, kind, memory_size=0x400, pointers=True, init=False, target=TARGETS[0]):
    image = module_with_symbol(True)
    struct.pack_into("<Q", image, 120 + 40, memory_size)
    image[0x898 + 4] = 0x10 | kind
    struct.pack_into("<QQ", image, 0x898 + 8, address, size)
    if pointers:
        struct.pack_into("<Q", image, 0x668, 48)
        struct.pack_into("<QQq", image, 0x900, target, (1 << 32) | 1, 0)
        struct.pack_into("<QQq", image, 0x918, TARGETS[1], 8, address)
    if init:
        struct.pack_into("<qQ", image, 0x680, 12, 0x1100)
        struct.pack_into("<qQ", image, 0x690, 0, 0)
        struct.pack_into("<QQ", image, 176 + 32, 160, 160)
    return image


def check_linux(data, address, kind):
    tags = dynamic_tags(data)
    symbols = file_offset(data, tags[6])
    info, _, section, value, size = struct.unpack_from("<BBHQQ", data, symbols + 24 + 4)
    assert (info, section, value, size) == (0x10 | kind, 1, address, 0)
    assert symbol_name(data, tags, 1) == "shared#guest"
    start = file_offset(data, tags[7])
    records = [struct.unpack_from("<QQq", data, offset) for offset in range(start, start + tags[8], 24)]
    assert records == [(TARGETS[0], (1 << 32) | 1, 0), (TARGETS[1], 8, address)], records


def check_windows(data, address):
    sections = pe_sections(data)
    first_rva = next(rva for name, rva, _, _, _ in sections if name == b".elf0")
    expected_rva = first_rva + address - FIRST_ADDRESS

    def offset(rva):
        for _, start, size, raw, _ in sections:
            if start <= rva < start + size:
                return raw + rva - start
        raise AssertionError(f"Unmapped PE RVA: {rva:#x}")

    header, = struct.unpack_from("<I", data, 0x3C)
    base, = struct.unpack_from("<Q", data, header + 24 + 24)
    export_rva, = struct.unpack_from("<I", data, header + 24 + 112)
    directory = offset(export_rva)
    count, functions, names, ordinals = struct.unpack_from("<IIII", data, directory + 24)
    exports = {}
    for index in range(count):
        name_rva, = struct.unpack_from("<I", data, offset(names) + index * 4)
        name_start = offset(name_rva)
        name = data[name_start:data.index(0, name_start)].decode()
        ordinal, = struct.unpack_from("<H", data, offset(ordinals) + index * 2)
        exports[name], = struct.unpack_from("<I", data, offset(functions) + ordinal * 4)
    assert exports["shared"] == expected_rva, exports
    for target in TARGETS:
        actual, = struct.unpack_from("<Q", data, offset(first_rva + target - FIRST_ADDRESS))
        assert actual == base + expected_rva, (target, actual, base + expected_rva)


def uleb(data, position):
    value = 0
    shift = 0
    while True:
        byte = data[position]
        position += 1
        value |= (byte & 0x7F) << shift
        if byte & 0x80 == 0:
            return value, position
        shift += 7
        assert shift < 64


def check_macos(data, address):
    image = MachO(data)
    first = next(segment for segment in image.segments if segment["name"] == "__ELF0")
    expected = first["vmaddr"] + address - FIRST_ADDRESS
    image_base = image.segment("__TEXT")["vmaddr"]
    export_offset, export_size = image.dyld_info[8:10]
    trie = data[export_offset:export_offset + export_size]
    exports = {}

    def visit(position, name):
        size, position = uleb(trie, position)
        children = position + size
        if size:
            flags, position = uleb(trie, position)
            value, _ = uleb(trie, position)
            assert flags == 0
            exports[name] = image_base + value
        count = trie[children]
        position = children + 1
        for _ in range(count):
            end = trie.index(0, position)
            label = trie[position:end].decode()
            child, position = uleb(trie, end + 1)
            visit(child, name + label)

    visit(0, "")
    assert exports["_shared#guest"] == expected, exports
    for target in TARGETS:
        wanted = first["vmaddr"] + target - FIRST_ADDRESS
        segment = next(segment for segment in image.segments
                       if segment["vmaddr"] <= wanted < segment["vmaddr"] + segment["filesize"])
        actual, = struct.unpack_from("<Q", data, segment["fileoff"] + wanted - segment["vmaddr"])
        assert actual == expected, (target, actual, expected)


def main():
    relinker = Path(sys.argv[1]).resolve()
    positive = (
        ("object-end", 0x2400, 1, 0x400),
        ("notype-end", 0x2400, 0, 0x400),
        ("gap-end", 0x1100, 1, 0x400),
        ("page-end-object", 0x3000, 1, 0x1000),
        ("page-end-notype", 0x3000, 0, 0x1000),
        ("interior-unknown", 0x23FF, 1, 0x400),
    )
    negative = (
        ("function-end", module(0x1100, 0, 2, pointers=False)),
        ("unmapped-gap", module(0x1101, 0, 1, pointers=False)),
        ("past-end", module(0x2401, 0, 1, pointers=False)),
        ("end-with-size", module(0x2400, 1, 1, pointers=False)),
        ("crossing-size", module(0x23FF, 2, 1, pointers=False)),
        ("init-end", module(0x23FF, 0, 1, pointers=False, init=True)),
        ("write-end", module(0x23FF, 0, 1, target=0x2400)),
    )
    with tempfile.TemporaryDirectory(prefix="anyps5-zero-sized-symbols-") as directory:
        work = Path(directory)

        def convert(label, system, options, image):
            case = work / system / label
            modules = case / "sce_module"
            modules.mkdir(parents=True)
            source = case / "input.elf"
            original = main_fixture()
            source.write_bytes(original)
            module_path = modules / "module.prx"
            module_path.write_bytes(image)
            output = case / "output.bin"
            result = subprocess.run([str(relinker), *options, str(source), str(output)],
                                    capture_output=True, text=True, timeout=20)
            assert source.read_bytes() == original and module_path.read_bytes() == image
            return result, output, case / "app0" / "sce_module" / "module.prx.guest.prx"

        for system, options in (("linux", []), ("windows", ["--windows"]), ("macos", ["--macos"])):
            for label, address, kind, memory_size in positive:
                result, output, artifact = convert(label, system, options, module(address, 0, kind, memory_size))
                assert result.returncode == 0 and output.is_file(), (system, label, result.stdout, result.stderr)
                data = artifact.read_bytes()
                if system == "linux":
                    check_linux(data, address, kind)
                elif system == "windows":
                    check_windows(data, address)
                else:
                    check_macos(data, address)
            for label, image in negative:
                result, output, artifact = convert(label, system, options, image)
                assert result.returncode == 2 and "Unmapped or inaccessible guest address" in result.stderr, (system, label, result.stdout, result.stderr)
                assert not output.exists() and not artifact.exists(), (system, label)
    print("39 zero-sized guest symbol cases passed")


if __name__ == "__main__":
    main()
