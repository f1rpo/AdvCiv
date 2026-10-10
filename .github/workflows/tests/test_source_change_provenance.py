# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: These fixtures protect the diff-aware source-history guard itself: reflowed legacy prose must remain legal, newly authored prose must be explicitly custom, missing model credits remain advisory for user comments, dependency edits need rationale, and changed non-SAS interfaces need nearby explanation without forcing comments on wholly new files. (ChatGPT-6-Sol) -->

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / ".github/workflows/build"))
import source_change_provenance as checker


class SourceChangeProvenanceTests(unittest.TestCase):
    # <!-- custom: Exercise the actual Git-backed CLI as well as parser fixtures: CI must fail on provenance violations but keep missing model credits advisory for user-authored comments. (GPT-6.1-Sol) -->
    def test_cli_failure_and_warning_exit_codes(self):
        with tempfile.TemporaryDirectory(prefix="sas-provenance-check-") as folder:
            repo = Path(folder)
            subprocess.run(["git", "init", "-q", str(repo)], check=True)
            source = repo / "CvGameCoreDLL" / "Test.cpp"
            source.parent.mkdir()
            source.write_text("int value = 1;\n", encoding="utf-8")
            subprocess.run(["git", "-C", str(repo), "add", "."], check=True)
            subprocess.run(["git", "-C", str(repo), "-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid", "-c", "commit.gpgsign=false", "commit", "-qm", "Baseline"], check=True)
            cases = [
                ("// <!-- custom: User explanation. -->\n", 0, True),
                ("// <!-- custom: Explain the rule. (GPT-6.1-Sol) -->\n", 0, False),
                ("// Newly authored explanation.\n", 1, False),
                ('#include "Changed.h"\n', 1, False),
            ]
            for prefix, status, warning in cases:
                with self.subTest(prefix=prefix):
                    source.write_text(prefix + "int value = 1;\n", encoding="utf-8")
                    result = subprocess.run([sys.executable, str(Path(checker.__file__).resolve()), "--repo-root", str(repo), "--base-ref", "HEAD"], capture_output=True, text=True)
                    self.assertEqual(result.returncode, status, result.stdout + result.stderr)
                    self.assertEqual("WARNING:" in result.stdout, warning)

    def errors(self, path, old, new):
        return checker.check_change(path, old, new)

    def test_new_noncustom_comment_fails_but_credentialed_custom_passes(self):
        old = "int value = 1;\n"
        bad = "// New explanation.\nint value = 1;\n"
        good = "// <!-- custom: Explain the new rule. (ChatGPT-5.6-Sol) -->\nint value = 1;\n"
        self.assertTrue(self.errors("CvGameCoreDLL/Test.cpp", old, bad))
        self.assertEqual(self.errors("CvGameCoreDLL/Test.cpp", old, good), [])

    def test_uncredited_user_comment_warns_without_failing(self):
        old = "int value = 1;\n"
        new = "// <!-- custom: Explain the new rule. -->\nint value = 1;\n"
        warnings = []
        self.assertEqual(checker.check_change("CvGameCoreDLL/Test.cpp", old, new, warnings), [])
        self.assertTrue(any("has no recognized model credit" in warning for warning in warnings))

    def test_legacy_comment_reflow_is_allowed(self):
        old = "// Inherited sentence that was wrapped\n// across two physical lines.\nint value = 1;\n"
        new = "// Inherited sentence that was wrapped across two physical lines.\nint value = 1;\n"
        self.assertEqual(self.errors("CvGameCoreDLL/Test.cpp", old, new), [])

    def test_legacy_comment_wording_change_fails(self):
        old = "// Inherited behavior is enabled.\nint value = 1;\n"
        new = "// Inherited behavior is disabled.\nint value = 1;\n"
        errors = self.errors("CvGameCoreDLL/Test.cpp", old, new)
        self.assertTrue(any("new/rewritten non-custom comment" in error for error in errors))

    def test_stale_legacy_comment_can_be_removed(self):
        old = "// Stale inherited description.\nint value = 1;\n"
        new = "int value = 1;\n"
        self.assertEqual(self.errors("CvGameCoreDLL/Test.cpp", old, new), [])

    def test_replacement_custom_comment_preserves_advc_reference(self):
        old = "// advc.130: Old inherited explanation.\nint value = 1;\n"
        bad = "// <!-- custom: Replace the stale explanation. (GPT-5.6-Sol) -->\nint value = 1;\n"
        good = "// <!-- custom: Replace the stale explanation while retaining its provenance. advc.130 (GPT-5.6-Sol) -->\nint value = 1;\n"
        self.assertTrue(any("dropped provenance" in error for error in self.errors("CvGameCoreDLL/Test.cpp", old, bad)))
        self.assertEqual(self.errors("CvGameCoreDLL/Test.cpp", old, good), [])

    def test_cpp_include_change_requires_nearby_new_custom_rationale(self):
        old = '#include "Old.h"\nint value;\n'
        bad = '#include "New.h"\nint value;\n'
        good = '// <!-- custom: Replace Old.h with New.h because the used declaration now lives there. (Codex) -->\n#include "New.h"\nint value;\n'
        self.assertTrue(any("include/import change" in error for error in self.errors("CvGameCoreDLL/Test.cpp", old, bad)))
        self.assertEqual(self.errors("CvGameCoreDLL/Test.cpp", old, good), [])

    def test_removed_python_import_requires_rationale(self):
        old = "import old_module\nvalue = 1\n"
        bad = "value = 1\n"
        good = "# <!-- custom: old_module is no longer used after inlining the conversion. (ChatGPT-5.6-Sol) -->\nvalue = 1\n"
        self.assertTrue(any("include/import change" in error for error in self.errors("LLM_Helpers/test.py", old, bad)))
        self.assertEqual(self.errors("LLM_Helpers/test.py", old, good), [])

    def test_parameter_change_requires_nearby_custom_comment(self):
        old = "int Worker::score(int iValue)\n{\n\treturn iValue;\n}\n"
        bad = "int Worker::score(int iValue, bool bFast)\n{\n\treturn iValue;\n}\n"
        good = "// <!-- custom: Add bFast so callers can request the cheap scoring path without duplicating the calculation. (GPT-5.6-Sol) -->\nint Worker::score(int iValue, bool bFast)\n{\n\treturn iValue;\n}\n"
        errors = self.errors("CvGameCoreDLL/Test.cpp", old, bad)
        self.assertTrue(any("parameters changed" in error for error in errors))
        self.assertEqual(self.errors("CvGameCoreDLL/Test.cpp", old, good), [])

    def test_header_parameter_change_is_checked(self):
        old = "class Worker {\n\tint score(int iValue);\n};\n"
        new = "class Worker {\n\t// <!-- custom: Add bFast so the public interface exposes the cheap scoring mode. (GPT-5.6-Sol) -->\n\tint score(int iValue, bool bFast);\n};\n"
        self.assertEqual(self.errors("CvGameCoreDLL/Test.h", old, new), [])

    def test_sas_named_parameter_change_is_optional(self):
        old = "int Worker::SAS_score(int iValue)\n{\n\treturn iValue;\n}\n"
        new = "int Worker::SAS_score(int iValue, bool bFast)\n{\n\treturn iValue;\n}\n"
        self.assertEqual(self.errors("CvGameCoreDLL/Test.cpp", old, new), [])

    def test_python_parameter_change_is_checked(self):
        old = "def render(value):\n    return value\n"
        bad = "def render(value, compact=False):\n    return value\n"
        good = "# <!-- custom: Add compact so callers can request the short representation directly. (GPT-5.6-Sol) -->\ndef render(value, compact=False):\n    return value\n"
        self.assertTrue(any("parameters changed" in error for error in self.errors("LLM_Helpers/test.py", old, bad)))
        self.assertEqual(self.errors("LLM_Helpers/test.py", old, good), [])

    def test_new_file_does_not_require_dependency_or_parameter_change_notes(self):
        new = (
            "# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)\n"
            "# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)\n"
            "# <!-- custom: Add a focused helper for the new workflow. (ChatGPT-5.6-Sol) -->\n"
            "import argparse\n\n"
            "def run(value):\n"
            "    return value\n"
        )
        self.assertEqual(self.errors("LLM_Helpers/new_helper.py", None, new), [])


    def test_single_line_custom_comment_does_not_absorb_following_legacy_comment(self):
        old = "// <!-- custom: Existing rationale. (GPT-5.6-Sol) -->\nint value = 1;\n"
        new = "// <!-- custom: Existing rationale. (GPT-5.6-Sol) -->\n// New untagged prose.\nint value = 1;\n"
        self.assertTrue(any("new/rewritten non-custom comment" in error for error in self.errors("CvGameCoreDLL/Test.cpp", old, new)))

    def test_variable_construction_is_not_treated_as_function_signature(self):
        source = "void Test::run()\n{\n\tTradeData peaceTreaty(TRADE_PEACE_TREATY);\n}\n"
        names = [sig.name for sig in checker.cpp_signatures(source)]
        self.assertIn("Test::run", names)
        self.assertNotIn("peaceTreaty", names)

    def test_wonderingabout_is_valid_custom_comment_credential(self):
        old = "int value = 1;\n"
        new = "// <!-- custom: User-authored explanation. (wonderingabout) -->\nint value = 1;\n"
        self.assertEqual(self.errors("CvGameCoreDLL/Test.cpp", old, new), [])

    def test_boilerplate_and_directive_comments_are_exempt(self):
        old = ""
        new = (
            "# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)\n"
            "# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)\n"
            "value = call()  # noqa\n"
        )
        self.assertEqual(self.errors("LLM_Helpers/new_helper.py", old, new), [])


if __name__ == "__main__":
    unittest.main()
