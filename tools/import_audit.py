import argparse
import json
import re
import struct
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

import nid_names
import progress

ROOT = Path(__file__).resolve().parent.parent
PRX = ROOT / "core" / "libs" / "prx"
NID = re.compile(r"[A-Za-z0-9+\-]{11}")
EXPORT_ALIAS = re.compile(r'APS5_EXPORT\("([A-Za-z0-9+\-]{11})",\s*(\w+)\)')
NID_POSTFIX = "_nid_postfix"
CLASSES = ("implemented", "stub", "absent", "module")
PE_DIRECTORY_OFFSET = {0x20b: 112, 0x10b: 96}
MACH_O_MAGIC_64 = 0xFEEDFACF
MACH_O_LC_DYLD_INFO = 0x22
MACH_O_LC_DYLD_INFO_ONLY = 0x80000022
MACH_O_LC_DYLD_EXPORTS_TRIE = 0x80000033
MACH_O_EXPORT_WEAK_DEFINITION = 0x04
MACH_O_EXPORT_REEXPORT = 0x08
MACH_O_EXPORT_STUB_AND_RESOLVER = 0x10


class AuditError(Exception):
    pass


def take(data, offset, size, path, what):
    if offset < 0 or size < 0 or offset + size > len(data):
        raise AuditError(f"{path}: {what} at 0x{offset:x} with size {size} lies outside the file ({len(data)} bytes)")
    return data[offset:offset + size]


def unpack(layout, data, offset, path, what):
    return struct.unpack(layout, take(data, offset, struct.calcsize(layout), path, what))


def cstring(data, offset, limit, path, what):
    end = data.find(b"\0", offset, limit)
    if offset < 0 or offset >= limit or end < 0:
        raise AuditError(f"{path}: {what} at 0x{offset:x} is not a NUL-terminated string inside 0x{limit:x} bytes")
    try:
        return data[offset:end].decode("utf-8")
    except UnicodeDecodeError as error:
        raise AuditError(f"{path}: {what} at 0x{offset:x} is not UTF-8: {error}") from error


def elf_exports(path, data):
    header = unpack("<16sHHIQQQIHHHHHH", data, 0, path, "ELF header")
    ident, section_offset, section_size, section_count = header[0], header[6], header[11], header[12]
    if ident[4] != 2 or ident[5] != 1 or section_size != 64:
        raise AuditError(f"{path}: only little-endian ELF64 with 64-byte section headers is supported (class {ident[4]}, data {ident[5]}, shentsize {section_size})")
    sections = [unpack("<IIQQQQIIQQ", data, section_offset + index * 64, path, f"section header {index}") for index in range(section_count)]
    names = set()
    for section in sections:
        kind, offset, size, link, entry = section[1], section[4], section[5], section[6], section[9]
        if kind != 11:
            continue
        if entry != 24 or size % 24 or link >= len(sections) or sections[link][1] != 3:
            raise AuditError(f"{path}: invalid dynamic symbol table (entry size {entry}, size {size}, string table section {link})")
        strings_offset, strings_size = sections[link][4], sections[link][5]
        take(data, strings_offset, strings_size, path, "dynamic string table")
        for position in range(0, size, 24):
            name, info, other, index, _, _ = unpack("<IBBHQQ", data, offset + position, path, "dynamic symbol")
            binding, visibility = info >> 4, other & 3
            if index == 0 or binding not in (1, 2, 10) or visibility not in (0, 3):
                continue
            names.add(cstring(data, strings_offset + name, strings_offset + strings_size, path, "export name"))
    return names


def pe_exports(path, data):
    header = unpack("<I", data, 0x3c, path, "PE header offset")[0]
    if unpack("<I", data, header, path, "PE signature")[0] != 0x4550:
        raise AuditError(f"{path}: invalid PE signature at 0x{header:x}")
    section_count, optional_size = unpack("<H", data, header + 6, path, "section count")[0], unpack("<H", data, header + 20, path, "optional header size")[0]
    optional = header + 24
    magic = unpack("<H", data, optional, path, "optional header magic")[0]
    if magic not in PE_DIRECTORY_OFFSET:
        raise AuditError(f"{path}: unsupported PE optional header magic 0x{magic:x}")
    directory = PE_DIRECTORY_OFFSET[magic]
    if optional_size < directory + 8:
        raise AuditError(f"{path}: PE optional header of {optional_size} bytes has no export directory")
    rva, size = unpack("<II", data, optional + directory, path, "export data directory")
    if not rva or size < 40:
        raise AuditError(f"{path}: PE file has no export table")
    sections = [unpack("<8sIIIIIIHHI", data, optional + optional_size + index * 40, path, f"PE section {index}") for index in range(section_count)]

    def to_offset(address, length):
        for section in sections:
            virtual, raw_size, raw_offset = section[2], section[3], section[4]
            if address < virtual:
                continue
            relative = address - virtual
            if relative >= raw_size or length > raw_size - relative:
                continue
            take(data, raw_offset + relative, length, path, f"data at RVA 0x{address:x}")
            return raw_offset + relative
        raise AuditError(f"{path}: RVA 0x{address:x} is not inside any section")

    table = unpack("<IIHHIIIIIII", data, to_offset(rva, 40), path, "export directory")
    name_count, names_rva = table[7], table[9]
    names = set()
    if not name_count:
        return names
    names_offset = to_offset(names_rva, name_count * 4)
    for index in range(name_count):
        name_rva = unpack("<I", data, names_offset + index * 4, path, "export name RVA")[0]
        offset = to_offset(name_rva, 1)
        names.add(cstring(data, offset, len(data), path, "export name"))
    return names


def read_uleb(data, cursor, end, path):
    value = 0
    shift = 0
    while True:
        if cursor >= end:
            raise AuditError(f"{path}: truncated ULEB128 at 0x{cursor:x}")
        byte = data[cursor]
        cursor += 1
        value |= (byte & 0x7F) << shift
        if not byte & 0x80:
            return value, cursor
        shift += 7
        if shift > 63:
            raise AuditError(f"{path}: ULEB128 at 0x{cursor:x} is longer than 64 bits")


def parse_trie(data, start, end, node, prefix, path, visited, out):
    if node >= end - start:
        raise AuditError(f"{path}: export trie node at 0x{start + node:x} is out of bounds")
    if node in visited:
        raise AuditError(f"{path}: export trie has a cycle at 0x{start + node:x}")
    visited.add(node)
    cursor = start + node
    terminal_size, cursor = read_uleb(data, cursor, end, path)
    if terminal_size > end - cursor:
        raise AuditError(f"{path}: export trie terminal at 0x{cursor:x} runs past the trie")
    children = cursor + terminal_size
    if terminal_size != 0:
        flags, cursor = read_uleb(data, cursor, children, path)
        if flags & MACH_O_EXPORT_REEXPORT:
            _, cursor = read_uleb(data, cursor, children, path)
        else:
            _, cursor = read_uleb(data, cursor, children, path)
            if flags & MACH_O_EXPORT_STUB_AND_RESOLVER:
                _, cursor = read_uleb(data, cursor, children, path)
        # A weak definition is an export the linker may drop, which the ELF
        # reader skips too.
        if not flags & MACH_O_EXPORT_WEAK_DEFINITION:
            out.add(prefix)
    cursor = children
    if cursor >= end:
        raise AuditError(f"{path}: export trie child count at 0x{cursor:x} is out of bounds")
    child_count = data[cursor]
    cursor += 1
    for _ in range(child_count):
        label, cursor = read_trie_string(data, cursor, end, path)
        child, cursor = read_uleb(data, cursor, end, path)
        parse_trie(data, start, end, child, prefix + label, path, visited, out)


def read_trie_string(data, cursor, end, path):
    stop = data.find(b"\0", cursor, end)
    if cursor >= end or stop < 0:
        raise AuditError(f"{path}: export trie string at 0x{cursor:x} is not NUL-terminated")
    return data[cursor:stop].decode("utf-8", "replace"), stop + 1


def macho_exports(path, data):
    header = unpack("<IiiIIIII", data, 0, path, "Mach-O header")
    if header[0] != MACH_O_MAGIC_64:
        raise AuditError(f"{path}: only 64-bit little-endian Mach-O is supported (magic 0x{header[0]:x})")
    _, _, _, _, command_count, command_size, _, _ = header
    if command_size < 8:
        raise AuditError(f"{path}: Mach-O declares {command_size} load command bytes, too few for any command")
    take(data, 32, command_size, path, "Mach-O load commands")
    trie = None
    at = 32
    for index in range(command_count):
        if at + 8 > 32 + command_size:
            raise AuditError(f"{path}: Mach-O load command {index} runs past the {command_size} load command bytes")
        kind, size = unpack("<II", data, at, path, f"Mach-O load command {index}")
        if size < 8:
            raise AuditError(f"{path}: Mach-O load command {index} declares a size of {size}")
        if kind in (MACH_O_LC_DYLD_INFO, MACH_O_LC_DYLD_INFO_ONLY):
            export_offset, export_size = unpack("<12I", data, at, path, "LC_DYLD_INFO")[10:12]
            trie = (export_offset, export_size)
        elif kind == MACH_O_LC_DYLD_EXPORTS_TRIE:
            export_offset, export_size = unpack("<IIII", data, at, path, "LC_DYLD_EXPORTS_TRIE")[2:4]
            trie = (export_offset, export_size)
        at += size
    if trie is None or trie[0] == 0 or trie[1] == 0:
        return set()
    start, size = trie
    take(data, start, size, path, "export trie")
    names = set()
    parse_trie(data, start, start + size, 0, "", path, set(), names)
    # Mach-O prefixes C symbols with an underscore; dyld strips it again when it
    # binds, so the name the game sees is the one without it.
    return {name[1:] if name.startswith("_") else name for name in names}


def read_exports(path):
    data = path.read_bytes()
    if data[:4] == b"\x7fELF":
        return elf_exports(path, data)
    if data[:2] == b"MZ":
        return pe_exports(path, data)
    if data[:4] in (b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe"):
        return macho_exports(path, data)
    raise AuditError(f"{path}: neither ELF, PE nor Mach-O (first bytes {data[:4]!r})")


def directory_files(directory, pattern):
    path = Path(directory)
    if not path.is_dir():
        raise AuditError(f"{directory}: not a directory")
    return sorted(item for item in path.glob(pattern) if item.is_file())


def built_libraries(directories):
    exports = defaultdict(set)
    libraries = set()
    for directory in directories:
        files = directory_files(directory, "*.prx")
        if not files:
            raise AuditError(f"{directory}: no .prx files; pass the directory the libs target builds (build/core/libs/libs)")
        for file in files:
            libraries.add(file.name)
            for name in read_exports(file):
                exports[name].add(file.name)
    return {name: sorted(files) for name, files in exports.items()}, libraries


def module_files(directories):
    return {file.name for directory in directories for file in directory_files(directory, "*")}


def nid_of(name, aliases):
    if name in aliases:
        return aliases[name]
    return nid_names.compute_nid(name.removesuffix(NID_POSTFIX))


def source_stubs(source):
    if not Path(source).is_dir():
        raise AuditError(f"{source}: not a directory")
    stubs = {}
    for library in sorted(item for item in Path(source).iterdir() if item.is_dir()):
        aliases = {}
        for file in library.rglob("*.cpp"):
            for nid, name in EXPORT_ALIAS.findall(file.read_text(errors="ignore")):
                aliases[name] = nid
        group = progress.scan_library(library)
        done = {nid_of(name, aliases) for name in group["done_names"]}
        for name in group["todo_names"]:
            nid = nid_of(name, aliases)
            if nid not in done:
                stubs.setdefault((library.name + ".prx", nid), name)
    return stubs


def read_registry(path):
    try:
        entries = json.loads(Path(path).read_text(encoding="utf-8-sig"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise AuditError(f"{path}: cannot read registry: {error}") from error
    if not isinstance(entries, list):
        raise AuditError(f"{path}: a registry is a JSON list, got {type(entries).__name__}")
    imports = []
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict) or not isinstance(entry.get("nid"), str) or not isinstance(entry.get("library"), str):
            raise AuditError(f"{path}: entry {index} needs string fields 'nid' and 'library': {entry!r}")
        nid = entry["nid"].split("#", 1)[0]
        if not NID.fullmatch(nid):
            raise AuditError(f"{path}: entry {index} has an invalid NID {entry['nid']!r}")
        imports.append((nid, entry["library"]))
    return imports


def check_conservation(references, records):
    classified = sum(record["references"] for record in records)
    unknown = sorted({record["class"] for record in records} - set(CLASSES))
    if classified != references or unknown:
        raise AuditError(f"conservation failed: {references} references in, {classified} classified, unknown classes {unknown}")


def audit(imports, exports, stubs, libraries, modules):
    records = {}
    for nid, library in imports:
        record = records.setdefault((nid, library), {"nid": nid, "library": library, "references": 0})
        record["references"] += 1
    for record in records.values():
        nid, library = record["nid"], record["library"]
        providers = exports.get(nid, [])
        if library and library in modules:
            kind = "module"
        elif not providers:
            kind = "absent"
        elif all((provider, nid) in stubs for provider in providers):
            kind = "stub"
        else:
            kind = "implemented"
        record["class"] = kind
        record["providers"] = providers
        record["library_mismatch"] = kind in ("implemented", "stub") and bool(library) and library not in providers and "libc.prx" not in providers
        record["name"] = next((stubs[(provider, nid)] for provider in providers if (provider, nid) in stubs), None) if kind == "stub" else None
    ordered = sorted(records.values(), key=lambda record: (record["class"], record["library"], record["nid"]))
    check_conservation(len(imports), ordered)
    missing = defaultdict(int)
    for record in ordered:
        library = record["library"]
        if library and library not in libraries and library not in modules:
            missing[library] += 1
    return ordered, dict(sorted(missing.items()))


def load_names(path):
    if not Path(path).is_file():
        raise AuditError(f"{path}: names file not found; the audit never downloads one (see tools/nid_names.py for the source)")
    return nid_names.load_db(Path(path))


def attach_names(records, names):
    for record in records:
        if record["name"] is None and record["class"] == "absent" and record["nid"] in names:
            name = names[record["nid"]]
            record["name"] = name
            record["name_verified"] = nid_names.compute_nid(name) == record["nid"]


def git_stamp():
    try:
        commit = subprocess.run(["git", "-C", str(ROOT), "rev-parse", "HEAD"], capture_output=True, text=True, check=True).stdout.strip()
        dirty = bool(subprocess.run(["git", "-C", str(ROOT), "status", "--porcelain"], capture_output=True, text=True, check=True).stdout.strip())
    except (OSError, subprocess.CalledProcessError):
        return None, None
    return commit, dirty


def summarize(records, references, missing):
    counts = {kind: sum(1 for record in records if record["class"] == kind) for kind in CLASSES}
    by_references = {kind: sum(record["references"] for record in records if record["class"] == kind) for kind in CLASSES}
    return {
        "references": references,
        "unique_imports": len(records),
        "unique_by_class": counts,
        "references_by_class": by_references,
        "missing_libraries": missing,
        "library_mismatch": sum(1 for record in records if record["library_mismatch"]),
    }


def label(record):
    if record["name"] is None:
        return record["nid"]
    mark = "" if record.get("name_verified", True) else " (name does not hash to this NID)"
    return f"{record['nid']}  {record['name']}{mark}"


def render(summary, records, missing):
    lines = [f"{summary['references']} references, {summary['unique_imports']} unique imports"]
    for kind in CLASSES:
        lines.append(f"  {kind:<12}{summary['unique_by_class'][kind]:>6} imports  {summary['references_by_class'][kind]:>6} references")
    for title, kind in (("Absent: no built library exports these, so the loader fails", "absent"), ("Stub: exported but throws when called", "stub")):
        selected = [record for record in records if record["class"] == kind]
        if selected:
            lines.append("")
            lines.append(f"{title} ({len(selected)})")
            current = None
            for record in selected:
                if record["library"] != current:
                    current = record["library"]
                    lines.append(f"  [{current or 'library not named'}]")
                lines.append(f"    {label(record)}")
    if missing:
        lines.append("")
        lines.append(f"Needed libraries with no file in --libs or --modules ({len(missing)})")
        lines.extend(f"  {library}: {count} imports" for library, count in missing.items())
    if summary["library_mismatch"]:
        lines.append("")
        lines.append(f"{summary['library_mismatch']} imports are exported only by a library other than the one they name: they resolve on Linux and can fail on Windows")
    return "\n".join(lines)


def main(argv=None):
    parser = argparse.ArgumentParser(description="classify the imports of a converted game against the built system libraries")
    parser.add_argument("registries", nargs="+", type=Path, help="JSON written by 'relinker --registry'")
    parser.add_argument("--libs", action="append", required=True, help="directory of built .prx files; repeat for several")
    parser.add_argument("--modules", action="append", default=[], help="directory of the title's own modules; libraries named there are not checked")
    parser.add_argument("--source", default=str(PRX), help="core/libs/prx tree used to find throwing stubs (default: this repository's)")
    parser.add_argument("--names", help="aerolib.csv style file (NID, name per line) used to name absent imports")
    parser.add_argument("--json", type=Path, help="write the full result to this file")
    args = parser.parse_args(argv)
    try:
        imports = [entry for registry in args.registries for entry in read_registry(registry)]
        exports, libraries = built_libraries(args.libs)
        modules = module_files(args.modules)
        records, missing = audit(imports, exports, source_stubs(args.source), libraries, modules)
        if args.names:
            attach_names(records, load_names(args.names))
        summary = summarize(records, len(imports), missing)
        if args.json:
            commit, dirty = git_stamp()
            result = {"source_commit": commit, "source_dirty": dirty, "registries": [str(path) for path in args.registries], **summary, "imports": records}
            args.json.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    except AuditError as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 2
    except OSError as error:
        print(f"FAIL: {error.strerror}: {error.filename}" if error.filename else f"FAIL: {error}", file=sys.stderr)
        return 2
    print(render(summary, records, missing))
    return 1 if summary["unique_by_class"]["absent"] or missing else 0


if __name__ == "__main__":
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    raise SystemExit(main())
