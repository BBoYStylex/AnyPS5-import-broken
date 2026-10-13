import json
import os
from pathlib import Path
import platform
import signal
import struct
import subprocess
import sys
import tempfile

from test_unused_filter_indirect import CODE, FUNCTION, POINTER, SLOT, at_offset, build, rel
from test_linux_dynamic_strings import dynamic_strings, file_offset
from test_linux_load_alignment import loads
from test_windows_fixed_base import file_offset as pe_file_offset, pe_headers


def fixture(kind=1, binding=0, addend=0, absolute=False, imported=False, linux=False):
    entry = bytes.fromhex("4883ec08") + b"\xff\x15" + rel(CODE + 10, POINTER) + bytes.fromhex("4883c408")
    if linux:
        entry += bytes.fromhex("83f82a7501cc0f0b")
    else:
        entry += b"\xc3"
    target = FUNCTION - addend if kind == 1 else FUNCTION
    function = b"\xb8\x2a\0\0\0\xc3"
    if absolute:
        target = 0x1234
        entry = b"\x48\x8b\x05" + rel(CODE + 7, POINTER)
        entry += b"\x48\x3d" + struct.pack("<I", target + addend)
        entry += bytes.fromhex("7506b82a000000c3b807000000c3") if not linux else bytes.fromhex("7501cc0f0b")
    if imported:
        function = b"\x48\x8b\x05" + rel(FUNCTION + 7, SLOT) + b"\xb8\x2a\0\0\0\xc3"
    if kind == 7:
        entry = bytes.fromhex("4883ec08") + b"\xe8" + rel(CODE + 9, CODE + 0x20) + entry[10:]
        thunk = b"\xff\x25" + rel(CODE + 0x26, POINTER) + b"\x68\0\0\0\0\xe9" + rel(CODE + 0x30, CODE + 0x80)
    else:
        thunk = b""
    image = build(at_offset([(0, entry), (0x20, thunk), (0x40, function)]))
    image.extend(bytes(0x2000 - len(image)))
    struct.pack_into("<QQ", image, 176 + 32, 0x1000, 0x1000)
    if kind == 7:
        struct.pack_into("<Q", image, POINTER, CODE + 0x26)
        plt0 = b"\xff\x35" + rel(CODE + 0x86, 0x848) + b"\xff\x25" + rel(CODE + 0x8c, 0x850)
        image[CODE + 0x80:CODE + 0x80 + len(plt0)] = plt0
    strings = b"\0internal\0foo\0lib.so\0"
    image[0x600:0x600 + len(strings)] = strings
    struct.pack_into("<IBBHQQ", image, 0x638, 1, (binding << 4) | (1 if absolute else 2), 0,
                     0xfff1 if absolute else 1, target, len(function))
    struct.pack_into("<IBBHQQ", image, 0x650, 10, 0x12, 0, 0, 0, 0)
    ordinary = [(SLOT, (2 << 32) | 6, 0)] if imported else []
    if kind != 7:
        ordinary.append((POINTER, (1 << 32) | kind, addend))
    for index, relocation in enumerate(ordinary):
        struct.pack_into("<QQq", image, 0x700 + index * 24, *relocation)
    tags = [(5, 0x600), (10, len(strings)), (6, 0x620), (11, 24),
            (7, 0x700), (8, len(ordinary) * 24), (9, 24), (0x6100003f, 72)]
    if imported:
        tags += [(1, 14)]
    if kind == 7:
        tags += [(3, 0x840), (2, 24), (20, 7), (23, 0x760)]
        struct.pack_into("<QQq", image, 0x760, POINTER, (1 << 32) | 7, addend)
    tags += [(0, 0)]
    struct.pack_into("<QQ", image, 152, len(tags) * 16, len(tags) * 16)
    for index, tag in enumerate(tags):
        struct.pack_into("<qQ", image, 0x400 + index * 16, *tag)
    return image


def check_symbols(data, absolute, binding, imported):
    tags, strings = dynamic_strings(data)
    segments = loads(data)
    symbols = file_offset(data, segments, tags[6])
    hash_table = file_offset(data, segments, tags[4])
    count = struct.unpack_from("<I", data, hash_table + 4)[0]
    entries = [struct.unpack_from("<IBBHQQ", data, symbols + index * 24) for index in range(count)]
    definitions = [entry for entry in entries if strings[entry[0]:].split(b"\0", 1)[0] == b"internal"]
    assert definitions and all(entry[3] == (0xfff1 if absolute else 1) for entry in definitions), definitions
    assert all(entry[1] >> 4 == binding for entry in definitions), definitions
    first_global = next((i for i, entry in enumerate(entries) if entry[1] >> 4 != 0), count)
    assert all(entry[1] >> 4 != 0 for entry in entries[first_global:]), entries
    shoff = struct.unpack_from("<Q", data, 40)[0]
    assert struct.unpack_from("<I", data, shoff + 3 * 64 + 44)[0] == first_global
    undefined = [strings[entry[0]:].split(b"\0", 1)[0] for entry in entries[1:] if entry[3] == 0]
    assert undefined == ([b"foo"] if imported else []), undefined


def check_pe(data, absolute, kind, addend):
    _, optional = pe_headers(data)
    image_base = struct.unpack_from("<Q", data, optional + 24)[0]
    slot = 0x10000 + POINTER
    value = struct.unpack_from("<Q", data, pe_file_offset(data, slot))[0]
    assert value == (0x1234 + addend if absolute else image_base + 0x10000 + FUNCTION), hex(value)
    rva, size = struct.unpack_from("<II", data, optional + 112 + 5 * 8)
    targets = []
    if size:
        offset = pe_file_offset(data, rva)
        end = offset + size
        while offset < end:
            page, block_size = struct.unpack_from("<II", data, offset)
            assert block_size >= 8 and offset + block_size <= end
            for at in range(offset + 8, offset + block_size, 2):
                entry = struct.unpack_from("<H", data, at)[0]
                if entry >> 12 == 10:
                    targets.append(page + (entry & 0xfff))
            offset += block_size
    assert (slot in targets) == (not absolute), targets


def main():
    relinker = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="anyps5-defined-relocations-") as directory:
        work = Path(directory)
        for kind, binding, addend, absolute in ((1, 0, 0, False), (1, 1, 4, False),
                                                (1, 2, -4, False), (6, 0, 0, False),
                                                (7, 0, 0, False), (1, 0, 7, True)):
            for level in (0, 1, 2):
                for windows in (False, True):
                    name = f"{kind}-{binding}-{addend}-{absolute}-{level}-{windows}"
                    source = work / (name + ".elf")
                    output = work / (name + (".exe" if windows else ".out"))
                    source.write_bytes(fixture(kind, binding, addend, absolute, linux=not windows))
                    command = [str(relinker), "--skip-sce-module", "--registry", f"unused-filter={level}"]
                    if windows:
                        command += ["--windows"]
                    result = subprocess.run([*command, str(source), str(output)], capture_output=True, text=True, timeout=10)
                    assert result.returncode == 0, (name, result.stdout, result.stderr)
                    registry = json.loads(output.with_suffix(".registry.json").read_text())
                    assert registry == [], (name, registry)
                    if not windows:
                        check_symbols(output.read_bytes(), absolute, binding, False)
                    else:
                        check_pe(output.read_bytes(), absolute, kind, addend)
                    native = os.name == "nt" if windows else sys.platform == "linux" and platform.machine() == "x86_64"
                    if native:
                        output.chmod(0o755)
                        executed = subprocess.run([str(output)], capture_output=True, timeout=10)
                        expected = 42 if windows else -signal.SIGTRAP
                        assert executed.returncode == expected, (name, executed.returncode, executed.stderr)
                    if kind == 7 and not windows:
                        lazy = output.with_name("lazy-" + output.name)
                        result = subprocess.run([*command, "--lazy-binding", str(source), str(lazy)],
                                                capture_output=True, text=True, timeout=10)
                        assert result.returncode == 0, (name, result.stdout, result.stderr)
                        if native:
                            lazy.chmod(0o755)
                            executed = subprocess.run([str(lazy)], capture_output=True, timeout=10)
                            assert executed.returncode == -signal.SIGTRAP, (name, executed.returncode, executed.stderr)
        for kind in (1, 7):
            for level in (1, 2):
                source = work / f"mixed-{kind}-{level}.elf"
                output = source.with_suffix(".out")
                source.write_bytes(fixture(kind, imported=True))
                result = subprocess.run([str(relinker), "--skip-sce-module", "--registry", f"unused-filter={level}",
                                         str(source), str(output)], capture_output=True, text=True, timeout=10)
                assert result.returncode == 0, (result.stdout, result.stderr)
                registry = json.loads(output.with_suffix(".registry.json").read_text())
                assert [entry["nid"] for entry in registry] == ["foo"], registry
                check_symbols(output.read_bytes(), False, 0, True)
        for level in (1, 2):
            image = fixture(7)
            struct.pack_into("<Q", image, POINTER, 0)
            struct.pack_into("<IIQQQQQQ", image, 64, 1, 6, 0x800, 0x800, 0x800, 0x800, 0x800, 0x1000)
            struct.pack_into("<IIQQQQQQ", image, 232, 0x61000000, 0, 0, 0, 0, 0x800, 0x800, 1)
            image[CODE + 0xa0:CODE + 0xb0] = (b"\xff\x25" + rel(CODE + 0xa6, SLOT)
                + b"\x68\0\0\0\0\xe9" + rel(CODE + 0xb0, CODE + 0x80))
            struct.pack_into("<I", image, CODE + 0x27, 1)
            struct.pack_into("<QQqQQq", image, 0x760, SLOT, (2 << 32) | 7, 0, POINTER, (1 << 32) | 7, 0)
            struct.pack_into("<Q", image, 0x400 + 9 * 16 + 8, 48)
            struct.pack_into("<qQqQ", image, 0x400 + 12 * 16, 1, 14, 0, 0)
            struct.pack_into("<QQ", image, 152, 14 * 16, 14 * 16)
            os_tags = {5: 0x61000035, 10: 0x61000037, 6: 0x61000039, 11: 0x6100003b,
                       7: 0x6100002f, 8: 0x61000031, 9: 0x61000033,
                       3: 0x61000027, 2: 0x6100002d, 20: 0x6100002b, 23: 0x61000029}
            for offset in range(0x400, 0x400 + 14 * 16, 16):
                tag = struct.unpack_from("<q", image, offset)[0]
                if tag in os_tags:
                    struct.pack_into("<q", image, offset, os_tags[tag])
            source = work / f"mixed-plt-{level}.elf"
            output = source.with_suffix(".out")
            source.write_bytes(image)
            result = subprocess.run([str(relinker), "--skip-sce-module", "--registry", f"unused-filter={level}",
                                     str(source), str(output)], capture_output=True, text=True, timeout=10)
            assert result.returncode == 0, (result.stdout, result.stderr)
            data = output.read_bytes()
            tags, _ = dynamic_strings(data)
            assert tags[2] == (48 if level == 1 else 24), (tags, result.stdout)
            thunk = file_offset(data, loads(data), CODE + 0x27)
            assert struct.unpack_from("<I", data, thunk)[0] == (1 if level == 1 else 0)
            registry = json.loads(output.with_suffix(".registry.json").read_text())
            assert [entry["nid"] for entry in registry] == (["foo"] if level == 1 else []), registry
            check_symbols(data, False, 0, level == 1)
        for index, value, message in ((6, 0xfff2, "defined symbol section"), (4, 6, "defined symbol type")):
            image = fixture()
            struct.pack_into("<H" if index == 6 else "B", image, 0x638 + index, value)
            source = work / f"invalid-{index}.elf"
            output = source.with_suffix(".out")
            source.write_bytes(image)
            result = subprocess.run([str(relinker), "--skip-sce-module", str(source), str(output)],
                                    capture_output=True, text=True, timeout=10)
            assert result.returncode == 2 and message in result.stderr and not output.exists(), result.stderr
    print("Defined executable relocation tests passed")


if __name__ == "__main__":
    main()
