import argparse
from collections import defaultdict
import json
from pathlib import Path
import re
import sys


CLASSES = {"implemented", "stub", "absent", "module"}
GAPS = {"stub", "absent"}
NID = re.compile(r"[A-Za-z0-9+\-]{11}")
COMMIT = re.compile(r"[0-9a-f]{40}")


def integer(value, minimum=0):
    return type(value) is int and value >= minimum


def read_audit(path):
    data = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(data, dict) or not isinstance(data.get("imports"), list):
        raise ValueError(str(path) + ": expected an import_audit JSON report")
    commit, dirty = data.get("source_commit"), data.get("source_dirty")
    if commit is not None and (not isinstance(commit, str) or not COMMIT.fullmatch(commit)):
        raise ValueError(str(path) + ": invalid source_commit")
    if dirty is not None and type(dirty) is not bool:
        raise ValueError(str(path) + ": invalid source_dirty")
    records = {}
    for entry in data["imports"]:
        if not isinstance(entry, dict):
            raise ValueError(str(path) + ": import must be an object")
        nid, library, kind = entry.get("nid"), entry.get("library"), entry.get("class")
        if not isinstance(nid, str) or not NID.fullmatch(nid) or not isinstance(library, str):
            raise ValueError(str(path) + ": invalid NID or library")
        if not isinstance(kind, str) or kind not in CLASSES or not integer(entry.get("references"), 1):
            raise ValueError(str(path) + ": invalid class or reference count")
        providers = entry.get("providers")
        if not isinstance(providers, list) or any(not isinstance(value, str) or not value for value in providers):
            raise ValueError(str(path) + ": invalid providers")
        if len(providers) != len(set(providers)) or (kind == "absent" and providers) or (kind in {"stub", "implemented"} and not providers):
            raise ValueError(str(path) + ": providers contradict classification")
        if type(entry.get("library_mismatch")) is not bool:
            raise ValueError(str(path) + ": invalid library_mismatch")
        if entry.get("name") is not None and not isinstance(entry["name"], str):
            raise ValueError(str(path) + ": invalid name")
        if "name_verified" in entry and type(entry["name_verified"]) is not bool:
            raise ValueError(str(path) + ": invalid name_verified")
        key = library, nid
        if key in records:
            raise ValueError(str(path) + ": duplicate library/NID import")
        records[key] = entry
    counts = {kind: sum(entry["class"] == kind for entry in records.values()) for kind in sorted(CLASSES)}
    references = {kind: sum(entry["references"] for entry in records.values() if entry["class"] == kind) for kind in sorted(CLASSES)}
    for field in ("unique_imports", "references", "library_mismatch"):
        if not integer(data.get(field)):
            raise ValueError(str(path) + ": invalid summary count")
    for field in ("unique_by_class", "references_by_class"):
        if not isinstance(data.get(field), dict) or any(not integer(value) for value in data[field].values()):
            raise ValueError(str(path) + ": invalid classification totals")
    if (data.get("unique_imports") != len(records) or data.get("references") != sum(references.values())
            or data.get("unique_by_class") != counts or data.get("references_by_class") != references
            or data.get("library_mismatch") != sum(entry["library_mismatch"] for entry in records.values())):
        raise ValueError(str(path) + ": audit summary does not conserve imports and references")
    missing = data.get("missing_libraries")
    if not isinstance(missing, dict) or any(not isinstance(name, str) or not name or not integer(count, 1) for name, count in missing.items()):
        raise ValueError(str(path) + ": invalid missing_libraries")
    return {"imports": records, "missing_libraries": missing, "source_commit": commit,
            "source_dirty": dirty, "path": str(path)}


def load_collection(manifest):
    manifest = Path(manifest).resolve()
    document = json.loads(manifest.read_text(encoding="utf-8-sig"))
    if not isinstance(document, dict) or type(document.get("version")) is not int or document["version"] != 1:
        raise ValueError("Expected collection manifest version 1")
    entries = document.get("titles")
    if not isinstance(entries, list) or not entries:
        raise ValueError("titles must be a nonempty list")
    titles, stamps, audit_paths = {}, set(), set()
    for entry in entries:
        if not isinstance(entry, dict) or set(entry) - {"id", "label", "audit"}:
            raise ValueError("Invalid title entry")
        name, audit = entry.get("id"), entry.get("audit")
        label = entry.get("label", name)
        if not isinstance(name, str) or not name or name in titles or not isinstance(label, str):
            raise ValueError("Title ids must be unique nonempty strings and labels must be strings")
        if not isinstance(audit, str) or not audit:
            raise ValueError("Each title needs one audit report containing all its registries")
        path = (manifest.parent / audit).resolve()
        if path in audit_paths:
            raise ValueError("An audit report cannot represent more than one title")
        audit_paths.add(path)
        title = read_audit(path)
        title["label"] = label
        titles[name] = title
        stamps.add((title["source_commit"], title["source_dirty"]))
    if len(stamps) != 1:
        raise ValueError("Collection audits must have the same source_commit and source_dirty stamp")
    return titles


def identifier(key):
    return {"library": key[0], "nid": key[1]}


def function_gaps(titles):
    return {name: {key for key, entry in title["imports"].items() if entry["class"] in GAPS}
            for name, title in titles.items()}


def make_plan(titles, budget=5):
    if not integer(budget):
        raise ValueError("budget must be a nonnegative integer")
    gaps = function_gaps(titles)
    candidates = {}
    for name, title in titles.items():
        for key in gaps[name]:
            entry = title["imports"][key]
            candidate = candidates.setdefault(key, {**identifier(key), "titles": set(), "absent_titles": set(),
                                                   "stub_titles": set(), "references": 0, "names": set(), "unverified_names": set()})
            candidate["titles"].add(name)
            candidate[entry["class"] + "_titles"].add(name)
            candidate["references"] += entry["references"]
            if entry.get("name"):
                field = "names" if entry.get("name_verified", True) else "unverified_names"
                candidate[field].add(entry["name"])
    for key, candidate in candidates.items():
        candidate["titles_with_no_other_function_gaps"] = sorted(name for name in candidate["titles"] if gaps[name] == {key})
        for field in ("titles", "absent_titles", "stub_titles", "names", "unverified_names"):
            candidate[field] = sorted(candidate[field])
        candidate["affected_titles"] = len(candidate["titles"])
    ranking = sorted(candidates, key=lambda key: (-len(candidates[key]["titles_with_no_other_function_gaps"]),
                     -candidates[key]["affected_titles"], -len(candidates[key]["absent_titles"]),
                     -candidates[key]["references"], key))
    remaining = {name: set(keys) for name, keys in gaps.items()}
    available, steps = set(candidates), []
    for index in range(min(budget, len(available))):
        selected = min(available, key=lambda key: (-sum(keys == {key} for keys in remaining.values()),
                       -sum(key in keys for keys in remaining.values()), -len(candidates[key]["absent_titles"]),
                       -candidates[key]["references"], key))
        newly_free = sorted(name for name, keys in remaining.items() if keys == {selected})
        for keys in remaining.values():
            keys.discard(selected)
        available.remove(selected)
        steps.append({"step": index + 1, **identifier(selected), "newly_function_gap_free_titles": newly_free,
                      "affected_titles": candidates[selected]["titles"]})
    bundles = defaultdict(list)
    for name, keys in gaps.items():
        if keys:
            bundles[tuple(sorted(keys))].append(name)
    completion_sets = [{"functions": [identifier(key) for key in keys], "function_count": len(keys),
                        "titles": sorted(names)} for keys, names in sorted(bundles.items(), key=lambda item: (len(item[0]), -len(item[1]), item[0]))]
    first = next(iter(titles.values()))
    return {"version": 1, "source_commit": first["source_commit"], "source_dirty": first["source_dirty"],
            "scope": "Reported import implementation gaps; not execution, rendering or compatibility evidence",
            "summary": {"titles": len(titles), "candidate_functions": len(candidates),
                        "already_function_gap_free": sorted(name for name, keys in gaps.items() if not keys)},
            "titles": [{"id": name, "label": title["label"], "audit": title["path"],
                        "function_gaps": len(gaps[name]), "missing_libraries": title["missing_libraries"],
                        "provider_mismatches": [identifier(key) for key, entry in sorted(title["imports"].items()) if entry["library_mismatch"]]}
                       for name, title in sorted(titles.items())],
            "ranking": [candidates[key] for key in ranking], "completion_sets": completion_sets,
            "plan": {"method": "greedy: complete remaining function sets, then maximize title coverage",
                     "budget": budget, "steps": steps,
                     "newly_function_gap_free_titles": sorted(name for name, keys in remaining.items() if not keys and gaps[name])}}


def compare_collections(before, after):
    changes = {kind: [] for kind in ("resolved", "regressions", "partial_progress", "new_gaps", "removed_imports",
                                   "new_missing_libraries", "new_provider_mismatches")}
    severity = {"module": 0, "implemented": 0, "stub": 1, "absent": 2}
    for name in sorted(set(before) & set(after)):
        old, new = before[name]["imports"], after[name]["imports"]
        for key in sorted(set(old) | set(new)):
            record = {"title": name, **identifier(key)}
            if key not in old:
                if new[key]["class"] in GAPS:
                    changes["new_gaps"].append({**record, "after": new[key]["class"]})
            elif key not in new:
                changes["removed_imports"].append({**record, "before": old[key]["class"]})
            elif old[key]["class"] != new[key]["class"]:
                previous, current = old[key]["class"], new[key]["class"]
                if severity[current] > severity[previous]:
                    kind = "regressions"
                elif severity[previous] > 0 and severity[current] == 0:
                    kind = "resolved"
                elif severity[current] < severity[previous]:
                    kind = "partial_progress"
                else:
                    continue
                changes[kind].append({**record, "before": previous, "after": current})
            if key in new and new[key]["library_mismatch"] and (key not in old or not old[key]["library_mismatch"]):
                changes["new_provider_mismatches"].append(record)
        for library in sorted(set(after[name]["missing_libraries"]) - set(before[name]["missing_libraries"])):
            changes["new_missing_libraries"].append({"title": name, "library": library})
    return {"before_source_commit": next(iter(before.values()))["source_commit"],
            "after_source_commit": next(iter(after.values()))["source_commit"],
            "added_titles": sorted(set(after) - set(before)), "removed_titles": sorted(set(before) - set(after)),
            **changes}


def render(report, limit):
    lines = [str(report["summary"]["titles"]) + " titles; " + str(report["summary"]["candidate_functions"]) + " candidate library/NID functions",
             report["scope"], "", "Rank  Titles  References  Library / NID / verified name"]
    for index, candidate in enumerate(report["ranking"][:limit], 1):
        names = ", ".join(candidate["names"])
        lines.append(f"{index:>4}  {candidate['affected_titles']:>6}  {candidate['references']:>10}  {candidate['library'] or '(unnamed library)'} / {candidate['nid']}" + (" / " + names if names else ""))
    lines.extend(["", "Suggested function budget: " + str(report["plan"]["budget"])])
    for step in report["plan"]["steps"]:
        lines.append(str(step["step"]) + ". " + (step["library"] or "(unnamed library)") + " / " + step["nid"]
                     + "; newly without function gaps: " + (", ".join(step["newly_function_gap_free_titles"]) or "none"))
    missing = sum(bool(title["missing_libraries"]) for title in report["titles"])
    mismatches = sum(bool(title["provider_mismatches"]) for title in report["titles"])
    lines.append(f"Separate deployment concerns: {missing} titles with missing libraries; {mismatches} with provider mismatches")
    if "comparison" in report:
        comparison = report["comparison"]
        lines.append(f"Comparison: {len(comparison['resolved'])} resolved, {len(comparison['regressions'])} regressed, {len(comparison['new_gaps'])} new function gaps")
    return "\n".join(lines)


def main(argv=None):
    parser = argparse.ArgumentParser(description="Rank library contributions across import audits and compare snapshots.")
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--budget", type=int, default=5)
    parser.add_argument("--limit", type=int, default=20)
    parser.add_argument("--baseline", type=Path)
    parser.add_argument("--fail-on-regression", action="store_true")
    parser.add_argument("--json", action="store_true", help="print the full machine-readable report")
    args = parser.parse_args(argv)
    try:
        if args.limit < 0 or (args.fail_on_regression and args.baseline is None):
            raise ValueError("limit must be nonnegative; --fail-on-regression requires --baseline")
        titles = load_collection(args.manifest)
        report = make_plan(titles, args.budget)
        if args.baseline:
            report["comparison"] = compare_collections(load_collection(args.baseline), titles)
    except (OSError, ValueError) as error:
        print("library_impact: " + str(error), file=sys.stderr)
        return 2
    print(json.dumps(report, ensure_ascii=False, sort_keys=True, indent=2) if args.json else render(report, args.limit))
    if args.fail_on_regression:
        comparison = report["comparison"]
        return int(any(comparison[kind] for kind in ("regressions", "new_gaps", "new_missing_libraries", "new_provider_mismatches")))
    return 0


if __name__ == "__main__":
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    raise SystemExit(main())
