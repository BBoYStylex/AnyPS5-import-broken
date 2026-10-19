import re
import struct
import sys
from pathlib import Path

DT_NULL = 0
DT_NEEDED = 1
DT_SONAME = 14
DT_RPATH = 15
DT_RUNPATH = 29
SHT_DYNAMIC = 6
MH_MAGIC_64 = 0xFEEDFACF
LC_LOAD_DYLIB = 0x0C
LC_RPATH = 0x8000001C


def load_command_strings(image):
    # macOS builds the fixture as Mach-O: LC_LOAD_DYLIB and LC_RPATH hold what DT_NEEDED and DT_RUNPATH hold in ELF.
    commands, = struct.unpack_from("<I", image, 0x10)
    entries = []
    position = 0x20
    for _ in range(commands):
        command, size = struct.unpack_from("<II", image, position)
        if command in (LC_LOAD_DYLIB, LC_RPATH):
            offset, = struct.unpack_from("<I", image, position + 8)
            assert offset < size, (command, offset, size)
            start = position + offset
            end = image.index(0, start)
            assert end < position + size, (command, start, end)
            entries.append((DT_NEEDED if command == LC_LOAD_DYLIB else DT_RUNPATH, image[start:end].decode()))
        position += size
    return entries


def dynamic_strings(image):
    if struct.unpack_from("<I", image, 0)[0] == MH_MAGIC_64:
        return load_command_strings(image)
    shoff, = struct.unpack_from("<Q", image, 0x28)
    shentsize, shnum = struct.unpack_from("<HH", image, 0x3A)
    sections = [struct.unpack_from("<IIQQQQIIQQ", image, shoff + index * shentsize) for index in range(shnum)]
    dynamic = next(section for section in sections if section[1] == SHT_DYNAMIC)
    strings = sections[dynamic[6]]
    entries = []
    for position in range(dynamic[4], dynamic[4] + dynamic[5], 16):
        tag, value = struct.unpack_from("<qQ", image, position)
        if tag == DT_NULL:
            break
        if tag in (DT_NEEDED, DT_SONAME, DT_RPATH, DT_RUNPATH):
            assert value < strings[5], (tag, value)
            start = strings[4] + value
            entries.append((tag, image[start:image.index(0, start)].decode()))
    return entries


def main():
    image = Path(sys.argv[1]).read_bytes()
    entries = dynamic_strings(image)
    runpaths = [value for tag, value in entries if tag in (DT_RPATH, DT_RUNPATH)]
    assert runpaths == ["$ORIGIN"], entries
    needed = [value for tag, value in entries if tag == DT_NEEDED]
    if struct.unpack_from("<I", image, 0)[0] == MH_MAGIC_64:
        assert needed and all(re.fullmatch(r"(/[\w.+-]+)+\.dylib", name) for name in needed), entries
    else:
        assert needed and all(re.fullmatch(r"[\w.+-]+\.so(\.\d+)*", name) for name in needed), entries
    print("NID patcher dynamic string tests passed")


if __name__ == "__main__":
    main()
