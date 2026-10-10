# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: The shared comment-reflow helper touches C++, Python and XML source.
# These regression cases protect exact technical wording, existing credits/KI references, structured comments and source content while checking logical reflow and repeat-run stability. (GPT-6.1-Sol) -->
import importlib.util
from pathlib import Path
# <!-- custom: Add subprocess, sys and tempfile to verify the real check command reports failing paths and exit codes without rewriting the source fixture. (GPT-6.1-Sol) -->
import subprocess
import sys
import tempfile
import unittest


HELPER = Path(__file__).resolve().parents[3] / "LLM_Helpers" / "reflow_custom_comments.py"
spec = importlib.util.spec_from_file_location("reflow_custom_comments", HELPER)
module = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(module)


class ReflowCustomCommentsTests(unittest.TestCase):
    # <!-- custom: CI now runs check mode automatically; prove eligible comments fail without edits, repaired comments pass and intentional structured skips remain non-blocking. (GPT-6.1-Sol) -->
    def test_cli_check_is_read_only_and_reports_paths(self):
        with tempfile.TemporaryDirectory(prefix="sas-reflow-check-") as folder:
            source = Path(folder) / "sample.py"
            original = b'# <!-- custom: First sentence. Second sentence. (GPT-6.1-Sol) -->\nvalue = 1\n'
            source.write_bytes(original)
            result = subprocess.run([sys.executable, str(HELPER), str(source), "--check"], capture_output=True, text=True)
            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
            self.assertIn("needs logical reflow:", result.stdout)
            self.assertIn(str(source), result.stdout)
            self.assertEqual(source.read_bytes(), original)
            subprocess.run([sys.executable, str(HELPER), str(source), "--apply"], capture_output=True, check=True)
            result = subprocess.run([sys.executable, str(HELPER), str(source), "--check"], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertNotIn("needs logical reflow:", result.stdout)
            structured = b'# <!-- custom: List:\n# - first item\n# - second item -->\n'
            source.write_bytes(structured)
            result = subprocess.run([sys.executable, str(HELPER), str(source), "--check"], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual(source.read_bytes(), structured)

    def transform(self, text, suffix=".cpp"):
        return module.transform_text(text, suffix)[0]

    def test_preserves_technical_payload_credit_and_ki(self):
        source = '// <!-- custom: Keep "0 / 0 / 0 / 0 / 100" unchanged. Preserve 2 placeholders vs 3 substitutions. See KI#234. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->\n'
        expected = '// <!-- custom: Keep "0 / 0 / 0 / 0 / 100" unchanged.\n// Preserve 2 placeholders vs 3 substitutions. See KI#234. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->\n'
        self.assertEqual(self.transform(source), expected)

    def test_joins_wrapped_sentence_without_touching_inherited_comment(self):
        source = '// Inherited note. Keep this line unchanged.\n// <!-- custom: Keep the threshold because\n// the current value avoids repeated scans. (GPT-5.3-Codex) -->\nint value = 3;\n'
        expected = '// Inherited note. Keep this line unchanged.\n// <!-- custom: Keep the threshold because the current value avoids repeated scans. (GPT-5.3-Codex) -->\nint value = 3;\n'
        self.assertEqual(self.transform(source), expected)

    def test_structured_and_code_like_comments_are_unchanged(self):
        sources = [
            '// <!-- custom: Values:\n// - preserve first item\n// - preserve second item -->\n',
            '// <!-- custom: Example:\n// if (enabled) logBBAI("x"); -->\n',
            '// <!-- custom: First paragraph.\n//\n// Second paragraph. -->\n',
        ]
        for source in sources:
            with self.subTest(source=source):
                self.assertEqual(self.transform(source), source)

    def test_xml_reflow_preserves_data_and_abbreviations(self):
        source = '<Root>\n\t<!-- custom: Keep e.g. these values. Preserve the tag. (GPT-5.5) -->\n\t<Value>70</Value>\n</Root>\n'
        expected = '<Root>\n\t<!-- custom: Keep e.g. these values.\n\tPreserve the tag. (GPT-5.5) -->\n\t<Value>70</Value>\n</Root>\n'
        self.assertEqual(self.transform(source, ".xml"), expected)

    def test_python_tokens_and_final_newline_are_preserved(self):
        source = '# <!-- custom: First sentence. Second sentence. (GPT-5.5) -->\nvalue = "not a comment"'
        result = self.transform(source, ".py")
        self.assertEqual(module.significant_python_tokens(source), module.significant_python_tokens(result))
        self.assertFalse(result.endswith("\n"))
        self.assertTrue(self.transform(source + "\n", ".py").endswith("\n"))

    def test_reflow_is_idempotent(self):
        source = '// <!-- custom: First observation. Another observation. See KI#234. (GPT-5.5) -->\n'
        result = self.transform(source)
        self.assertEqual(self.transform(result), result)


if __name__ == "__main__":
    unittest.main()
