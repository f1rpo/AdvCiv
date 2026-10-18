# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: pathlib/tempfile isolate disposable Git repositories, subprocess creates real commit ranges, sys locates the checked helper, and unittest verifies accepted and rejected histories. (GPT-6.1-Sol) -->
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[4] / ".github/workflows/build/diagnostics"))
# <!-- custom: Exercise the production revision checker rather than a separate model of its progression rules. (GPT-6.1-Sol) -->
import runtime_revision_history as history


class RuntimeRevisionTests(unittest.TestCase):
    def git(self, *args):
        return subprocess.check_output(["git", *args], cwd=self.repo, stderr=subprocess.DEVNULL, text=True).strip()

    def write(self, path, text):
        target = self.repo / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text, encoding="utf-8")

    def commit(self):
        self.git("add", ".")
        self.git("commit", "-qm", "fixture")
        return self.git("rev-parse", "HEAD")

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.repo = Path(self.temporary.name)
        self.git("init", "-q")
        for key, value in (("user.name", "CI fixture"), ("user.email", "fixture@example.invalid"), ("commit.gpgsign", "false"), ("core.autocrlf", "false")):
            self.git("config", key, value)
        self.write("README.md", "baseline\n")
        self.base = self.commit()
        self.git("branch", "-f", "main", self.base)
        self.record(1)
        self.introduction = self.commit()

    def record(self, value, comment=""):
        self.write(history.SOURCE, f"enum {{ SAS_RUNTIME_REVISION = {value} }};\n// {comment}\n")
        self.write(history.HISTORY, "\n".join(f"### Revision {number}\n\n- **Change:** Fixture behavior.\n" for number in range(value, 0, -1)))

    def check_push(self, tip):
        return history.check(self.repo, "main", "push", {"before": self.base, "after": tip})

    def test_introduction_and_documentation_only_commit(self):
        self.assertEqual(self.check_push(self.introduction), [])
        self.write("README.md", "updated documentation\n")
        self.assertEqual(self.check_push(self.commit()), [])

    def test_multiple_commits_and_single_pending_draft_increment(self):
        self.record(2, "minor implementation comment")
        second = self.commit()
        self.record(3, "pending implementation")
        self.assertEqual(self.check_push(second), [])
        self.assertEqual(history.transition_errors(self.repo, "HEAD", None), [])

    def test_clean_committed_crlf_source_does_not_require_an_increment(self):
        target = self.repo / history.SOURCE
        target.write_bytes(target.read_text(encoding="utf-8").replace("\n", "\r\n").encode("utf-8"))
        self.git("add", ".")
        self.git("commit", "--amend", "--no-edit", "-q")
        self.assertEqual(self.check_push(self.git("rev-parse", "HEAD")), [])

    def test_missing_increment_in_earlier_push_commit_is_not_hidden(self):
        self.record(1, "changed without revision")
        bad = self.commit()
        self.record(2, "later corrected label")
        errors = self.check_push(self.commit())
        self.assertTrue(any(bad[:12] in error and "expected 2" in error for error in errors), errors)

    def test_jumps_and_regressions_fail(self):
        self.record(3)
        self.assertTrue(any("expected 2" in error for error in self.check_push(self.commit())))
        self.record(2)
        self.assertTrue(any("3->2" in error for error in self.check_push(self.commit())))

    def test_history_mismatch_and_duplicate_entries_fail(self):
        self.record(2)
        self.write(history.HISTORY, "### Revision 1\n")
        self.assertTrue(any("history" in error for error in history.transition_errors(self.repo, "HEAD", None)))
        self.write(history.HISTORY, "### Revision 2\n### Revision 1\n### Revision 1\n")
        self.assertTrue(any("history" in error for error in history.transition_errors(self.repo, "HEAD", None)))

    def test_missing_change_and_metadata_only_entries_fail(self):
        source = "enum { SAS_RUNTIME_REVISION = 1 };"
        for content in ('', '- **Change:** ', '- **Change:** <!-- explanation later -->'):
            with self.subTest(content=content):
                text = '### Revision 1\n- **Date:** 2026-10-19\n- **Git commit:** pending\n' + content
                self.assertTrue(any('nonempty Change' in error for error in history.history_errors(source, text)))

    def test_later_documentation_does_not_hide_missing_committed_blurb(self):
        self.record(2, 'new behavior')
        text = (self.repo / history.HISTORY).read_text()
        self.write(history.HISTORY, text.replace('- **Change:** Fixture behavior.', '- **Change:** ', 1))
        bad = self.commit()
        self.record(2, 'new behavior')
        errors = self.check_push(self.commit())
        self.assertTrue(any(bad[:12] in error and 'nonempty Change' in error for error in errors), errors)

    def test_shared_hook_changes_qualify_but_test_only_changes_do_not(self):
        self.write("CvGameCoreDLL/CvGame.cpp", "updateSASRuntimeStatusTurnCompleted();\n")
        self.assertTrue(any("expected 2" in error for error in history.transition_errors(self.repo, "HEAD", None)))
        self.record(2)
        self.assertEqual(self.check_push(self.commit()), [])
        self.write("tests/diagnostics/fixture.cpp", "SASRuntime test fixture\n")
        self.assertEqual(self.check_push(self.commit()), [])

    def test_checker_only_changes_keep_the_compiled_revision(self):
        for name in ("runtime_status.py", "runtime_revision_history.py"):
            self.write(".github/workflows/build/diagnostics/" + name, "# SAS_RUNTIME_REVISION checker maintenance\n")
        self.assertEqual(self.check_push(self.commit()), [])
        self.write(".github/workflows/build/diagnostics/runtime_revision_history.py", "# Changed SASRuntime policy explanation\n")
        self.assertEqual(self.check_push(self.commit()), [])


if __name__ == "__main__":
    unittest.main()
