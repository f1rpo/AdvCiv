# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Protect incremental prose enforcement: untouched historical lines remain valid, changed prose is checked in full document context, and the CLI fails without rewriting files. (GPT-6.1-Sol) -->
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / ".github/workflows/build"))
import markdown_prose_changes as checker

LONG = "First sentence explains the source documentation contract in substantial detail " + "carefully " * 30 + ". Another independent sentence describes the next part in enough detail to justify its own paragraph and make the intended responsibility clear."


class MarkdownProseChangesTests(unittest.TestCase):
    def test_new_and_changed_prose_fails_but_untouched_history_passes(self):
        self.assertTrue(checker.findings("README.md", "", LONG + "\n"))
        self.assertTrue(checker.findings("README.md", LONG + "\n", LONG.replace("First", "Updated") + "\n"))
        self.assertEqual(checker.findings("README.md", LONG + "\n", LONG + "\n\nShort addition.\n"), [])

    def test_full_fence_context_and_html_comments_are_preserved(self):
        for opener, closer in (("```text", "```"), ("~~~text", "~~~"), ("<!--", "-->")):
            with self.subTest(opener=opener):
                before = opener + "\nold content\n" + closer + "\n"
                after = opener + "\n" + LONG + "\n" + closer + "\n"
                self.assertEqual(checker.findings("README.md", before, after), [])

    def test_short_follow_up_or_heading_stays_with_the_main_sentence(self):
        long_sentence = "A substantial sentence " + "word " * 90 + "."
        for body in (long_sentence + " Brief follow-up.", "Short heading. " + long_sentence):
            self.assertEqual(checker.findings("README.md", "", body), [])

    def test_html_comment_literal_inside_code_does_not_hide_later_prose(self):
        source = '```text\n<!--\n```\n\n' + LONG + '\n'
        self.assertTrue(checker.findings("README.md", "", source))

    def test_single_long_sentence_and_structured_content_pass(self):
        for text in ("One sentence " + "continued " * 60 + ".", "| " + LONG + " | data |", "    " + LONG, LONG + "\\"):
            with self.subTest(text=text):
                self.assertEqual(checker.findings("README.md", "", text + "\n"), [])

    def test_reflowed_list_and_abbreviations_preserve_content(self):
        source = "- " + LONG + " Keep e.g. this example and version 1.14 intact.\n"
        updated, _changed, _breaks = checker.prose.transform_text(source, "\n", 400)
        self.assertIn("e.g. this example", updated)
        self.assertIn("version 1.14", updated)
        self.assertEqual(" ".join(source.split()), " ".join(updated.split()))
        self.assertTrue(checker.findings("README.md", "", updated))
        paragraphs = "\n\n".join(checker.prose.split_prose_line(source.rstrip(), 400)) + "\n"
        self.assertEqual(checker.findings("README.md", "", paragraphs), [])

    def test_short_prose_stays_together_and_soft_wrapping_cannot_bypass(self):
        self.assertEqual(checker.findings("README.md", "", "My A is B. My C is D.\n"), [])
        wrapped = LONG.replace(". Another", ".\nAnother")
        self.assertTrue(checker.findings("README.md", "", wrapped))
        self.assertEqual(checker.findings("README.md", "", wrapped.replace(".\nAnother", ".\n\nAnother")), [])

    def test_threshold_is_400_characters_and_list_items_separate_blocks(self):
        short = "A" * 190 + ". " + "B" * 190 + "."
        self.assertLess(len(short), 400)
        self.assertEqual(checker.findings("README.md", "", short), [])
        self.assertTrue(checker.findings("README.md", "", short + " More explanatory detail here."))
        children = "- Parent.\n  - " + short + "\n  - Another child.\n"
        self.assertEqual(checker.findings("README.md", "", children), [])

    def test_reference_scope_and_new_markdown_locations(self):
        for path in ("_0_Common_Docs/copied.md", "LLM_Helpers/context/copied.md", "_1_AdvCiv-SAS/Docs/changelogs_web/6600.md"):
            self.assertEqual(checker.findings(path, "", LONG), [])
        self.assertTrue(checker.findings("NewFolder/new.md", "", LONG))

    def test_git_cli_exit_codes_and_read_only_behavior(self):
        with tempfile.TemporaryDirectory(prefix="sas-md-prose-") as folder:
            repo = Path(folder)
            subprocess.run(["git", "init", "-q", str(repo)], check=True)
            source = repo / "README.md"
            source.write_text("Baseline.\n", encoding="utf-8")
            subprocess.run(["git", "-C", str(repo), "add", "."], check=True)
            subprocess.run(["git", "-C", str(repo), "-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid", "-c", "commit.gpgsign=false", "commit", "-qm", "Baseline"], check=True)
            source.write_text(LONG + "\n", encoding="utf-8")
            original = source.read_bytes()
            command = [sys.executable, str(Path(checker.__file__).resolve()), "--repo-root", str(repo), "--base-ref", "HEAD"]
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
            self.assertIn("README.md:1:", result.stdout)
            self.assertEqual(source.read_bytes(), original)
            updated = "\n\n".join(checker.prose.split_prose_line(LONG, 400)) + "\n"
            source.write_text(updated, encoding="utf-8")
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
