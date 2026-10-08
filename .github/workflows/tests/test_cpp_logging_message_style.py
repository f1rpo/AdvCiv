# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "LLM_Helpers"))
# <!-- custom: Import the repo-local logging audit after adding its helper directory, matching the workflow-test import convention. (GPT-6.1-Sol) -->
import audit_cpp_logging_pregates as audit  # noqa: E402


class MessageStyleTests(unittest.TestCase):
    def test_prose_and_indentation_are_separate_review_findings(self):
        text = 'logBBAI("Settler founding at %d,%d", x, y);\nlogBBAI("    SETTLER_FOUND site=%d,%d", x, y);'
        findings = audit.audit_message_style(text)
        self.assertEqual([(line, kind) for line, kind, _ in findings], [(1, "BBAI_UNSTRUCTURED_PREFIX"), (2, "BBAI_INDENTED_PREFIX")])

    def test_inactive_calls_and_unrelated_literals_are_ignored(self):
        text = '// logBBAI("Old prose");\n/* logBBAI("More old prose"); */\nconst char* text = "logBBAI(legacy)";\nlogBBAI("SETTLER_FOUND site=%d,%d", x, y);'
        self.assertEqual(audit.audit_message_style(text), [])

    def test_multiline_calls_are_checked_without_interpreting_dynamic_formats(self):
        text = 'logBBAI(\n"Settler travelling", x);\nlogBBAI(format, x);\nlogSASGameRecord("Other prose", x);'
        self.assertEqual([(line, kind) for line, kind, _ in audit.audit_message_style(text)], [(1, "BBAI_UNSTRUCTURED_PREFIX")])


if __name__ == "__main__":
    unittest.main()
