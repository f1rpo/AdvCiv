# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Exercise the real lifecycle dispatcher with a handler that changes Autolog state; a pre-handler profile would observe the wrong session. (GPT-6.1-Sol) -->
import ast
from pathlib import Path
from types import SimpleNamespace
import unittest


ROOT = Path(__file__).resolve().parents[3]


class DiagnosticContextLifecycleTests(unittest.TestCase):
    def dispatch(self, tag, level, fail_handler=False):
        source = ROOT / "Assets/Python/EntryPoints/CvEventInterface.py"
        tree = ast.parse(source.read_text(encoding="utf-8"))
        function = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "onEvent")
        module = ast.Module(body=[function], type_ignores=[])
        observations = []
        state = {"autolog": False, "game_wrappers": 0, "define_reads": 0}

        def handle_event(args):
            self.assertEqual(observations, [])
            if fail_handler:
                raise ValueError("handler failed")
            state["autolog"] = True
            return 7

        def get_define(name):
            self.assertEqual(name, "SAS_GAME_RECORD_LOG_LEVEL")
            state["define_reads"] += 1
            return level

        def game():
            state["game_wrappers"] += 1
            return SimpleNamespace(logSASGameRecordDiagnosticContext=lambda: observations.append(state["autolog"]))

        namespace = {
            "SASDefineGuard": SimpleNamespace(verify_or_raise=lambda context: None),
            "getEventManager": lambda: SimpleNamespace(handleEvent=handle_event),
            "CyGlobalContext": lambda: SimpleNamespace(getDefineINT=get_define),
            "CyGame": game,
        }
        exec(compile(module, str(source), "exec"), namespace)
        result = namespace["onEvent"]([tag])
        return result, observations, state

    def test_new_and_loaded_sessions_observe_post_handler_state(self):
        for tag in ("GameStart", "OnLoad"):
            with self.subTest(tag=tag):
                result, observations, state = self.dispatch(tag, 3)
                self.assertEqual(result, 7)
                self.assertEqual(observations, [True])
                self.assertEqual(state["game_wrappers"], 1)

    def test_disabled_logging_never_enters_native_logger(self):
        for tag in ("GameStart", "OnLoad"):
            with self.subTest(tag=tag):
                result, observations, state = self.dispatch(tag, 0)
                self.assertEqual(result, 7)
                self.assertEqual(observations, [])
                self.assertEqual(state["game_wrappers"], 0)

    def test_ordinary_events_do_not_read_settings_or_emit_profiles(self):
        result, observations, state = self.dispatch("BeginGameTurn", 3)
        self.assertEqual(result, 7)
        self.assertEqual(observations, [])
        self.assertEqual(state["define_reads"], 0)
        self.assertEqual(state["game_wrappers"], 0)

    def test_failed_handler_does_not_emit_finalized_context(self):
        with self.assertRaisesRegex(ValueError, "handler failed"):
            self.dispatch("GameStart", 3, fail_handler=True)


if __name__ == "__main__":
    unittest.main()
