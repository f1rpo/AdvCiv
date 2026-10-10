import importlib.util
from pathlib import Path
import sys
import unittest


REPO_ROOT = Path(__file__).resolve().parents[3]
HELPER = REPO_ROOT / "LLM_Helpers" / "collapse_cpp_log_guards.py"
HELPER_DIR = str(HELPER.parent)
if HELPER_DIR not in sys.path:
    sys.path.insert(0, HELPER_DIR)
spec = importlib.util.spec_from_file_location("collapse_cpp_log_guards", HELPER)
module = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(module)


class CollapseCppLogGuardsTests(unittest.TestCase):
    def rewrite(self, source: str) -> str:
        lines = source.splitlines(keepends=True)
        rewritten, _stats = module.rewrite_lines(lines)
        return "".join(rewritten)

    def test_recognizes_broad_local_log_gate_names(self):
        source = (
            "if (bSASSuspiciousPeaceLog)\n"
            "{\n"
            "\tlogSomething(1);\n"
            "}\n"
            "if (bDetailedYieldLogging)\n"
            "\tlogSomethingElse(2);\n"
        )
        self.assertEqual(
            self.rewrite(source),
            "if (bSASSuspiciousPeaceLog) logSomething(1);\n"
            "if (bDetailedYieldLogging) logSomethingElse(2);\n",
        )

    def test_recognizes_should_log_and_logger_enabled_predicates(self):
        source = (
            "if (SAS_shouldLogBuildingValueGateChange(a, b))\n"
            "{\n"
            "\tlogBBAI(\"x\");\n"
            "}\n"
            "if (GC.getLogger().isEnabledRand() && szLog != NULL)\n"
            "\tprintToLog(szLog);\n"
        )
        self.assertEqual(
            self.rewrite(source),
            "if (SAS_shouldLogBuildingValueGateChange(a, b)) logBBAI(\"x\");\n"
            "if (GC.getLogger().isEnabledRand() && szLog != NULL) printToLog(szLog);\n",
        )

    def test_preserves_multiline_condition_layout(self):
        source = (
            "if (gWarLogLevel >= 3 && target != NULL &&\n"
            "\t!team.isAtWar(targetTeam) && power > 0)\n"
            "{\n"
            "\tlogBBAI(\"x=%d\",\n"
            "\t\tvalue);\n"
            "}\n"
        )
        self.assertEqual(self.rewrite(source), source)

    def test_skips_semantic_only_guard(self):
        source = (
            "if (bCoastal)\n"
            "{\n"
            "\tlogBBAI(\"coastal\");\n"
            "}\n"
        )
        self.assertEqual(self.rewrite(source), source)

    def test_skips_braced_if_with_following_else(self):
        source = (
            "if (bLogDetails)\n"
            "{\n"
            "\tlogBBAI(\"a\");\n"
            "}\n"
            "else\n"
            "\tlogBBAI(\"b\");\n"
        )
        self.assertEqual(self.rewrite(source), source)

    def test_rewrite_is_idempotent(self):
        source = (
            "if (bLogDetails)\n"
            "{\n"
            "\tlogBBAI(\"x=%d\",\n"
            "\t\tvalue);\n"
            "}\n"
        )
        once = self.rewrite(source)
        twice = self.rewrite(once)
        self.assertEqual(once, twice)


if __name__ == "__main__":
    unittest.main()
