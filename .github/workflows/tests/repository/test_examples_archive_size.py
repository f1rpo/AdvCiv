# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Exercise tracked-file selection and real Git attributes in isolated temporary repositories; small custom thresholds keep aggregate/exclusion fixtures cheap.
# pathlib locates the checker, subprocess sets up Git fixtures, tempfile contains them, and unittest checks the review failures without changing attributes automatically. (GPT-6.1-Sol) -->
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

# <!-- custom: Import the same repository helper used by checks-repository.yml, without requiring Civ4 or a game DLL. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "build" / "repository"))
import examples_archive_size as checker


class ExampleArchiveSizeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.repo = Path(self.temp.name)
        subprocess.run(["git", "init", "-q", str(self.repo)], check=True)

    def example(self, relative, size):
        path = self.repo / str(checker.EXAMPLES) / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"x" * size)
        subprocess.run(["git", "add", "--", str(path.relative_to(self.repo))], cwd=self.repo, check=True)
        return path

    def attributes(self, text):
        path = self.repo / ".gitattributes"
        path.write_text(text, encoding="utf-8")
        subprocess.run(["git", "add", ".gitattributes"], cwd=self.repo, check=True)
        return path

    def test_decimal_mb_boundary_and_untracked_files(self):
        path = self.example("small.log", checker.MAX_BYTES)
        (path.parent / "untracked.log").write_bytes(b"x" * (checker.MAX_BYTES + 1))
        self.assertEqual(checker.check(self.repo), [])
        path.write_bytes(b"x" * (checker.MAX_BYTES + 1))
        errors = checker.check(self.repo)
        self.assertTrue(any("small.log:" in error for error in errors))
        self.assertTrue(any("examples/:" in error for error in errors))

    def test_small_files_can_make_an_oversized_folder(self):
        self.example("family/one.log", 6)
        self.example("family/two.log", 6)
        errors = checker.check(self.repo, 10)
        self.assertTrue(any("family/:" in error and "12" in error for error in errors))
        self.assertTrue(any("examples/:" in error for error in errors))
        self.assertFalse(any(".log:" in error for error in errors))

    def test_excluded_files_do_not_inflate_included_folders(self):
        self.example("heavy.log", 40)
        self.example("compact.log", 5)
        attrs = self.attributes(f"/{checker.EXAMPLES}/heavy.log export-ignore\n")
        before = attrs.read_bytes()
        self.assertEqual(checker.check(self.repo, 10), [])
        self.assertEqual(attrs.read_bytes(), before)

    def test_parent_folder_exclusion_prunes_children(self):
        self.example("heavy/one.log", 40)
        self.example("compact.log", 5)
        self.attributes(f"/{checker.EXAMPLES}/heavy export-ignore\n")
        self.assertEqual(checker.check(self.repo, 10), [])
        self.attributes("/_1_AdvCiv-SAS/Docs export-ignore\n")
        self.assertEqual(checker.check(self.repo, 1), [])

    def test_git_patterns_and_later_unset_are_respected(self):
        self.example("heavy.log", 40)
        self.attributes(f"/{checker.EXAMPLES}/*.log export-ignore\n")
        self.assertEqual(checker.check(self.repo, 10), [])
        self.attributes(f"/{checker.EXAMPLES}/*.log export-ignore\n/{checker.EXAMPLES}/heavy.log -export-ignore\n")
        self.assertTrue(checker.check(self.repo, 10))

    def test_all_oversized_files_are_reported_together(self):
        self.example("one.log", 20)
        self.example("two.log", 30)
        errors = checker.check(self.repo, 10)
        self.assertTrue(any("one.log:" in error for error in errors))
        self.assertTrue(any("two.log:" in error for error in errors))

    def test_missing_tracked_file_fails(self):
        self.example("gone.log", 5).unlink()
        self.assertTrue(any("cannot measure tracked example" in error for error in checker.check(self.repo)))

    def test_cli_failure_requires_manual_review_without_editing_attributes(self):
        self.example("large.log", checker.MAX_BYTES + 1)
        result = subprocess.run([sys.executable, checker.__file__, "--repo-root", str(self.repo)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("large.log:", result.stdout)
        self.assertIn("manually review", result.stdout)
        self.assertFalse((self.repo / ".gitattributes").exists())


if __name__ == "__main__":
    unittest.main()
