#!/usr/bin/env python3
"""Deterministic pre-commit checker against low-effort "slop" additions.

Scope: only newly added lines are inspected (staged diff by default), so the
current tree is always clean and any finding points at the change under review.
The check is deterministic: findings are sorted, there is no timing or random
input, and only the Python standard library is used.

Usage:
    python3 tools/anti_slop.py                  # check staged changes (pre-commit)
    python3 tools/anti_slop.py --full-tree       # audit every file under --root
    python3 tools/anti_slop.py --root <dir>      # repository root (default: git top level)

Rules (each finding prints ``file:line [rule-id] detail``):
    no-stub
        Newly added NotImplemented-style stubs (``NotImplemented_nid_no_patch``
        or equivalent "not implemented" markers) in C/C++ files, unless the
        file is already registered in the "Silent stubs" section of
        ``docs/dev/TechnicalDebt.md`` (either in the working tree or via a
        link added in the same staged diff). New UI-less behaviour must be
        declared as technical debt in the same change.
    empty-catch
        A newly added ``catch`` block whose body swallows the error: the body
        is empty, only silences the exception (``(void)`` casts), or only logs
        and continues. A catch must rethrow, return/take an error path, abort,
        or do real handling work. Exempt: catches of a named
        shutdown/sentinel exception type with an empty body (sentinel control
        flow), and ``catch (...)`` whose body only invokes a handler or
        terminates (terminate guard).
    const-cast
        Any newly added ``const_cast`` that removes constness outside
        ``3rdparty/``. Casts that add const (``const_cast<const void*>``) and
        the standard Vulkan pNext-chain idiom (``const_cast<void*>`` on a
        ``pNext`` chain) silence nothing and are exempt.
    magic-hardware-address
        A newly added hexadecimal literal above ``0x1000`` in shader/AGC paths
        (``core/shader/`` or any path containing ``Agc``) that is not given a
        named symbol. Define a ``constexpr`` constant, a ``#define``, or an
        enum value instead of inlining the address. Exempt: braced data-table
        rows (lines starting with ``{``, such as register-default tables),
        ``case`` labels, and literals directly compared against or combined
        with masks (adjacent ``== != <= >= << >> & | ~ ^``). A line carrying
        the inline token ``// anti-slop: allow(reason)`` is exempt from every
        rule; the reason is mandatory.

Only C/C++ sources are inspected (.c, .cc, .cpp, .cxx, .h, .hh, .hpp, .hxx,
.inl, .ipp). ``3rdparty/`` and ``tests/``/``test/`` directories are skipped:
vendored code is not ours to fix, and tests legitimately need raw values and
exception assertions.

Exit code 0 means clean, 1 means findings were reported.
"""

import argparse
import os
import re
import subprocess
import sys

CPP = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl", ".ipp"}

RULE_IDS = ("no-stub", "empty-catch", "const-cast", "magic-hardware-address")

STRINGS = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
LINE_COMMENT = re.compile(r"//.*")
BLOCK_COMMENT_INLINE = re.compile(r"/\*.*?\*/")
STUB = re.compile(r"NotImplemented_nid_no_patch|\bnot[-_ ]implemented\b", re.I)
CONST_CAST = re.compile(r"\bconst_cast\s*<")
CATCH = re.compile(r"\bcatch\b")
CATCH_PARAM = re.compile(r"\bcatch\s*\(([^)]*)\)")
HEX = re.compile(r"\b0[xX]([0-9a-fA-F]+)[uUlL]*\b")
MASK_BEFORE = re.compile(r"(==|!=|<=|>=|<<|>>|&&|\|\||[&|~^])\s*$")
MASK_AFTER = re.compile(r"^\s*(==|!=|<=|>=|<<|>>|&&|\|\||[&|])")
CASE_LABEL = re.compile(r"\bcase\b")
ALLOW_TOKEN = re.compile(r"anti-slop:\s*allow\s*\(")
SENTINEL_TYPE = re.compile(r"shutdown|sentinel", re.I)
TERMINATE_GUARD_CALL = re.compile(
    r"\b(terminat\w*|abort\w*|exit\w*|panic\w*|fatal\w*|rethrow\w*|handle\w*)\s*\(",
    re.I,
)
NAMED_SYMBOL = re.compile(r"\bconstexpr\b|#\s*define\b|\benum\b|\bconst\b[^;{}]*=")
LOG_CALL = re.compile(
    r"\b(log|logger|printf|fprintf|sprintf|snprintf|vprintf|puts|perror|"
    r"cout|cerr|clog|cin|trace)\b"
    r"|\b(LOG|DLOG|VLOG|TRACE)(_[A-Z0-9]+)?\b"
    r"|\bOutputDebugString\w*\b",
    re.I,
)
HANDLER = re.compile(
    r"\bthrow\b|\breturn\b|\babort\b|\bexit\b|\bterminate\b|"
    r"\brethrow\b|\bassert\b|static_assert"
)
SILENCE = re.compile(r"\(\s*void\s*\)|static_cast\s*<\s*void\s*>")
LINK = re.compile(r"\]\(([^)\s]+)")
HUNK = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,\d+)? @@")


def bare(text):
    """Remove string literals and comments so patterns match code only."""
    text = STRINGS.sub('""', text)
    text = BLOCK_COMMENT_INLINE.sub("", text)
    text = LINE_COMMENT.sub("", text)
    return text


def is_cpp(path):
    return os.path.splitext(path)[1].lower() in CPP


def is_skipped(path):
    if path.startswith("3rdparty/"):
        return True
    parts = path.split("/")
    return "tests" in parts or "test" in parts


def is_shader_agc_path(path):
    return path.startswith("core/shader/") or "Agc" in path


def git_root():
    try:
        out = subprocess.run(
            ["git", "rev-parse", "--show-toplevel"],
            capture_output=True,
            check=True,
            text=True,
        )
        return out.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return os.getcwd()


def run_git(args, cwd):
    proc = subprocess.run(
        ["git"] + args, capture_output=True, text=True, cwd=cwd
    )
    if proc.returncode != 0:
        raise RuntimeError("git %s failed: %s" % (" ".join(args), proc.stderr.strip()))
    return proc.stdout


def debt_registered_files(repo_root, debt_rel="docs/dev/TechnicalDebt.md"):
    """Resolve relative link targets inside the Silent stubs section to repo paths.

    Only links under the ``### Silent stubs`` heading (up to the next ``###``
    heading) count: a stub registered anywhere else in the file does not
    allowlist new NotImplemented markers.
    """
    registered = set()
    try:
        with open(
            os.path.join(repo_root, debt_rel), encoding="utf-8", errors="replace"
        ) as handle:
            text = handle.read()
    except OSError:
        return registered
    section = re.search(
        r"^###\s+Silent stubs\s*$(.*?)(?=^###\s|\Z)", text, re.M | re.S
    )
    body = section.group(1) if section else ""
    parent = os.path.dirname(debt_rel)
    for target in LINK.findall(body):
        if re.match(r"[a-z]+:|#", target):
            continue
        target = target.split("#", 1)[0].split("?", 1)[0]
        if not target:
            continue
        if target.startswith("/"):
            resolved = os.path.normpath(target.lstrip("/"))
        else:
            resolved = os.path.normpath(os.path.join(parent, target))
        registered.add(resolved)
    return registered


def parse_unified_diff(diff_text):
    """Yield (path, new_line, is_added, text) for every content line."""
    path = None
    number = 0
    for line in diff_text.splitlines():
        if line.startswith("+++ "):
            rest = line[4:]
            path = None if rest == "/dev/null" else rest
            if path is not None and path.startswith(("a/", "b/")):
                path = path[2:]
        elif line.startswith("@@"):
            match = HUNK.search(line)
            if match:
                number = int(match.group(1))
        elif path is not None and line:
            kind = line[0]
            text = line[1:]
            if kind == "+":
                yield (path, number, True, text)
                number += 1
            elif kind == " ":
                yield (path, number, False, text)
                number += 1
            # '-' lines only exist in the old file: no new line number.


def staged_changes(repo_root):
    """Return added/context lines of the staged diff as {path: [(line, added, text)]}."""
    try:
        diff = run_git(
            ["diff", "--cached", "-M", "-U6", "--no-color", "--no-prefix"],
            repo_root,
        )
    except RuntimeError as exc:
        print("anti_slop: %s" % exc, file=sys.stderr)
        return {}
    changes = {}
    for path, number, added, text in parse_unified_diff(diff):
        changes.setdefault(path, []).append((number, added, text))
    return changes


def catch_body(lines, index):
    """Reconstruct the body of the catch starting at hunk line index.

    Only '+' (added) and ' ' (context) lines exist in the new file, so only
    those are followed. Returns the body text, or None when the body is not
    fully visible (truncated hunk: never flag what cannot be seen).
    """
    _, _, opening = lines[index]
    code = bare(opening)
    catch_at = CATCH.search(code)
    brace = code.find("{", catch_at.end() if catch_at else 0)
    pos = index
    if brace < 0:
        # Allman style: the opening brace is on a following new-file line.
        found = False
        for pos in range(index + 1, len(lines)):
            text = lines[pos][2]
            code = bare(text)
            brace = code.find("{")
            if brace >= 0:
                found = True
                break
            if text.strip() and text.strip() != "{":
                # No brace-led block follows: not a braced catch.
                return None
        if not found:
            return None
    # Scan forward from the opening brace; depth starts at 1. If the hunk
    # ends before depth returns to 0, the body is truncated and None is
    # returned so truncated bodies are never flagged.
    depth = 1
    chunks = []
    first = True
    for _, _, text in lines[pos:]:
        code = bare(text)
        start = brace + 1 if first else 0
        first = False
        i = start
        while i < len(code):
            if code[i] == "{":
                depth += 1
            elif code[i] == "}":
                depth -= 1
                if depth == 0:
                    return "".join(chunks) + code[start:i]
            i += 1
        chunks.append(code[start:] + "\n")
    return None


def swallows(body):
    """Decide whether a catch body swallows the error.

    Flags empty bodies, silence-only bodies ((void) casts), and log-and-continue
    bodies. Any sign of real handling (throw, return, abort/exit, assert, or an
    unrecognized statement) means the catch is not flagged.
    """
    fragments = [frag.strip() for frag in body.split(";")]
    fragments = [frag for frag in fragments if frag]
    if not fragments:
        return True
    seen_log_or_silence = False
    for frag in fragments:
        if HANDLER.search(frag):
            return False
        if LOG_CALL.search(frag) or SILENCE.search(frag):
            seen_log_or_silence = True
        else:
            return False
    return seen_log_or_silence


def const_cast_target(code, begin):
    """Return the target type inside const_cast<...> starting at begin.

    Parses balanced angle brackets so nested templates resolve. Returns None
    when the brackets never close.
    """
    open_at = code.find("<", begin)
    if open_at < 0:
        return None
    depth = 0
    for pos in range(open_at, len(code)):
        if code[pos] == "<":
            depth += 1
        elif code[pos] == ">":
            depth -= 1
            if depth == 0:
                return code[open_at + 1 : pos]
    return None


def check_added_line(path, number, text, registered):
    findings = []
    code = bare(text)
    if STUB.search(code) and path not in registered:
        findings.append(
            (path, number, "no-stub", "new stub is not registered in TechnicalDebt.md")
        )
    for match in CONST_CAST.finditer(code):
        target = const_cast_target(code, match.start())
        if target is None:
            findings.append(
                (path, number, "const-cast", "unparseable const_cast: %s" % text.strip()[:80])
            )
            continue
        if re.search(r"\bconst\b", target):
            continue  # casting TO const silences nothing
        if target.replace(" ", "") == "void*" and "pnext" in code.lower():
            continue  # standard Vulkan pNext-chain idiom
        findings.append(
            (path, number, "const-cast", "const_cast removes constness: %s" % text.strip()[:80])
        )
    if (
        is_shader_agc_path(path)
        and not NAMED_SYMBOL.search(code)
        and not code.strip().startswith("{")
        and not CASE_LABEL.search(code)
    ):
        for hex_match in HEX.finditer(code):
            try:
                value = int(hex_match.group(1), 16)
            except ValueError:
                continue
            if value <= 0x1000:
                continue
            before, after = code[: hex_match.start()], code[hex_match.end() :]
            if MASK_BEFORE.search(before) or MASK_AFTER.match(after):
                continue  # inline flag/bit mask comparison, not an address
            findings.append(
                (
                    path,
                    number,
                    "magic-hardware-address",
                    "hex literal 0x%s has no named symbol" % hex_match.group(1),
                )
            )
            break
    return findings


def check_changes(changes, registered):
    findings = []
    for path in sorted(changes):
        if not is_cpp(path) or is_skipped(path):
            continue
        lines = changes[path]
        for index, (number, added, text) in enumerate(lines):
            if not added:
                continue
            if ALLOW_TOKEN.search(text):
                continue  # explicit human waiver with a mandatory reason
            findings.extend(check_added_line(path, number, text, registered))
            catch_at = CATCH.search(bare(text))
            if catch_at:
                body = catch_body(lines, index)
                if body is None:
                    continue
                param = CATCH_PARAM.search(bare(text))
                param_text = param.group(1) if param else ""
                if SENTINEL_TYPE.search(param_text) and not body.strip():
                    continue  # sentinel-exception control flow, not an error
                if param_text.strip() == "..." and TERMINATE_GUARD_CALL.search(body):
                    continue  # terminate guard: the handler runs, nothing continues
                if swallows(body):
                    findings.append(
                        (path, number, "empty-catch", "catch block swallows the error")
                    )
    return findings


def check_full_tree(repo_root, registered):
    findings = []
    for dirpath, dirnames, filenames in os.walk(repo_root):
        dirnames[:] = sorted(
            d
            for d in dirnames
            if d not in (".git", "3rdparty") and d not in ("tests", "test")
        )
        for name in sorted(filenames):
            rel = os.path.relpath(os.path.join(dirpath, name), repo_root)
            if not is_cpp(rel) or is_skipped(rel):
                continue
            try:
                with open(
                    os.path.join(dirpath, name), encoding="utf-8", errors="replace"
                ) as handle:
                    content = handle.read()
            except OSError:
                continue
            fake = [(i + 1, True, line) for i, line in enumerate(content.splitlines())]
            changes = {rel: fake}
            findings.extend(check_changes(changes, registered))
    return findings


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Flag newly added low-effort code (slop) before commit."
    )
    parser.add_argument(
        "--full-tree",
        action="store_true",
        help="audit every file instead of only staged changes",
    )
    parser.add_argument(
        "--root",
        default=None,
        help="repository root (default: git top level)",
    )
    parser.add_argument(
        "--debt",
        default="docs/dev/TechnicalDebt.md",
        help="TechnicalDebt file used for the no-stub allowlist",
    )
    args = parser.parse_args(argv)
    repo_root = args.root or git_root()

    registered = debt_registered_files(repo_root, args.debt)
    findings = []
    # A change that registers its own debt in the same staged diff is allowed.
    if not args.full_tree:
        changes = staged_changes(repo_root)
        debt_lines = changes.get(args.debt, [])
        parent = os.path.dirname(args.debt)
        for _, added, text in debt_lines:
            if not added:
                continue
            for target in LINK.findall(text):
                if re.match(r"[a-z]+:|#", target):
                    continue
                target = target.split("#", 1)[0].split("?", 1)[0]
                if target:
                    registered.add(
                        os.path.normpath(os.path.join(parent, target))
                        if not target.startswith("/")
                        else os.path.normpath(target.lstrip("/"))
                    )
        findings = check_changes(changes, registered)
    else:
        findings = check_full_tree(repo_root, registered)

    for path, number, rule, detail in sorted(
        findings, key=lambda f: (f[0], f[1], f[2])
    ):
        print("%s:%d: [%s] %s" % (path, number, rule, detail))
    if findings:
        print("%d finding(s)" % len(findings))
        return 1
    print("anti_slop: clean")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
