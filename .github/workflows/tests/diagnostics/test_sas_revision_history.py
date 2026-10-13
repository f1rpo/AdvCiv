# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[4]
# <!-- custom: Restore blocking checks in the automated subject folders; regression fixtures import those same helpers. (GPT-6.1-Sol) -->
sys.path.insert(0, str(ROOT / ".github/workflows/build/diagnostics"))
import sas_revision_history as history


def git(repo, *args):
    return subprocess.check_output(["git", *args], cwd=repo, stderr=subprocess.DEVNULL).decode().strip()


def fixture_git(repo):
    git(repo, "init", "-q")
    git(repo, "config", "user.name", "CI fixture")
    git(repo, "config", "user.email", "fixture@example.invalid")
    git(repo, "config", "commit.gpgsign", "false")
    git(repo, "config", "core.autocrlf", "false")

class RevisionHistoryTests(unittest.TestCase):
    def test_feature_rewrite_does_not_replace_default_branch_commit(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            header = repo / history.HEADER
            header.parent.mkdir(parents=True)
            header.write_text('enum { SAS_GAME_RECORD_REVISION = 69 };\n')
            git(repo, 'add', '.')
            git(repo, 'commit', '-qm', 'original fixture revision')
            original = git(repo, 'rev-parse', 'HEAD')
            git(repo, 'branch', 'main')
            git(repo, 'switch', '-qc', 'feature')
            git(repo, 'commit', '--amend', '-qm', 'amended fixture revision')
            amended = git(repo, 'rev-parse', 'HEAD')
            target = repo / history.HISTORY
            target.parent.mkdir(parents=True)
            def record(commit):
                target.write_text('### Revision 70 - fixture\n- **Git commit:** pending\n\n### Revision 69 - fixture\n- **Git commit:** `' + commit + '`\n', 'utf-8')
            record(original)
            self.assertEqual(history.check(repo, "main"), [])
            record(amended)
            git(repo, 'cat-file', '-e', amended)
            self.assertTrue(any('ancestry' in e for e in history.check(repo, "main")))
            record(original)
            git(repo, 'cat-file', '-e', original)
            record('pending')
            self.assertTrue(any('latest entry' in e for e in history.check(repo, "main")))
            record(original)
            target.write_text(target.read_text().replace('Revision 69', 'Revision 71'))
            self.assertTrue(any('matching explicit source marker' in e for e in history.check(repo, "main")))

    def test_missing_default_branch_abstains_from_ancestry(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            target = repo / history.HISTORY
            target.parent.mkdir(parents=True)
            target.write_text('### Revision 68 - fixture\n- **Git commit:** `0000000000000000000000000000000000000000`\n', 'utf-8')
            self.assertEqual(history.check(repo, "refs/remotes/origin/missing"), [])


if __name__ == "__main__":
    unittest.main()
