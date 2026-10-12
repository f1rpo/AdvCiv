# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
from pathlib import Path
from contextlib import redirect_stderr
import io
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
# <!-- custom: Restore blocking checks in the automated subject folders; regression fixtures import those same helpers. (GPT-6.1-Sol) -->
sys.path.insert(0, str(ROOT / ".github/workflows/build/documentation"))
# <!-- custom: This fixture also uses the XML namespace helper; the XML and documentation tools now have separate subject folders. (GPT-6.1-Sol) -->
sys.path.insert(0, str(ROOT / ".github/workflows/build/xml"))
import asset_primary_tech as tech
import generated_docs as docs


def git(repo, *args):
    return subprocess.check_output(["git", *args], cwd=repo, stderr=subprocess.DEVNULL).decode().strip()


def fixture_git(repo):
    git(repo, "init", "-q")
    git(repo, "config", "user.name", "CI fixture")
    git(repo, "config", "user.email", "fixture@example.invalid")
    git(repo, "config", "commit.gpgsign", "false")
    git(repo, "config", "core.autocrlf", "false")

class GeneratedDocsTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.repo = Path(self.temporary.name)
        for relative in (docs.BASELINE, docs.REPORT, docs.HANDICAP, docs.MANUAL / 'manual.odt', docs.MANUAL / 'manual.txt'):
            target = self.repo / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / relative, target)

    def test_current_and_changed_xml(self):
        self.assertEqual(docs.check(self.repo), [])
        path = self.repo / docs.HANDICAP
        tree = ET.parse(path)
        field = next(node for node in tree.getroot().iter() if tech.tag(node) == 'iAIResearchPercent')
        field.text = str(int(field.text) + 1)
        tree.write(path, encoding='utf-8', xml_declaration=True)
        self.assertTrue(any('handicap' in e for e in docs.check(self.repo)))

    def test_changed_report(self):
        path = self.repo / docs.REPORT
        text = path.read_text('utf-8')
        changed = re.sub(r'(Changed fields: )(\d+)', lambda match: match.group(1) + str(int(match.group(2)) + 1), text, count=1)
        self.assertNotEqual(changed, text)
        path.write_text(changed, 'utf-8')
        self.assertTrue(any('handicap' in e for e in docs.check(self.repo)))

    def test_stale_manual(self):
        with (self.repo / docs.MANUAL / 'manual.txt').open('a', encoding='utf-8') as stream:
            stream.write('stale extraction\n')
        self.assertTrue(any('manual.txt' in e for e in docs.check(self.repo)))

    def second_conversion(self):
        def convert(source, output):
            output.write_text(source.read_text('utf-8').upper(), 'utf-8')
        conversion = docs.TextConversion(Path('Other Docs/notes.source'), Path('Search Copies/notes.txt'), convert, 'python other_converter.py', (Path('Other Docs/notes.export'),))
        for relative, text in ((conversion.source, 'converted notes\n'), (conversion.output, 'CONVERTED NOTES\n'), (conversion.related_sources[0], 'related artifact')):
            target = self.repo / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(text, 'utf-8')
        return conversion

    def test_second_conversion_uses_its_own_converter_and_paths(self):
        conversion = self.second_conversion()
        conversions = (*docs.TEXT_CONVERSIONS, conversion)
        self.assertEqual(docs.check(self.repo, conversions=conversions), [])
        (self.repo / conversion.source).write_text('updated notes\n', 'utf-8')
        errors = docs.check(self.repo, conversions=conversions)
        self.assertEqual(errors, ['Search Copies/notes.txt is stale; run python other_converter.py'])
        conversion.converter(self.repo / conversion.source, self.repo / conversion.output)
        self.assertEqual(docs.check(self.repo, conversions=conversions), [])

    def test_registered_conversion_requires_source_and_output(self):
        conversion = self.second_conversion()
        for relative in (conversion.source, conversion.output):
            (self.repo / relative).unlink()
        errors = docs.text_conversion_errors(self.repo, (conversion,))
        self.assertEqual(len(errors), 1)
        self.assertIn(conversion.source.as_posix(), errors[0])
        self.assertIn(conversion.output.as_posix(), errors[0])

    def test_second_conversion_pairs_related_source_over_entire_range(self):
        conversion = self.second_conversion()
        fixture_git(self.repo)
        git(self.repo, 'add', '.')
        git(self.repo, 'commit', '-qm', 'initial conversions fixture')
        before = git(self.repo, 'rev-parse', 'HEAD')
        (self.repo / conversion.related_sources[0]).write_text('updated related artifact', 'utf-8')
        git(self.repo, 'add', '.')
        git(self.repo, 'commit', '-qm', 'related source update')
        self.assertEqual(docs.changed_conversion_errors(self.repo, before, (conversion,)), ['Other Docs/notes.export changed without a Search Copies/notes.txt refresh in this change range'])
        (self.repo / conversion.output).write_text('reviewed paired update', 'utf-8')
        git(self.repo, 'add', '.')
        git(self.repo, 'commit', '-qm', 'later converted text update')
        self.assertEqual(docs.changed_conversion_errors(self.repo, before, (conversion,)), [])

    def test_missing_base_fails_for_pr_and_local_checks(self):
        fixture_git(self.repo)
        for event in (None, 'pull_request'):
            errors = docs.changed_conversion_errors(self.repo, 'f' * 40, event_name=event)
            self.assertEqual(len(errors), 1)
            self.assertIn('change-range base', errors[0])
            self.assertIn('unavailable', errors[0])

    def test_missing_push_base_reports_gap_but_still_checks_current_content(self):
        fixture_git(self.repo)
        diagnostics = io.StringIO()
        with redirect_stderr(diagnostics):
            self.assertEqual(docs.check(self.repo, 'f' * 40, event_name='push'), [])
        self.assertIn('paired source/text change-range validation cannot run', diagnostics.getvalue())
        with (self.repo / docs.MANUAL / 'manual.txt').open('a', encoding='utf-8') as stream:
            stream.write('stale extraction')
        with redirect_stderr(io.StringIO()):
            errors = docs.check(self.repo, 'f' * 40, event_name='push')
        self.assertTrue(any('manual.txt is stale' in error for error in errors))

    def test_missing_push_base_is_fetched_and_pairing_is_enforced(self):
        fixture_git(self.repo)
        pdf = self.repo / docs.MANUAL / 'manual.pdf'
        pdf.write_bytes(b'old PDF fixture')
        git(self.repo, 'add', '.')
        git(self.repo, 'commit', '-qm', 'initial fixture')
        with tempfile.TemporaryDirectory() as temporary:
            clone = Path(temporary) / 'runner'
            subprocess.run(['git', 'clone', '-q', str(self.repo), str(clone)], check=True)
            pdf.write_bytes(b'changed PDF fixture')
            git(self.repo, 'add', '.')
            git(self.repo, 'commit', '-qm', 'commit absent from runner checkout')
            before = git(self.repo, 'rev-parse', 'HEAD')
            self.assertFalse(docs.has_commit(clone, before))
            errors = docs.changed_conversion_errors(clone, before, event_name='push')
            self.assertTrue(docs.has_commit(clone, before))
            self.assertTrue(any('manual.pdf changed without' in error for error in errors))

    def test_root_push_without_previous_tree_needs_no_range(self):
        self.assertEqual(docs.changed_conversion_errors(self.repo, '0' * 40, event_name='push'), [])

    def test_pdf_pairing_over_entire_range(self):
        fixture_git(self.repo)
        pdf = self.repo / docs.MANUAL / 'manual.pdf'
        pdf.write_bytes(b'old PDF fixture')
        git(self.repo, 'add', '.')
        git(self.repo, 'commit', '-qm', 'initial fixture')
        before = git(self.repo, 'rev-parse', 'HEAD')
        pdf.write_bytes(b'new PDF fixture')
        git(self.repo, 'add', '.')
        git(self.repo, 'commit', '-qm', 'PDF fixture update')
        self.assertTrue(docs.changed_conversion_errors(self.repo, before))
        with (self.repo / docs.MANUAL / 'manual.txt').open('a') as stream:
            stream.write('paired fixture update')
        git(self.repo, 'add', '.')
        git(self.repo, 'commit', '-qm', 'later text fixture update')
        self.assertEqual(docs.changed_conversion_errors(self.repo, before), [])


if __name__ == "__main__":
    unittest.main()
