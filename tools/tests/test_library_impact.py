import contextlib
import copy
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import library_impact as impact
import test_import_audit as fixtures


A, B, C = "AAAAAAAAAAA", "BBBBBBBBBBB", "CCCCCCCCCCC"


def entry(nid, kind="absent", library="libExample.prx", references=1, **extra):
    value = {"nid": nid, "library": library, "references": references, "class": kind,
             "providers": [library or "libExample.prx"] if kind in {"implemented", "stub"} else [],
             "library_mismatch": False, "name": None}
    value.update(extra)
    return value


def audit(entries, **extra):
    value = {"source_commit": "a" * 40, "source_dirty": False, "registries": [],
             "imports": entries, "references": sum(e["references"] for e in entries),
             "unique_imports": len(entries), "missing_libraries": {},
             "unique_by_class": {kind: sum(e["class"] == kind for e in entries) for kind in impact.CLASSES},
             "references_by_class": {kind: sum(e["references"] for e in entries if e["class"] == kind) for kind in impact.CLASSES},
             "library_mismatch": sum(e["library_mismatch"] for e in entries)}
    value.update(extra)
    return value


class ImpactTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.manifest = self.root / "titles.json"
        self.documents = {"alpha": audit([entry(A, references=20), entry(B, "stub", references=3)]),
                          "beta": audit([entry(A, references=2)]),
                          "gamma": audit([entry(A, "implemented"), entry(C, "module")])}
        self.save()

    def save(self):
        entries = []
        for name, document in self.documents.items():
            (self.root / (name + ".json")).write_text(json.dumps(document), encoding="utf-8")
            entries.append({"id": name, "label": name.title(), "audit": name + ".json"})
        self.manifest.write_text(json.dumps({"version": 1, "titles": entries}), encoding="utf-8")

    def collection(self):
        return impact.load_collection(self.manifest)

    def invoke(self, *args):
        output, error = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(output), contextlib.redirect_stderr(error):
            code = impact.main([str(self.manifest), *args])
        return code, output.getvalue(), error.getvalue()

    def test_ranks_distinct_titles_and_preserves_reference_counts(self):
        result = impact.make_plan(self.collection(), budget=2)
        self.assertEqual([value["nid"] for value in result["ranking"]], [A, B])
        self.assertEqual(result["ranking"][0]["affected_titles"], 2)
        self.assertEqual(result["ranking"][0]["references"], 22)
        self.assertEqual(result["ranking"][0]["titles_with_no_other_function_gaps"], ["beta"])
        self.assertEqual(result["summary"]["already_function_gap_free"], ["gamma"])

    def test_greedy_plan_and_completion_sets(self):
        result = impact.make_plan(self.collection(), budget=2)
        self.assertEqual(result["plan"]["steps"][0]["newly_function_gap_free_titles"], ["beta"])
        self.assertEqual(result["plan"]["steps"][1]["newly_function_gap_free_titles"], ["alpha"])
        self.assertEqual(result["plan"]["newly_function_gap_free_titles"], ["alpha", "beta"])
        self.assertEqual([value["function_count"] for value in result["completion_sets"]], [1, 2])

    def test_zero_budget_and_already_complete_collection(self):
        result = impact.make_plan(self.collection(), budget=0)
        self.assertEqual(result["plan"]["steps"], [])
        self.documents = {"complete": audit([])}
        self.save()
        result = impact.make_plan(self.collection())
        self.assertEqual(result["ranking"], [])
        self.assertEqual(result["summary"]["already_function_gap_free"], ["complete"])

    def test_deployment_concerns_do_not_disappear_when_functions_are_selected(self):
        self.documents["beta"] = audit([entry(A), entry(C, "implemented", library_mismatch=True)], missing_libraries={"other.prx": 1})
        self.save()
        result = impact.make_plan(self.collection(), budget=2)
        beta = next(value for value in result["titles"] if value["id"] == "beta")
        self.assertEqual(beta["missing_libraries"], {"other.prx": 1})
        self.assertEqual(beta["provider_mismatches"], [{"library": "libExample.prx", "nid": C}])

    def test_same_nid_in_different_libraries_remains_separate(self):
        self.documents["delta"] = audit([entry(A, library="libOther.prx"), entry(A, library="")])
        self.save()
        result = impact.make_plan(self.collection())
        self.assertEqual(result["summary"]["candidate_functions"], 4)

    def test_unverified_names_are_not_presented_as_verified(self):
        self.documents["beta"] = audit([entry(A, name="untrustedName", name_verified=False)])
        self.save()
        result = impact.make_plan(self.collection())
        self.assertEqual(result["ranking"][0]["names"], [])
        self.assertEqual(result["ranking"][0]["unverified_names"], ["untrustedName"])
        self.assertNotIn("untrustedName", impact.render(result, 10))

    def test_ranking_is_deterministic_under_title_and_record_reordering(self):
        original = impact.make_plan(self.collection())
        self.documents = dict(reversed(list(self.documents.items())))
        for document in self.documents.values():
            document["imports"].reverse()
        self.save()
        self.assertEqual(impact.make_plan(self.collection()), original)

    def test_invalid_report_fields_are_rejected(self):
        original = copy.deepcopy(self.documents["beta"])
        changes = [{"nid": "bad"}, {"class": "unknown"}, {"references": True},
                   {"references": -1}, {"providers": ["contradiction.prx"]},
                   {"library_mismatch": 1}, {"name_verified": "false"}]
        for change in changes:
            with self.subTest(change=change):
                self.documents["beta"] = copy.deepcopy(original)
                self.documents["beta"]["imports"][0].update(change)
                self.save()
                with self.assertRaises(ValueError):
                    self.collection()

    def test_duplicate_imports_and_inconsistent_totals_are_rejected(self):
        self.documents["beta"]["imports"].append(entry(A))
        self.save()
        with self.assertRaises(ValueError):
            self.collection()
        self.documents["beta"] = audit([entry(A)], references=50)
        self.save()
        with self.assertRaises(ValueError):
            self.collection()

    def test_different_source_stamps_are_rejected(self):
        for field, value in (("source_commit", "b" * 40), ("source_dirty", True)):
            original = self.documents["beta"][field]
            self.documents["beta"][field] = value
            self.save()
            with self.assertRaisesRegex(ValueError, "same source_commit"):
                self.collection()
            self.documents["beta"][field] = original

    def test_duplicate_title_or_audit_path_is_rejected(self):
        document = json.loads(self.manifest.read_text(encoding="utf-8"))
        document["titles"][1]["audit"] = "alpha.json"
        self.manifest.write_text(json.dumps(document), encoding="utf-8")
        with self.assertRaises(ValueError):
            self.collection()
        document["titles"][1]["audit"] = "beta.json"
        document["titles"][1]["id"] = "alpha"
        self.manifest.write_text(json.dumps(document), encoding="utf-8")
        with self.assertRaises(ValueError):
            self.collection()

    def test_comparison_distinguishes_resolution_regression_and_partial_progress(self):
        before = self.collection()
        self.documents["alpha"] = audit([entry(A, "stub"), entry(B, "implemented")])
        self.documents["beta"] = audit([entry(A, "implemented")])
        self.documents["gamma"] = audit([entry(A, "absent"), entry(C, "module")])
        self.save()
        result = impact.compare_collections(before, self.collection())
        self.assertEqual(len(result["resolved"]), 2)
        self.assertEqual(len(result["partial_progress"]), 1)
        self.assertEqual(len(result["regressions"]), 1)

    def test_removed_import_is_not_counted_as_an_implemented_function(self):
        before = self.collection()
        self.documents["beta"] = audit([])
        self.save()
        result = impact.compare_collections(before, self.collection())
        self.assertEqual(result["resolved"], [])
        self.assertEqual(len(result["removed_imports"]), 1)

    def test_added_titles_are_reported_without_comparing_unrelated_titles(self):
        before = self.collection()
        del self.documents["beta"]
        self.documents["delta"] = audit([entry(C)])
        self.save()
        result = impact.compare_collections(before, self.collection())
        self.assertEqual(result["added_titles"], ["delta"])
        self.assertEqual(result["removed_titles"], ["beta"])
        self.assertEqual(result["new_gaps"], [])

    def test_json_and_regression_exit_codes(self):
        baseline = self.root / "baseline"
        baseline.mkdir()
        for name in self.documents:
            (baseline / (name + ".json")).write_text(json.dumps(self.documents[name]), encoding="utf-8")
        (baseline / "titles.json").write_text(self.manifest.read_text(encoding="utf-8"), encoding="utf-8")
        code, output, error = self.invoke("--json", "--baseline", str(baseline / "titles.json"), "--fail-on-regression")
        self.assertEqual((code, error), (0, ""))
        self.assertIn("comparison", json.loads(output))
        self.documents["gamma"] = audit([entry(A), entry(C, "module")])
        self.save()
        self.assertEqual(self.invoke("--baseline", str(baseline / "titles.json"), "--fail-on-regression")[0], 1)
        self.assertEqual(self.invoke("--fail-on-regression")[0], 2)
        self.assertEqual(self.invoke("--budget", "-1")[0], 2)

    def test_new_gaps_and_deployment_regressions(self):
        before = self.collection()
        self.documents["gamma"] = audit([entry(A, "implemented", library_mismatch=True), entry(C, "module"), entry(B)], missing_libraries={"missing.prx": 1})
        self.save()
        result = impact.compare_collections(before, self.collection())
        self.assertEqual(len(result["new_gaps"]), 1)
        self.assertEqual(len(result["new_missing_libraries"]), 1)
        self.assertEqual(len(result["new_provider_mismatches"]), 1)

    def test_real_import_audit_cli_output_is_consumed(self):
        work = fixtures.Workspace()
        self.addCleanup(work.close)
        (work.libs / "libExample.prx").write_bytes(fixtures.exporting(B))
        registry = work.root / "homebrew.registry.json"
        fixtures.write_registry(registry, [(A, "libExample.prx"), (B, "libExample.prx")])
        report = self.root / "actual.json"
        result = subprocess.run([sys.executable, str(Path(impact.__file__).with_name("import_audit.py")), str(registry),
                                 "--libs", str(work.libs), "--source", str(work.source), "--json", str(report)],
                                capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 1, result.stderr)
        self.manifest.write_text(json.dumps({"version": 1, "titles": [{"id": "actual", "audit": report.name}]}), encoding="utf-8")
        result = impact.make_plan(self.collection())
        self.assertEqual([value["nid"] for value in result["ranking"]], [A])


if __name__ == "__main__":
    unittest.main()
