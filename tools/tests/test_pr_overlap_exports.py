import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import pr_overlap

TRAILING = "+extern \"C\" std::int64_t APS5_VABI recv_nid_postfix(int, void*, std::uint64_t, int) noexcept;   \n"
DIFF = """diff --git a/core/libs/prx/libkernel/File/src/Open.cpp b/core/libs/prx/libkernel/File/src/Open.cpp
@@ -96,0 +97,2 @@
+extern "C" int* APS5_VABI __error_nid_postfix();
""" + TRAILING + """@@ -135,0 +136,5 @@
+int APS5_VABI mlock_nid_postfix(const void* address, std::size_t length) {
+std::int64_t APS5_VABI sceKernelRead(int d, void* buf, std::size_t nbytes) noexcept {
+int APS5_VABI getpid_nid_postfix() { return 1; }
+int APS5_VABI sceNpUniversalDataSystemCreateEvent(
+void APS5_VABI Helper_nid_no_patch(int value) {
@@ -200 +205,0 @@
-int APS5_VABI removed_nid_postfix(int) {
diff --git a/core/libs/prx/libkernel/File/src/Stdio.cpp b/core/libs/prx/libkernel/File/src/Stdio.cpp
@@ -10,2 +10,2 @@
 int APS5_VABI context_nid_postfix(int) {
"""


class ExportTests(unittest.TestCase):
    def test_added_definitions_are_exports(self):
        self.assertEqual(pr_overlap.exports(DIFF), {"mlock", "sceKernelRead", "getpid", "sceNpUniversalDataSystemCreateEvent"})

    def test_nid_postfix_aliases_match_plain_export_names(self):
        suffixed = "+int APS5_VABI sceExample_nid_postfix() { return 0; }\n"
        plain = "+int APS5_VABI sceExample() { return 0; }\n"
        self.assertEqual(pr_overlap.exports(suffixed), pr_overlap.exports(plain))

    def test_nid_disambiguation_suffix_is_normalized(self):
        diff = "+int APS5_VABI sceExample_nid_disambig12_nid_postfix() { return 0; }\n"
        self.assertEqual(pr_overlap.exports(diff), {"sceExample"})

    def test_cut_helpers_are_not_export_overlaps(self):
        diff = DIFF + '+void APS5_VABI Helper_nid_no_patch_cut() {\n'
        self.assertEqual(pr_overlap.exports(diff), pr_overlap.exports(DIFF))

    def test_declarations_are_not_exports(self):
        self.assertTrue(TRAILING.endswith(";   \n"))
        found = pr_overlap.exports(DIFF)
        self.assertNotIn("__error_nid_postfix", found)
        self.assertNotIn("recv_nid_postfix", found)

    def test_removed_context_and_helper_lines_are_not_exports(self):
        found = pr_overlap.exports(DIFF)
        self.assertNotIn("removed_nid_postfix", found)
        self.assertNotIn("context_nid_postfix", found)
        self.assertNotIn("Helper_nid_no_patch", found)

    def test_explicit_aps5_export_nids_are_exports(self):
        diff = """diff --git a/core/libs/prx/libSceFreeTypeOt/src/Export.cpp b/core/libs/prx/libSceFreeTypeOt/src/Export.cpp
@@ -10,0 +11,4 @@
+int APS5_VABI placeholder_nid_no_patch() {
+    NotImplemented_nid_no_patch(__func__);
+}
+APS5_EXPORT("Pz262plj20w", placeholder_nid_no_patch);
+APS5_EXPORT("1m6DD9BBmSE", VrTracker2GetRelativePose);
+"""
        self.assertEqual(pr_overlap.exports(diff), {"Pz262plj20w", "1m6DD9BBmSE"})

    def test_explicit_alias_overlaps_with_named_export_sharing_nid_in_same_provider(self):
        diff_named = """diff --git a/core/libs/prx/libSceFreeTypeOt/src/Export.cpp b/core/libs/prx/libSceFreeTypeOt/src/Export.cpp
@@ -10,0 +11,3 @@
+int APS5_VABI FT_Outline_GetInsideBorder_nid_postfix() {
+    return 0;
+}
+"""
        diff_alias = """diff --git a/core/libs/prx/libSceFreeTypeOt/src/Export.cpp b/core/libs/prx/libSceFreeTypeOt/src/Export.cpp
@@ -10,0 +11,4 @@
+int APS5_VABI placeholder_nid_no_patch() {
+    NotImplemented_nid_no_patch(__func__);
+}
+APS5_EXPORT("Pz262plj20w", placeholder_nid_no_patch);
+"""
        prs = {
            1: {"author": "alice", "depends": set(), "added": set(), "tree": None,
                "exports": pr_overlap.exports(diff_named), "export_map": pr_overlap.scan_exports(diff_named)},
            2: {"author": "bob", "depends": set(), "added": set(), "tree": None,
                "exports": pr_overlap.exports(diff_alias), "export_map": pr_overlap.scan_exports(diff_alias)},
        }
        found = pr_overlap.overlaps(prs, 1, 2)
        self.assertIsNotNone(found)
        self.assertEqual(found["exports"], {"FT_Outline_GetInsideBorder"})

    def test_same_nid_in_different_providers_does_not_overlap(self):
        diff_named = """diff --git a/core/libs/prx/libSceFreeTypeOt/src/Export.cpp b/core/libs/prx/libSceFreeTypeOt/src/Export.cpp
@@ -10,0 +11,3 @@
+int APS5_VABI FT_Outline_GetInsideBorder_nid_postfix() {
+    return 0;
+}
+"""
        diff_other = """diff --git a/core/libs/prx/libSceAmpr/src/Export.cpp b/core/libs/prx/libSceAmpr/src/Export.cpp
@@ -10,0 +11,4 @@
+int APS5_VABI placeholder_nid_no_patch() {
+    NotImplemented_nid_no_patch(__func__);
+}
+APS5_EXPORT("Pz262plj20w", placeholder_nid_no_patch);
+"""
        prs = {
            1: {"author": "alice", "depends": set(), "added": set(), "tree": None,
                "exports": pr_overlap.exports(diff_named), "export_map": pr_overlap.scan_exports(diff_named)},
            2: {"author": "bob", "depends": set(), "added": set(), "tree": None,
                "exports": pr_overlap.exports(diff_other), "export_map": pr_overlap.scan_exports(diff_other)},
        }
        self.assertIsNone(pr_overlap.overlaps(prs, 1, 2))

    def test_unaliased_helper_functions_do_not_overlap(self):
        diff_helper1 = """diff --git a/core/libs/prx/libSceFreeTypeOt/src/Export.cpp b/core/libs/prx/libSceFreeTypeOt/src/Export.cpp
@@ -10,0 +11,3 @@
+int APS5_VABI placeholder_nid_no_patch() {
+    NotImplemented_nid_no_patch(__func__);
+}
+"""
        diff_helper2 = """diff --git a/core/libs/prx/libSceFreeTypeOt/src/Export.cpp b/core/libs/prx/libSceFreeTypeOt/src/Export.cpp
@@ -10,0 +11,3 @@
+int APS5_VABI other_helper_nid_no_patch() {
+    NotImplemented_nid_no_patch(__func__);
+}
+"""
        prs = {
            1: {"author": "alice", "depends": set(), "added": set(), "tree": None,
                "exports": pr_overlap.exports(diff_helper1), "export_map": pr_overlap.scan_exports(diff_helper1)},
            2: {"author": "bob", "depends": set(), "added": set(), "tree": None,
                "exports": pr_overlap.exports(diff_helper2), "export_map": pr_overlap.scan_exports(diff_helper2)},
        }
        self.assertIsNone(pr_overlap.overlaps(prs, 1, 2))

    def test_two_explicit_aliases_without_named_symbol_report_nid(self):
        diff_alias1 = """diff --git a/core/libs/prx/libSceFreeTypeOt/src/Export.cpp b/core/libs/prx/libSceFreeTypeOt/src/Export.cpp
@@ -10,0 +11,4 @@
+int APS5_VABI h1_nid_no_patch() { return 0; }
+APS5_EXPORT("Pz262plj20w", h1_nid_no_patch);
+"""
        diff_alias2 = """diff --git a/core/libs/prx/libSceFreeTypeOt/src/Export.cpp b/core/libs/prx/libSceFreeTypeOt/src/Export.cpp
@@ -10,0 +11,4 @@
+int APS5_VABI h2_nid_no_patch() { return 0; }
+APS5_EXPORT("Pz262plj20w", h2_nid_no_patch);
+"""
        prs = {
            1: {"author": "alice", "depends": set(), "added": set(), "tree": None,
                "exports": pr_overlap.exports(diff_alias1), "export_map": pr_overlap.scan_exports(diff_alias1)},
            2: {"author": "bob", "depends": set(), "added": set(), "tree": None,
                "exports": pr_overlap.exports(diff_alias2), "export_map": pr_overlap.scan_exports(diff_alias2)},
        }
        found = pr_overlap.overlaps(prs, 1, 2)
        self.assertIsNotNone(found)
        self.assertEqual(found["exports"], {"Pz262plj20w"})


if __name__ == "__main__":
    unittest.main()
