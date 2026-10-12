# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
from pathlib import Path
import re
import shutil
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / ".github/workflows/build/diagnostics"))
import sas_game_record_log as recorder

class RevisionDocumentationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.repo = Path(self.temporary.name)
        for relative in (recorder.REVISION_HEADER, recorder.REVISION_SOURCE, recorder.REVISION_HISTORY):
            target = self.repo / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / relative, target)
        self.history = self.repo / recorder.REVISION_HISTORY
        self.text = self.history.read_text('utf-8')

    def test_current_revision_documentation(self):
        self.assertEqual(recorder.check_revision(self.repo), [])

    def test_source_bump_without_history_update(self):
        path = self.repo / recorder.REVISION_HEADER
        text = path.read_text('utf-8')
        updated = re.sub(r'(SAS_GAME_RECORD_REVISION\s*=\s*)(\d+)', lambda match: match.group(1) + str(int(match.group(2)) + 1), text, count=1)
        path.write_text(updated, 'utf-8')
        self.assertTrue(any('revision mismatch' in error for error in recorder.check_revision(self.repo)))

    def test_removed_latest_history_entry(self):
        entries = list(re.finditer(r'^### Revision ', self.text, flags=re.MULTILINE))
        self.history.write_text(self.text[:entries[0].start()] + self.text[entries[1].start():], 'utf-8')
        self.assertTrue(any('revision mismatch' in error for error in recorder.check_revision(self.repo)))


if __name__ == "__main__":
    unittest.main()
