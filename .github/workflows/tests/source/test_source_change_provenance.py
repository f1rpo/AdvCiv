# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: These fixtures protect the diff-aware source-history guard itself: reflowed legacy prose must remain legal, newly authored prose must be explicitly custom, missing model credits remain advisory for user comments, dependency edits need rationale, and changed non-SAS interfaces need nearby explanation while new files require dependency rationale but no changed-parameter explanation. (ChatGPT-6-Sol + GPT-6.1-Sol) -->

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
# <!-- custom: Remove duplicate subprocess/tempfile imports; the single imports above remain necessary for the real Git fixtures. (GPT-6.1-Sol) -->

# <!-- custom: Restore blocking checks in the automated subject folders; regression fixtures import those same helpers. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[4] / ".github/workflows/build/source"))
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

    # <!-- custom: Guard the specific loss that an any-credit check misses, while keeping deletions, unrelated replacements and newly uncredited user comments legal. (GPT-6.1-Sol) -->
    def test_partial_rewrite_cannot_drop_one_existing_credit(self):
        prose = "Keep pending diagnostic rows bounded across every active file so failures cannot exhaust memory."
        for path, prefix, close in (("CvGameCoreDLL/Test.cpp", "// ", "int value = 1;"), ("LLM_Helpers/test.py", "# ", "value = 1"), ("Assets/XML/Test.xml", "", "<root/>")):
            with self.subTest(path=path):
                old = prefix + "<!-- custom: " + prose + " (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->\n" + close + "\n"
                bad = prefix + "<!-- custom: " + prose + " Report overflow after recovery. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->\n" + close + "\n"
                errors = self.errors(path, old, bad)
                self.assertTrue(any("dropped existing credit(s): GPT-5.6-Sol" in error for error in errors), errors)
                good = bad.replace("ChatGPT-5.6-Sol + GPT-6.1-Sol", "GPT-6.1-Sol + GPT-5.6-Sol + ChatGPT-5.6-Sol")
                self.assertEqual(self.errors(path, old, good), [])

    def test_credit_only_removal_fails_even_when_all_credits_are_removed(self):
        old = "// <!-- custom: Existing rationale. (GPT-5.6-Sol) -->\nint value = 1;\n"
        new = "// <!-- custom: Existing rationale. -->\nint value = 1;\n"
        self.assertTrue(any("dropped existing credit" in error for error in self.errors("CvGameCoreDLL/Test.cpp", old, new)))

    def test_chatgpt_credit_does_not_replace_gpt_credit(self):
        old = "// <!-- custom: Existing rationale. (GPT-5.6-Sol) -->\nint value = 1;\n"
        new = old.replace("(GPT-5.6-Sol)", "(ChatGPT-5.6-Sol)")
        self.assertTrue(any("dropped existing credit" in error for error in self.errors("CvGameCoreDLL/Test.cpp", old, new)))

    def test_credit_reordering_and_comment_reflow_are_allowed(self):
        old = "// <!-- custom: Keep the explanation on two\n// physical lines. (GPT-5.6-Sol + ChatGPT-5.6-Sol) -->\nint value = 1;\n"
        new = "// <!-- custom: Keep the explanation on two physical lines. (ChatGPT-5.6-Sol + GPT-5.6-Sol + GPT-6.1-Sol) -->\nint value = 1;\n"
        self.assertEqual(self.errors("CvGameCoreDLL/Test.cpp", old, new), [])

    def test_deleted_comment_and_unrelated_replacement_do_not_require_old_credit(self):
        old = "// <!-- custom: Explain obsolete worker allocation. (GPT-5.6-Sol) -->\nint value = 1;\n"
        for new in ("int value = 1;\n", "// <!-- custom: Describe network packet routing. (GPT-6.1-Sol) -->\nint value = 1;\n"):
            with self.subTest(new=new):
                self.assertEqual(self.errors("CvGameCoreDLL/Test.cpp", old, new), [])

    def test_entirely_corrected_explanation_can_replace_the_old_credit(self):
        old = "// <!-- custom: Close the handle because it forces every byte onto physical disk. (GPT-5.6-Sol) -->\nint value = 1;\n"
        new = "// <!-- custom: Closing publishes the buffered output for readers; storage durability is not guaranteed. (GPT-6.1-Sol) -->\nint value = 1;\n"
        self.assertEqual(self.errors("CvGameCoreDLL/Test.cpp", old, new), [])

    def test_reflow_does_not_hide_credit_loss(self):
        old = "// <!-- custom: Keep the explanation on two\n// physical lines. (GPT-5.6-Sol + ChatGPT-5.6-Sol) -->\nint value = 1;\n"
        new = "// <!-- custom: Keep the explanation on two physical lines. (ChatGPT-5.6-Sol) -->\nint value = 1;\n"
        self.assertTrue(any("dropped existing credit" in error for error in self.errors("CvGameCoreDLL/Test.cpp", old, new)))

    def test_model_mention_in_prose_is_not_a_suffix_credit(self):
        old = "// <!-- custom: GPT-5.6-Sol reviewed this explanation. (GPT-6.1-Sol) -->\nint value = 1;\n"
        new = old.replace("GPT-5.6-Sol reviewed", "ChatGPT-5.6-Sol reviewed")
        self.assertEqual(self.errors("CvGameCoreDLL/Test.cpp", old, new), [])

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

    def test_python_binding_requires_rationale_even_with_help_string(self):
        path = "CvGameCoreDLL/CyGameInterface.cpp"
        binding = '\t.def("observe", &CyGame::observe, "void () - observe state")\n'
        rationale = '// <!-- custom: Python owns this screen callback, so expose its observation to the recorder. (GPT-6.1-Sol) -->\n'
        for old in (None, "void setup() {}\n"):
            self.assertTrue(any(".def binding" in error for error in self.errors(path, old, binding)))
            self.assertEqual(self.errors(path, old, rationale + binding), [])
        self.assertTrue(any(".def binding" in error for error in self.errors(path, binding, "")))
        changed = binding.replace("&CyGame::observe", "&CyGame::observeNew")
        self.assertTrue(any(".def binding" in error for error in self.errors(path, binding, changed)))
        self.assertEqual(self.errors(path, binding, rationale + changed), [])
        self.assertEqual(self.errors(path, binding, rationale), [])
        self.assertEqual(self.errors(path, binding, binding.replace(", &", ",\n\t	&")), [])

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

    def test_new_file_functions_do_not_require_changed_parameter_notes(self):
        new = "def run(value):\n    return value\n"
        self.assertEqual(self.errors("LLM_Helpers/new_helper.py", None, new), [])

    def test_new_cpp_file_requires_include_rationale(self):
        bad = '#include "CvGame.h"\nint value;\n'
        good = '// <!-- custom: CvGame.h supplies complete game getters used by this source. (GPT-6.1-Sol) -->\n' + bad
        self.assertTrue(any("include/import change" in error for error in self.errors("CvGameCoreDLL/New.cpp", None, bad)))
        self.assertEqual(self.errors("CvGameCoreDLL/New.cpp", None, good), [])

    def test_new_python_file_requires_import_rationale(self):
        bad = "import argparse\nvalue = 1\n"
        good = "# <!-- custom: argparse exposes the helper's command-line options. (GPT-6.1-Sol) -->\n" + bad
        self.assertTrue(any("include/import change" in error for error in self.errors("LLM_Helpers/new.py", None, bad)))
        self.assertEqual(self.errors("LLM_Helpers/new.py", None, good), [])

    def test_distant_comment_cannot_cover_new_file_dependency(self):
        new = "# <!-- custom: Explain a later calculation. (GPT-6.1-Sol) -->\n" + "\n" * 12 + "import argparse\n"
        self.assertTrue(any("include/import change" in error for error in self.errors("LLM_Helpers/new.py", None, new)))

    def test_each_dependency_requires_nearby_rationale(self):
        new = "# <!-- custom: argparse handles CLI options. (GPT-6.1-Sol) -->\nimport argparse\n" + "\n" * 12 + "import pathlib\n"
        errors = self.errors("LLM_Helpers/new.py", None, new)
        self.assertEqual(len(errors), 1)
        self.assertIn("pathlib", errors[0])

    def test_untouched_inherited_includes_do_not_need_new_comments(self):
        old = '#include "Inherited.h"\nint value = 1;\n'
        new = '#include "Inherited.h"\nint value = 2;\n'
        self.assertEqual(self.errors("CvGameCoreDLL/Test.cpp", old, new), [])

    def test_shared_comment_can_explain_related_new_dependencies(self):
        new = '// <!-- custom: vector stores fixture rows and string holds their text. (GPT-6.1-Sol) -->\n#include <vector>\n#include <string>\n'
        self.assertEqual(self.errors("tests/New.cpp", None, new), [])


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

    # <!-- custom: The noqa below is literal fixture text, not an active lint suppression; it verifies that machine directives do not require explanatory-comment authorship markers. (GPT-6.1-Sol) -->
    def test_boilerplate_and_directive_comments_are_exempt(self):
        old = ""
        new = (
            "# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)\n"
            "# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)\n"
            "value = call()  # noqa\n"
        )
        self.assertEqual(self.errors("LLM_Helpers/new_helper.py", old, new), [])


    # <!-- custom: Use a real Git fixture to verify that restoring checks under new paths preserves comment provenance across renames. (GPT-6.1-Sol) -->
    def test_rename_preserves_inherited_comments_but_checks_new_comments(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            def git(*args):
                return subprocess.check_output(["git", *args], cwd=repo, stderr=subprocess.DEVNULL).decode().strip()
            git("init", "-q")
            git("config", "user.name", "CI fixture")
            git("config", "user.email", "fixture@example.invalid")
            git("config", "commit.gpgsign", "false")
            original = repo / "LLM_Helpers/old_helper.py"
            original.parent.mkdir()
            original.write_text("# Inherited explanation.\n" + "".join("value_%d = %d\n" % (i, i) for i in range(20)), encoding="utf-8")
            git("add", ".")
            git("commit", "-qm", "fixture base")
            base = git("rev-parse", "HEAD")
            moved = original.with_name("new_helper.py")
            original.rename(moved)
            git("add", "-A")
            self.assertEqual(checker.check(repo, base)[0], [])
            moved.write_text(moved.read_text(encoding="utf-8") + "# Newly authored untagged explanation.\n", encoding="utf-8")
            git("add", "-A")
            self.assertTrue(any("new/rewritten non-custom comment" in error for error in checker.check(repo, base)[0]))


if __name__ == "__main__":
    unittest.main()
