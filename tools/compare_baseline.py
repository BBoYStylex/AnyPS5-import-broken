#!/usr/bin/env python3
"""Ratchet check for the NotImplemented stub baseline.

Compares the NotImplemented_nid_no_patch counts of one revision against
tools/debt_baseline.json and fails only when a per-library count grows
beyond the baseline. Shrinking counts never fail.
Usage: python3 tools/compare_baseline.py --base HEAD
"""

import argparse
import json
import os
import re
import subprocess

MARKER = "NotImplemented_nid_no_patch"
LIB_PREFIX = "core/libs/prx/"


def counts_at(root, rev):
    proc = subprocess.run(
        ["git", "grep", "-o", MARKER, rev, "--", LIB_PREFIX],
        capture_output=True, text=True, cwd=root,
    )
    if proc.returncode not in (0, 1):  # 1 means no matches, which is fine
        raise SystemExit("git grep failed: %s" % proc.stderr.strip())
    counts = {}
    for line in proc.stdout.splitlines():
        pos = line.find(LIB_PREFIX)
        if pos < 0:
            continue
        lib = line[pos + len(LIB_PREFIX):].split("/", 1)[0]
        counts[lib] = counts.get(lib, 0) + 1
    return counts


def main(argv=None):
    parser = argparse.ArgumentParser(description="Fail only when stub counts grow beyond the baseline.")
    parser.add_argument("--base", default="HEAD", help="revision to audit when --head is absent")
    parser.add_argument("--head", default=None, help="revision to audit (default: --base)")
    parser.add_argument("--baseline", default="tools/debt_baseline.json")
    parser.add_argument("--root", default=None)
    args = parser.parse_args(argv)
    root = args.root or subprocess.run(
        ["git", "rev-parse", "--show-toplevel"], capture_output=True,
        text=True, cwd=os.getcwd(), check=True).stdout.strip()
    rev = args.head or args.base
    with open(os.path.join(root, args.baseline), encoding="utf-8") as handle:
        baseline = json.load(handle)
    expected = baseline["not_implemented_per_library"]
    actual = counts_at(root, rev)
    grown = {lib: (expected.get(lib, 0), actual.get(lib, 0))
             for lib in set(expected) | set(actual)
             if actual.get(lib, 0) > expected.get(lib, 0)}
    for lib in sorted(grown):
        print("%s: %d -> %d (beyond baseline)" % (lib, grown[lib][0], grown[lib][1]))
    if grown:
        print("%d %s grew beyond the baseline" % (len(grown), "library" if len(grown) == 1 else "libraries"))
        return 1
    print("ratchet: clean at %s (total %d, baseline %d)"
          % (rev, sum(actual.values()), baseline["not_implemented_total"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
