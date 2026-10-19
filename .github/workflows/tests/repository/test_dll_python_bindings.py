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
# <!-- custom: Locate PowerShell for a build-snapshot/parser agreement test without invoking the compiler; non-PowerShell environments retain the portable integration regressions. (GPT-6.1-Sol) -->
import shutil

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

    # <!-- custom: Optional capture_bindings lets fixtures cover both new registration snapshots and older reports that still require an exact source hash. (GPT-6.1-Sol) -->
    def release(self, configuration="Release", source=NEW, capture_bindings=False):
        binary = b"new DLL"
        self.write(checker.DLL, binary)
        report = dict(configuration=configuration, status="SUCCEEDED", request="BUILD", exitCode=0,
                      errorCount=0, warningCount=0, durationMilliseconds=1000,
                      startedUtc="2026-10-18T10:00:00Z", endedUtc="2026-10-18T10:00:01Z",
                      dllAfter=dict(sizeBytes=len(binary), sha256=hashlib.sha256(binary).hexdigest()),
                      buildInputsBefore=[dict(path=SOURCE, sha256=hashlib.sha256(source).hexdigest())])
        if capture_bindings:
            report['buildInputsBefore'][0]['pythonBindingsSha256'] = checker.binding_fingerprint(source.decode())
        self.write(checker.DLL + ".build.json", json.dumps(report).encode())
        return self.commit()

    def test_missing_dll_update_fails(self):
        head = self.add_binding()
        errors, affected = checker.check(self.repo, [head], head)
        self.assertEqual({name for name, _ in affected}, {"flushSASDiagnosticLogs"})
        self.assertIn("rebuilt Release DLL", errors[0])

    # <!-- custom: The timestamp bridge is a module-level function, not a class method; ensure the same stale-DLL protection covers both registration forms. (GPT-6.1-Sol) -->
    def test_global_function_requires_matching_release(self):
        content = OLD + b'python::def("getSASDiagnosticUtcTimestamp", getSASDiagnosticUtcTimestamp, "help");\n'
        self.write(SOURCE, content)
        source = self.commit()
        errors, affected = checker.check(self.repo, [source], source)
        self.assertIn("rebuilt Release DLL", errors[0])
        self.assertEqual({name for name, _ in affected}, {"getSASDiagnosticUtcTimestamp"})
        head = self.release(source=content, capture_bindings=True)
        self.assertFalse(checker.check(self.repo, [source, head], head)[0])

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

    # <!-- custom: Reproduce the real failure: add a genuine binding and build it, then add its rationale in the same integration candidate without recompiling. (GPT-6.1-Sol) -->
    def test_post_build_comments_reflow_and_help_pass(self):
        source = self.add_binding()
        release = self.release(capture_bindings=True)
        self.write(SOURCE, b'/* documented later */\n.def ("old", & CyGame :: old, "new help")\n' +
                   b'.def(\n "flushSASDiagnosticLogs", /* why */ &CyGame::flushSASDiagnosticLogs, "new help")\n')
        head = self.commit()
        self.assertFalse(checker.check(self.repo, [source, release, head], head)[0])

    def test_captured_stale_bindings_still_fail(self):
        source = self.add_binding()
        head = self.release(source=OLD, capture_bindings=True)
        self.assertIn("does not match binding source", checker.check(self.repo, [source, head], head)[0][0])

    def test_post_build_retarget_still_fails(self):
        source = self.add_binding()
        release = self.release(capture_bindings=True)
        self.write(SOURCE, NEW.replace(b'CyGame::flushSASDiagnosticLogs', b'CyGame::replacement'))
        head = self.commit()
        self.assertIn("bindings changed", checker.check(self.repo, [source, release, head], head)[0][0])

    def test_missing_or_malformed_snapshot_does_not_excuse_mismatch(self):
        source = self.add_binding()
        self.release()
        self.write(SOURCE, b'// later comment\n' + NEW)
        head = self.commit()
        self.assertTrue(checker.check(self.repo, [source, head], head)[0])
        report_path = self.repo / (checker.DLL + '.build.json')
        report = json.loads(report_path.read_text())
        for invalid in (None, {}, [], '', 'not-a-hash'):
            with self.subTest(snapshot=invalid):
                report['buildInputsBefore'][0]['pythonBindingsSha256'] = invalid
                report_path.write_text(json.dumps(report))
                head = self.commit()
                self.assertTrue(checker.check(self.repo, [source, head], head)[0])

    # <!-- custom: Exercise the actual build-wrapper function against casts, comments, strings and every current interface file; invoke only its extracted function, never the build script's executable body. (GPT-6.1-Sol) -->
    def test_powershell_snapshot_matches_registration_checker(self):
        powershell = shutil.which('pwsh') or shutil.which('powershell')
        if not powershell:
            self.skipTest('PowerShell unavailable; integration fixtures still run')
        cases = [NEW.decode(), 'python::def("utc", getUtc); boost::python::def("other", getOther);', '', '// .def("fake", &CyGame::fake)\n',
                 '.def("f", static_cast<void (CyGame::*)(int)>(&CyGame::f), "help").def("f", &CyCity::f)',
                 '.def(/* comment */ "f", & CyGame :: f, "help").def("f", &CyGame::f)',
                 '.def("é", &CyGame::f).def("É", &CyGame::f).def("Ω", &CyGame::g).def("𐐀", &CyGame::h)']
        cases.extend(path.read_text(encoding='utf-8-sig') for path in sorted((ROOT / 'CvGameCoreDLL').glob('*Interface*.cpp')))
        inputs = self.repo / 'bindings.json'
        inputs.write_text(json.dumps(cases), encoding='utf-8')
        script = self.repo / 'CvGameCoreDLL/Project/binding_snapshot.ps1'
        script.write_text('\n'.join([
            '$ErrorActionPreference = "Stop"',
            '[Console]::OutputEncoding = New-Object Text.UTF8Encoding($false)',
            '$parseTokens = $null; $parseErrors = $null',
            '$ast = [Management.Automation.Language.Parser]::ParseFile($args[0], [ref]$parseTokens, [ref]$parseErrors)',
            'if ($parseErrors.Count) { throw ($parseErrors | Out-String) }',
            '$fixtureProject = Split-Path -Parent $PSCommandPath',
            'foreach ($name in @("Get-BuildFileState", "Get-PythonBindingRegistrations", "Get-PythonBindingFingerprint", "Get-BuildInputSnapshot")) {',
            '  $function = $ast.Find({ param($n) $n -is [Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -eq $name }, $true)',
            '  Invoke-Expression ($function.Extent.Text.Replace(\'$PSScriptRoot\', \'$fixtureProject\'))',
            '}',
            '$rows = @(foreach ($text in ([IO.File]::ReadAllText($args[1]) | ConvertFrom-Json)) { @{ bindings = @(Get-PythonBindingRegistrations $text); fingerprint = Get-PythonBindingFingerprint $text } })',
            '$snapshot = Get-BuildInputSnapshot',
            '@{ rows = $rows; captured = $snapshot["CvGameCoreDLL/CyGameInterface.cpp"].pythonBindingsSha256 } | ConvertTo-Json -Depth 5 -Compress',
        ]), encoding='utf-8')
        # <!-- custom: The Unicode-name fixture failed when Python decoded PowerShell output as the Windows CP1252 default; explicitly use UTF-8 on both sides of this test pipe. (GPT-6.1-Sol) -->
        result = subprocess.check_output([powershell, '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', str(script),
                                          str(ROOT / 'CvGameCoreDLL/Project/build_with_report.ps1'), str(inputs)], encoding='utf-8')
        snapshot = json.loads(result)
        self.assertEqual(snapshot['captured'].lower(), checker.binding_fingerprint(OLD.decode()))
        rows = snapshot['rows']
        self.assertEqual(len(rows), len(cases))
        for text, row in zip(cases, rows):
            self.assertEqual({(item['name'], item['target']) for item in row['bindings']}, checker.registrations(text))
            self.assertEqual(row['fingerprint'].lower(), checker.binding_fingerprint(text))

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
