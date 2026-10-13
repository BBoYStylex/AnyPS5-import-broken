import os
from pathlib import Path
import platform
import struct
import subprocess
import sys
import tempfile

from test_linux_load_alignment import fixture, loads
from test_linux_dynamic_strings import dynamic_strings, file_offset


def name_hash(name):
    value = 5381
    for byte in name.encode():
        value = (value * 33 + byte) & 0xFFFFFFFF
    return value


def image(kind, library=None):
    elf = fixture()
    struct.pack_into("<Q", elf, 24, 0x210)
    names = ["FixtureExport#A#B", "OtherExport", "ThirdExport"]
    bucket_count = 3 if kind == "gnu-multiple" else 1
    names.sort(key=lambda name: name_hash(name) % bucket_count, reverse=True)
    strings = bytearray(b"\0")
    buckets = [0] * bucket_count
    chains = []
    bloom = 0
    previous_bucket = None
    for index, name in enumerate(names, 1):
        code = 0x210 + (index - 1) * 16
        elf[0x4000 + code:0x4000 + code + 6] = bytes.fromhex("b82a000000c3")
        struct.pack_into("<IBBHQQ", elf, 0x4900 + index * 24,
                         len(strings), 0x12, 0, 1, code, 6)
        strings.extend(name.encode() + b"\0")
        hashed = name_hash(name)
        bucket = hashed % bucket_count
        if bucket != previous_bucket:
            buckets[bucket] = index
            if chains:
                chains[-1] |= 1
        chains.append(hashed & ~1)
        bloom |= (1 << (hashed % 64)) | (1 << ((hashed >> 5) % 64))
        previous_bucket = bucket
    chains[-1] |= 1
    tags = [(5, 0x600), (10, 0), (6, 0x900), (11, 24),
            (7, 0x700), (8, 0), (9, 24)]
    if kind == "sce":
        tags.append((0x6100003F, 96))
    elif kind == "sysv":
        struct.pack_into("<IIIIIII", elf, 0x4A00, 1, 4, 1, 0, 2, 3, 0)
        tags.append((4, 0xA00))
    else:
        struct.pack_into("<IIIIQ", elf, 0x4A00, bucket_count, 1, 1, 5, bloom)
        struct.pack_into("<" + "I" * (len(buckets) + len(chains)), elf, 0x4A18,
                         *buckets, *chains)
        tags.append((0x6FFFFEF5, 0xA00))
    if library is not None:
        tags.append((1, len(strings)))
        strings.extend(str(library).encode() + b"\0")
    tags[1] = (10, len(strings))
    tags.append((0, 0))
    elf[0x4600:0x4600 + len(strings)] = strings
    struct.pack_into("<IIQQQQQQ", elf, 120, 2, 6, 0x4800, 0x800, 0x800,
                     len(tags) * 16, len(tags) * 16, 8)
    for index, tag in enumerate(tags):
        struct.pack_into("<qQ", elf, 0x4800 + index * 16, *tag)
    return elf


def run(relinker, work, name, elf, error=None, execute=False):
    source = work / (name + ".elf")
    output = work / (name + ".out")
    source.write_bytes(elf)
    result = subprocess.run([str(relinker), "--skip-sce-module", str(source), str(output)],
                            capture_output=True, text=True, timeout=10)
    if error is not None:
        assert result.returncode == 2 and error in result.stderr and not output.exists(), (
            name, result.returncode, result.stdout, result.stderr)
        return
    assert result.returncode == 0, (name, result.stdout, result.stderr)
    data = output.read_bytes()
    tags, strings = dynamic_strings(data)
    segments = loads(data)
    count = struct.unpack_from("<I", data, file_offset(data, segments, tags[4]) + 4)[0]
    symbols = file_offset(data, segments, tags[6])
    found = {}
    for index in range(count):
        entry = struct.unpack_from("<IBBHQQ", data, symbols + index * 24)
        symbol_name = strings[entry[0]:].split(b"\0", 1)[0]
        if symbol_name:
            found[symbol_name] = entry
    assert set(found) == {b"FixtureExport", b"OtherExport", b"ThirdExport"}, (name, found)
    for entry in found.values():
        assert entry[1:4] == (0x12, 0, 1) and entry[5] == 6, entry
        code = file_offset(data, segments, entry[4])
        assert data[code:code + 6] == bytes.fromhex("b82a000000c3"), entry
    if execute:
        output.chmod(0o755)
        executed = subprocess.run([str(output)], capture_output=True, timeout=10)
        assert executed.returncode == 42, (executed.returncode, executed.stderr)


def main():
    relinker = Path(sys.argv[1]).resolve()
    execute = sys.platform == "linux" and platform.machine() == "x86_64"
    with tempfile.TemporaryDirectory(prefix="anyps5-main-export-") as directory:
        work = Path(directory)
        library = None
        if execute:
            probe = work / "probe.c"
            probe.write_text(
                "#include <dlfcn.h>\n"
                "#include <unistd.h>\n"
                "void check(void) {\n"
                "    const char *names[] = {\"FixtureExport\", \"OtherExport\", \"ThirdExport\"};\n"
                "    for (int index = 0; index < 3; ++index) {\n"
                "        int (*function)(void) = dlsym(RTLD_DEFAULT, names[index]);\n"
                "        if (!function || function() != 42) _exit(1);\n"
                "    }\n"
                "    _exit(42);\n"
                "}\n")
            library = work / "probe.so"
            subprocess.run([os.environ.get("CC", "cc"), "-shared", "-fPIC", str(probe),
                            "-o", str(library), "-ldl", "-Wl,-init,check"], check=True, timeout=10)
        for kind in ("sce", "sysv", "gnu", "gnu-multiple"):
            run(relinker, work, kind, image(kind, library), execute=execute)

        elf = image("sce")
        struct.pack_into("<Q", elf, 0x4800 + 7 * 16 + 8, 95)
        run(relinker, work, "partial-sce-symbol", elf, "dynamic symbol table size")

        elf = image("sysv")
        struct.pack_into("<I", elf, 0x4A04, 0xFFFFFFFF)
        run(relinker, work, "sysv-size-overflow", elf, "unmapped address")

        for kind in ("sysv", "gnu"):
            elf = image(kind)
            struct.pack_into("<Q", elf, 0x4800 + 2 * 16 + 8, 0xFD0)
            run(relinker, work, kind + "-symbols-outside-load", elf, "unmapped address")

        elf = image("sysv")
        struct.pack_into("<QQ", elf, 0x4800 + 7 * 16, 4, 0xFF8)
        struct.pack_into("<IIIII", elf, 0x4FF8, 1, 2, 1, 0, 0)
        run(relinker, work, "sysv-prefix-outside-load", elf, "unmapped address")

        elf = image("gnu")
        struct.pack_into("<Q", elf, 0x4800 + 7 * 16 + 8, 0xFE8)
        struct.pack_into("<IIIIQII", elf, 0x4FE8, 1, 1, 1, 5, 0, 1, 1)
        run(relinker, work, "gnu-prefix-outside-load", elf, "unmapped address")

        elf = image("gnu")
        struct.pack_into("<Q", elf, 0x4800 + 7 * 16 + 8, 0xFE0)
        struct.pack_into("<IIIIQII", elf, 0x4FE0, 1, 1, 1, 5, 0, 1, 2)
        run(relinker, work, "gnu-unterminated-chain", elf, "unmapped address")
    print("Executable export metadata and bounds tests passed; Linux runtime:", execute)


if __name__ == "__main__":
    main()
