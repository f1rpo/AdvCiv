# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
from pathlib import Path
import re
import shutil
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[4] / ".github/workflows/build/diagnostics"))
# <!-- custom: Keep the recorder import directly after its search-path setup and move ROOT below imports so Ruff can check the fixture without an E402 suppression; the imported dependency is unchanged. (GPT-6.1-Sol) -->
import sas_game_record_log as recorder

ROOT = Path(__file__).resolve().parents[4]

class RevisionDocumentationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.repo = Path(self.temporary.name)
        for relative in (recorder.REVISION_DEFINES, recorder.REVISION_SOURCE, recorder.REVISION_HISTORY):
            target = self.repo / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / relative, target)
        self.history = self.repo / recorder.REVISION_HISTORY
        self.text = self.history.read_text('utf-8')
        # <!-- custom: Derive mutation fixtures from the current newest revision so the regression tests survive each legitimate revision bump instead of hardcoding yesterday's number. (ChatGPT-5.6-Sol) -->
        self.revision = recorder.latest_history_revision(self.text)[0]
        self.assertIsNotNone(self.revision)

    def test_current_revision_documentation(self):
        self.assertEqual(recorder.check_revision(self.repo), [])

    def _set_revision(self, value):
        path = self.repo / recorder.REVISION_DEFINES
        text = path.read_text('utf-8')
        pattern = r'(<DefineName>SAS_GAME_RECORD_REVISION</DefineName>\s*<iDefineIntVal>)\d+(</iDefineIntVal>)'
        updated, count = re.subn(pattern, r'\g<1>%d\g<2>' % value, text, count=1)
        self.assertEqual(count, 1)
        path.write_text(updated, 'utf-8')

    def test_define_bump_without_history_update(self):
        self._set_revision(self.revision + 1)
        self.assertTrue(any('revision mismatch' in error for error in recorder.check_revision(self.repo)))

    def test_history_bump_without_define_update(self):
        pattern = r'^(### Revision )%d( - SAS practical \d+)' % self.revision
        replacement = r'\g<1>%d\g<2>' % (self.revision + 1)
        self.history.write_text(re.sub(pattern, replacement, self.text, count=1, flags=re.MULTILINE), 'utf-8')
        self.assertTrue(any('revision mismatch' in error for error in recorder.check_revision(self.repo)))

    def test_removed_latest_history_entry(self):
        entries = list(re.finditer(r'^### Revision ', self.text, flags=re.MULTILINE))
        self.history.write_text(self.text[:entries[0].start()] + self.text[entries[1].start():], 'utf-8')
        self.assertTrue(any('revision mismatch' in error for error in recorder.check_revision(self.repo)))

    def test_latest_revision_needs_more_than_date_and_commit(self):
        entries = list(re.finditer(r'^### Revision ', self.text, flags=re.MULTILINE))
        start, end = entries[0].start(), entries[1].start()
        metadata = self.text[start:end].split('- **Change:**', 1)[0]
        for content in ('', '- **Change:** \n', '- **Change:** <!-- later -->\n'):
            with self.subTest(content=content):
                self.history.write_text(self.text[:start] + metadata + content + self.text[end:], 'utf-8')
                errors = recorder.check_revision(self.repo)
                self.assertTrue(any('nonempty Change' in error for error in errors), errors)

if __name__ == "__main__":
    unittest.main()
