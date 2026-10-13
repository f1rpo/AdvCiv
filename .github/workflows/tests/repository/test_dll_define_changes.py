# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Exercise structural define changes in real Git trees, including a later Release build and removed consumers. (GPT-6.1-Sol) -->

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
checker = importlib.import_module("dll_define_changes")


class DLLDefineChangesTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.repo = Path(temp.name)
        self.git("init", "-q")
        self.git("config", "user.name", "CI fixture")
        self.git("config", "user.email", "fixture@example.invalid")
        self.git("config", "commit.gpgsign", "false")
        self.git("config", "core.autocrlf", "false")
        self.write(checker.DLL, b"old binary")
        self.write("CvGameCoreDLL/Project/build_with_report.ps1", b"# wrapper fixture")
        self.defines(["SAS_OLD"])
        self.write("CvGameCoreDLL/example.cpp", b'getDefineINT("SAS_OLD");')
        self.base = self.commit()

    def git(self, *args):
        return subprocess.check_output(["git", *args], cwd=self.repo, stderr=subprocess.DEVNULL).decode().strip()

    def write(self, path, data):
        target = self.repo / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)

    # <!-- custom: Add an optional XML path so fixtures exercise non-SAS files and moving declarations between files; existing callers retain the original default. (GPT-6.1-Sol) -->
    def defines(self, names, value=0, path="Assets/XML/GlobalDefines_advciv_sas.xml"):
        self.write(path, ("<Defines>" + "".join(f"<Define><DefineName>{name}</DefineName><iDefineIntVal>{value}</iDefineIntVal></Define>" for name in names) + "</Defines>").encode())

    def commit(self):
        self.git("add", "-A")
        self.git("commit", "-qm", "fixture")
        return self.git("rev-parse", "HEAD")

    def add_cpp_define(self):
        self.defines(["SAS_OLD", "SAS_NEW"])
        self.write("CvGameCoreDLL/example.cpp", b'getDefineINT("SAS_OLD"); getDefineINT("SAS_NEW");')
        return self.commit()

    def release(self, configuration="Release"):
        data = b"new binary"
        self.write(checker.DLL, data)
        report = dict(configuration=configuration, status="SUCCEEDED", request="BUILD", exitCode=0, logFile="AdvCiv.log",
                      errorCount=0, warningCount=0, durationMilliseconds=1000,
                      startedUtc="2026-10-12T10:00:00Z", endedUtc="2026-10-12T10:00:01Z",
                      dllAfter=dict(sizeBytes=len(data), sha256=hashlib.sha256(data).hexdigest()))
        self.write(checker.DLL + ".build.json", json.dumps(report).encode())
        return self.commit()

    def test_non_sas_names_in_arbitrarily_named_xml_are_checked(self):
        self.defines(["FOOD_CONSUMPTION_PER_POPULATION"], path="Assets/XML/Custom/rules.xml")
        self.write("CvGameCoreDLL/example.cpp", b'getDefineINT("FOOD_CONSUMPTION_PER_POPULATION");')
        head = self.commit()
        errors, affected = checker.check(self.repo, [head], head)
        self.assertTrue(errors)
        self.assertEqual(affected, {"FOOD_CONSUMPTION_PER_POPULATION"})

    def test_define_move_between_files_passes(self):
        self.defines([])
        self.defines(["SAS_OLD"], path="Assets/XML/Custom/rules.xml")
        head = self.commit()
        self.assertEqual(checker.check(self.repo, [head], head), ([], set()))

    def test_deleted_define_file_checks_old_cpp(self):
        (self.repo / "Assets/XML/GlobalDefines_advciv_sas.xml").unlink()
        head = self.commit()
        errors, affected = checker.check(self.repo, [head], head)
        self.assertTrue(errors)
        self.assertEqual(affected, {"SAS_OLD"})

    def test_namespaced_declarations_and_commented_names(self):
        self.write("Assets/XML/Custom/rules.xml", b'<Defines xmlns="fixture"><!-- <DefineName>COMMENT_ONLY</DefineName> --><Define><DefineName>BASE_OPTION</DefineName></Define></Defines>')
        self.write("CvGameCoreDLL/example.cpp", b'getDefineINT("BASE_OPTION"); getDefineINT("COMMENT_ONLY");')
        head = self.commit()
        errors, affected = checker.check(self.repo, [head], head)
        self.assertTrue(errors)
        self.assertEqual(affected, {"BASE_OPTION"})

    def test_added_cpp_name_requires_release(self):
        head = self.add_cpp_define()
        errors, affected = checker.check(self.repo, [head], head)
        self.assertEqual(affected, {"SAS_NEW"})
        self.assertIn("rebuilt Release DLL", errors[0])
        self.assertIn("added: SAS_NEW", errors[0])
        self.assertIn("removed: none", errors[0])

    def test_removed_consumer_is_checked_in_old_tree(self):
        self.defines([])
        self.write("CvGameCoreDLL/example.cpp", b"// consumer retired")
        head = self.commit()
        self.assertTrue(checker.check(self.repo, [head], head)[0])

    def test_value_only_change_passes(self):
        self.defines(["SAS_OLD"], value=42)
        head = self.commit()
        self.assertEqual(checker.check(self.repo, [head], head), ([], set()))

    def test_python_only_and_cpp_comment_pass(self):
        self.defines(["SAS_OLD", "SAS_NEW"])
        self.write("Assets/Python/example.py", b'getDefineINT("SAS_NEW")')
        self.write("CvGameCoreDLL/example.cpp", b'// SAS_NEW\n/* SAS_NEW */\ngetDefineINT("SAS_OLD");')
        head = self.commit()
        self.assertEqual(checker.check(self.repo, [head], head), ([], set()))

    def test_later_release_at_integration_head_passes(self):
        source = self.add_cpp_define()
        head = self.release()
        self.assertEqual(checker.check(self.repo, [source, head], head), ([], {"SAS_NEW"}))

    def test_debug_build_is_rejected(self):
        source = self.add_cpp_define()
        head = self.release("Debug-opt")
        self.assertIn("requires configuration Release", checker.check(self.repo, [source, head], head)[0][0])

    def test_renamed_define_requires_release(self):
        self.defines(["SAS_RENAMED"])
        self.write("CvGameCoreDLL/example.cpp", b'getDefineINT("SAS_RENAMED");')
        head = self.commit()
        errors, affected = checker.check(self.repo, [head], head)
        self.assertTrue(errors)
        self.assertEqual(affected, {"SAS_OLD", "SAS_RENAMED"})
        self.assertIn("added: SAS_RENAMED", errors[0])
        self.assertIn("removed: SAS_OLD", errors[0])

    def test_default_branch_pr_requires_release(self):
        head = self.add_cpp_define()
        payload = self.repo / "event.json"
        event = dict(repository=dict(default_branch="trunk"), pull_request=dict(base=dict(ref="trunk", sha=self.base), head=dict(sha=head)))
        payload.write_text(json.dumps(event))
        with patch.object(sys, "argv", ["checker", "--repo-root", str(self.repo), "--event-name", "pull_request", "--event-path", str(payload)]):
            self.assertEqual(checker.main(), 1)

    def test_feature_push_defers_but_default_push_fails(self):
        head = self.add_cpp_define()
        payload = self.repo / "event.json"
        event = dict(repository=dict(default_branch="trunk"), ref="refs/heads/feature", after=head, commits=[dict(id=head)])
        for branch, expected in (("feature", 0), ("trunk", 1)):
            event["ref"] = "refs/heads/" + branch
            payload.write_text(json.dumps(event))
            with patch.object(sys, "argv", ["checker", "--repo-root", str(self.repo), "--event-name", "push", "--event-path", str(payload)]):
                self.assertEqual(checker.main(), expected)


if __name__ == "__main__":
    unittest.main()
