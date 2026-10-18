# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Exercise stale shipped DLLs against real temporary Git commits; fixture manifests must identify the binding source as well as the binary. (GPT-6.1-Sol) -->

import hashlib
import importlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / ".github/workflows/build/repository"))
checker = importlib.import_module("dll_python_bindings")
SOURCE = "CvGameCoreDLL/CyGameInterface.cpp"
OLD = b'.def("old", &CyGame::old, "help")\n'
NEW = OLD + b'.def("flushSASDiagnosticLogs", &CyGame::flushSASDiagnosticLogs, "help")\n'


class DLLPythonBindingsTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.repo = Path(temporary.name)
        self.git("init", "-q")
        for key, value in (("user.name", "CI fixture"), ("user.email", "fixture@example.invalid"), ("commit.gpgsign", "false"), ("core.autocrlf", "false")):
            self.git("config", key, value)
        self.write(checker.DLL, b"old DLL")
        self.write("CvGameCoreDLL/Project/build_with_report.ps1", b"# fixture wrapper")
        self.write(SOURCE, OLD)
        self.base = self.commit()

    def git(self, *args):
        return subprocess.check_output(["git", *args], cwd=self.repo, stderr=subprocess.DEVNULL).decode().strip()

    def write(self, path, data):
        target = self.repo / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)

    def commit(self):
        self.git("add", "-A")
        self.git("commit", "-qm", "fixture")
        return self.git("rev-parse", "HEAD")

    def add_binding(self):
        self.write(SOURCE, NEW)
        return self.commit()

    def release(self, configuration="Release", source=NEW):
        binary = b"new DLL"
        self.write(checker.DLL, binary)
        report = dict(configuration=configuration, status="SUCCEEDED", request="BUILD", exitCode=0,
                      errorCount=0, warningCount=0, durationMilliseconds=1000,
                      startedUtc="2026-10-18T10:00:00Z", endedUtc="2026-10-18T10:00:01Z",
                      dllAfter=dict(sizeBytes=len(binary), sha256=hashlib.sha256(binary).hexdigest()),
                      buildInputsBefore=[dict(path=SOURCE, sha256=hashlib.sha256(source).hexdigest())])
        self.write(checker.DLL + ".build.json", json.dumps(report).encode())
        return self.commit()

    def test_missing_dll_update_fails(self):
        head = self.add_binding()
        errors, affected = checker.check(self.repo, [head], head)
        self.assertEqual({name for name, _ in affected}, {"flushSASDiagnosticLogs"})
        self.assertIn("rebuilt Release DLL", errors[0])

    def test_later_matching_release_passes(self):
        source = self.add_binding()
        head = self.release()
        self.assertFalse(checker.check(self.repo, [source, head], head)[0])

    def test_matching_windows_newlines_pass(self):
        source = self.add_binding()
        head = self.release(source=NEW.replace(b"\n", b"\r\n"))
        self.assertFalse(checker.check(self.repo, [source, head], head)[0])

    def test_wrong_build_source_fails(self):
        source = self.add_binding()
        head = self.release(source=OLD)
        self.assertIn("does not match binding source", checker.check(self.repo, [source, head], head)[0][0])

    def test_debug_build_fails(self):
        source = self.add_binding()
        head = self.release(configuration="Debug-opt")
        self.assertIn("requires configuration Release", checker.check(self.repo, [source, head], head)[0][0])

    def test_missing_report_infrastructure_fails(self):
        source = self.add_binding()
        (self.repo / "CvGameCoreDLL/Project/build_with_report.ps1").unlink()
        head = self.release()
        self.assertIn("build-report infrastructure", checker.check(self.repo, [source, head], head)[0][0])

    def test_removed_and_retargeted_bindings_fail(self):
        for content in (b"// removed\n", OLD.replace(b"CyGame::old", b"CyGame::replacement")):
            with self.subTest(content=content):
                self.write(SOURCE, content)
                head = self.commit()
                self.assertTrue(checker.check(self.repo, [head], head)[0])
                self.write(SOURCE, OLD)
                self.commit()

    def test_move_reflow_help_and_comments_do_not_require_release(self):
        self.write(SOURCE, b'// .def("fake", &CyGame::fake)\nconst char* text=".def(\\"fake\\", &CyGame::fake)";\n')
        self.write("CvGameCoreDLL/CyGameInterfaceOther.cpp", b'.def ( /* note */ "old",\n & CyGame :: old, "changed help")\n')
        head = self.commit()
        self.assertEqual(checker.check(self.repo, [head], head), ([], set()))

    def test_cast_target_and_duplicate_names_remain_distinct(self):
        text = '.def("f", static_cast<void (CyGame::*)(int)>(&CyGame::f), "help").def("f", &CyCity::f)'
        found = checker.registrations(text)
        self.assertEqual(len(found), 2)
        self.assertTrue(any("static_cast" in target for _name, target in found))

    def test_default_branch_enforces_and_feature_branch_defers(self):
        head = self.add_binding()
        payload = self.repo / "event.json"
        event = dict(repository=dict(default_branch="trunk"), ref="refs/heads/feature", after=head, commits=[dict(id=head)])
        for branch, expected in (("feature", 0), ("trunk", 1)):
            event["ref"] = "refs/heads/" + branch
            payload.write_text(json.dumps(event))
            with patch.object(sys, "argv", ["checker", "--repo-root", str(self.repo), "--event-name", "push", "--event-path", str(payload)]):
                self.assertEqual(checker.main(), expected)


if __name__ == "__main__":
    unittest.main()
