import json
import os
import shutil
import subprocess
import tempfile
import textwrap
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import pr_overlap


class OpenPullRequestsTests(unittest.TestCase):
    def test_collects_pull_requests_beyond_the_previous_limit(self):
        pull_requests = [
            {"number": number, "user": {"login": f"author{number}"},
             "head": {"sha": f"sha{number}"}, "body": None}
            for number in range(1, 202)
        ]
        pull_requests[-1]["body"] = "Depends on: #12\n- #34\n"

        def response(*args):
            if args[0] == "api":
                return json.dumps(pull_requests)
            limit = int(args[args.index("-L") + 1])
            return json.dumps([
                {"number": pr["number"], "author": pr["user"], "headRefOid": pr["head"]["sha"],
                 "body": pr["body"]} for pr in pull_requests[:limit]
            ])

        with patch.object(pr_overlap, "REPO", "owner/project", create=True), \
                patch.object(pr_overlap, "gh", side_effect=response):
            found = pr_overlap.open_prs("main")
        self.assertEqual(set(found), set(range(1, 202)))
        self.assertEqual(found[201], {
            "ref": "refs/pr/201", "sha": "sha201", "author": "author201", "depends": {12, 34},
        })
        self.assertEqual(found[1]["depends"], set())

    def test_base_branch_is_encoded_in_the_paginated_request(self):
        with patch.object(pr_overlap, "REPO", "owner/project", create=True), \
                patch.object(pr_overlap, "gh", return_value="[]") as gh:
            self.assertEqual(pr_overlap.open_prs("topic/one&two"), {})
        gh.assert_called_once_with(
            "api", "--paginate",
            "repos/owner/project/pulls?state=open&base=topic%2Fone%26two&per_page=100",
        )

    def test_single_page_preserves_author_head_and_dependencies(self):
        pull_requests = [{"number": 7, "user": {"login": "contributor"}, "head": {"sha": "head-sha"},
                          "body": "Depends on: none\nUnrelated issue #42"}]
        with patch.object(pr_overlap, "REPO", "owner/project", create=True), \
                patch.object(pr_overlap, "gh", return_value=json.dumps(pull_requests)):
            found = pr_overlap.open_prs("main")
        self.assertEqual(found, {
            7: {"ref": "refs/pr/7", "sha": "head-sha", "author": "contributor", "depends": set()},
        })


class FetchTests(unittest.TestCase):
    def test_fetches_each_head_commit_the_api_reported(self):
        prs = {
            7: {"ref": "refs/pr/7", "sha": "a" * 40, "author": "one", "depends": set()},
            9: {"ref": "refs/pr/9", "sha": "b" * 40, "author": "two", "depends": set()},
        }
        with patch.object(pr_overlap, "REPO", "owner/project", create=True), \
                patch.object(pr_overlap, "git", return_value="") as git:
            pr_overlap.fetch("main", prs)
        git.assert_called_once_with(
            "fetch", "--no-tags", "-q", "https://github.com/owner/project.git",
            "+refs/heads/main:refs/pr/base", f"+{'a' * 40}:refs/pr/7", f"+{'b' * 40}:refs/pr/9",
        )


class OverlapWorkflowTests(unittest.TestCase):
    def run_reports(self, reports):
        bash = shutil.which("bash")
        if bash is None and os.name == "nt":
            candidate = Path(os.environ.get("ProgramFiles", "C:/Program Files")) / "Git/bin/bash.exe"
            if candidate.is_file():
                bash = str(candidate)
        if bash is None:
            self.skipTest("Bash is required to exercise the workflow")
        workflow = Path(__file__).resolve().parents[2] / ".github/workflows/pr-overlap.yml"
        text = workflow.read_text(encoding="utf-8")
        start = text.index("          marker=")
        script = textwrap.dedent(text[start:])
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "reports").mkdir()
            for name, content in reports.items():
                (root / "reports" / name).write_text(content, encoding="utf-8")
            setup = """set -euo pipefail
out=reports
GH_REPO=owner/project
gh() { printf '%s\n' "$*" >> calls; }
jq() { printf ''; }
"""
            (root / "run.sh").write_text(setup + script, encoding="utf-8", newline="\n")
            result = subprocess.run([bash, "run.sh"], cwd=root, capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            calls = root / "calls"
            return calls.read_text(encoding="utf-8").splitlines() if calls.exists() else []

    def test_empty_report_directory_makes_no_github_requests(self):
        self.assertEqual(self.run_reports({}), [])

    def test_non_markdown_files_do_not_become_pull_request_numbers(self):
        self.assertEqual(self.run_reports({"metadata.json": "{}"}), [])

    def test_real_report_still_reads_and_creates_its_comment(self):
        calls = self.run_reports({"42.md": "overlap report\n"})
        self.assertEqual(len(calls), 2)
        self.assertIn("repos/owner/project/issues/42/comments", calls[0])
        self.assertEqual(calls[1], "pr comment 42 --body-file reports/42.md")


if __name__ == "__main__":
    unittest.main()
