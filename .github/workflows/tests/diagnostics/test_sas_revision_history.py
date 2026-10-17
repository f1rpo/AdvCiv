# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

# <!-- custom: Restore blocking checks in the automated subject folders; regression fixtures import those same helpers. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[4] / ".github/workflows/build/diagnostics"))
import sas_revision_history as history


def git(repo, *args):
    return subprocess.check_output(["git", *args], cwd=repo, stderr=subprocess.DEVNULL).decode().strip()


def fixture_git(repo):
    git(repo, "init", "-q")
    git(repo, "config", "user.name", "CI fixture")
    git(repo, "config", "user.email", "fixture@example.invalid")
    git(repo, "config", "commit.gpgsign", "false")
    git(repo, "config", "core.autocrlf", "false")


def write_revision_define(repo, value):
    path = repo / history.DEFINES
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text('<Civ4Defines><Define><DefineName>SAS_GAME_RECORD_REVISION</DefineName><iDefineIntVal>%d</iDefineIntVal></Define></Civ4Defines>' % value, 'utf-8')


def commit_fixture(repo, message):
    git(repo, 'add', '.')
    git(repo, 'commit', '-qm', message)
    return git(repo, 'rev-parse', 'HEAD')


class RevisionHistoryTests(unittest.TestCase):
    def test_feature_rewrite_does_not_replace_default_branch_commit(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            header = repo / history.HEADER
            header.parent.mkdir(parents=True)
            header.write_text('enum { SAS_GAME_RECORD_REVISION = 69 };\n')
            original = commit_fixture(repo, 'original fixture revision')
            git(repo, 'branch', 'main')
            git(repo, 'switch', '-qc', 'feature')
            git(repo, 'commit', '--amend', '-qm', 'amended fixture revision')
            amended = git(repo, 'rev-parse', 'HEAD')
            target = repo / history.HISTORY
            target.parent.mkdir(parents=True)
            def record(commit):
                target.write_text('### Revision 70 - fixture\n- **Git commit:** pending\n\n### Revision 69 - fixture\n- **Git commit:** `' + commit + '`\n', 'utf-8')
            record(original)
            self.assertEqual(history.check_history(repo, "main"), [])
            record(amended)
            self.assertTrue(any('ancestry' in e for e in history.check_history(repo, "main")))
            record(original)
            record('pending')
            self.assertTrue(any('latest entry' in e for e in history.check_history(repo, "main")))
            record(original)
            target.write_text(target.read_text().replace('Revision 69', 'Revision 71'))
            self.assertTrue(any('matching explicit source marker' in e for e in history.check_history(repo, "main")))

    def test_define_era_finalized_entry_uses_single_xml_revision(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            write_revision_define(repo, 153)
            committed = commit_fixture(repo, 'define fixture revision')
            git(repo, 'branch', 'main')
            target = repo / history.HISTORY
            target.parent.mkdir(parents=True)
            target.write_text('### Revision 153 - fixture\n- **Git commit:** `' + committed + '`\n', 'utf-8')
            self.assertEqual(history.check_history(repo, "main"), [])
            target.write_text(target.read_text().replace('Revision 153', 'Revision 154'))
            self.assertTrue(any('SAS_GAME_RECORD_REVISION=154' in e for e in history.check_history(repo, "main")))

    def test_progression_requires_exactly_one_increment_for_qualifying_commit(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            write_revision_define(repo, 153)
            source = repo / 'CvGameCoreDLL/SASGameRecordLog.cpp'
            source.parent.mkdir(parents=True)
            source.write_text('// base\n')
            commit_fixture(repo, 'base')
            git(repo, 'branch', 'main')
            git(repo, 'switch', '-qc', 'feature')
            source.write_text('// changed recorder implementation\n')
            write_revision_define(repo, 154)
            commit_fixture(repo, 'qualifying +1')
            self.assertEqual(history.check_progression(repo, 'main'), [])

    def test_progression_rejects_missing_double_and_regressing_bumps(self):
        scenarios = ((153, 'requires exactly one revision increment'), (155, 'revision jumped'), (152, 'revision regressed'))
        for value, expected in scenarios:
            with self.subTest(value=value), tempfile.TemporaryDirectory() as temp:
                repo = Path(temp)
                fixture_git(repo)
                write_revision_define(repo, 153)
                source = repo / 'CvGameCoreDLL/SASGameRecordLog.cpp'
                source.parent.mkdir(parents=True)
                source.write_text('// base\n')
                commit_fixture(repo, 'base')
                git(repo, 'branch', 'main')
                git(repo, 'switch', '-qc', 'feature')
                source.write_text('// qualifying change\n')
                write_revision_define(repo, value)
                commit_fixture(repo, 'bad progression')
                errors = history.check_progression(repo, 'main')
                self.assertTrue(any(expected in error for error in errors), errors)

    def test_progression_checks_each_commit_since_main(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            write_revision_define(repo, 153)
            source = repo / 'CvGameCoreDLL/SASGameRecordLog.cpp'
            source.parent.mkdir(parents=True)
            source.write_text('// base\n')
            commit_fixture(repo, 'base')
            git(repo, 'branch', 'main')
            git(repo, 'switch', '-qc', 'feature')
            source.write_text('// first recorder change\n')
            write_revision_define(repo, 154)
            commit_fixture(repo, 'revision 154')
            other = repo / 'README.md'
            other.write_text('docs only\n')
            commit_fixture(repo, 'unrelated middle commit')
            source.write_text('// second recorder change\n')
            write_revision_define(repo, 155)
            commit_fixture(repo, 'revision 155')
            self.assertEqual(history.check_progression(repo, 'main'), [])

    def test_progression_recognizes_bridge_marker_change(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            write_revision_define(repo, 153)
            bridge = repo / 'Assets/Python/BUG/BugEventManager.py'
            bridge.parent.mkdir(parents=True)
            bridge.write_text('pass\n')
            commit_fixture(repo, 'base')
            git(repo, 'branch', 'main')
            git(repo, 'switch', '-qc', 'feature')
            bridge.write_text('SASGameRecordSettings.logSessionSettings("GAME_START")\n')
            write_revision_define(repo, 154)
            commit_fixture(repo, 'bridge recorder change')
            self.assertEqual(history.check_progression(repo, 'main'), [])

    def test_progression_recognizes_nonrevision_sas_define_change(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            defines = repo / history.DEFINES
            defines.parent.mkdir(parents=True, exist_ok=True)
            defines.write_text('<Civ4Defines><Define><DefineName>SAS_GAME_RECORD_REVISION</DefineName><iDefineIntVal>153</iDefineIntVal></Define><Define><DefineName>SAS_GAME_RECORD_LOG_LEVEL</DefineName><iDefineIntVal>0</iDefineIntVal></Define></Civ4Defines>', 'utf-8')
            commit_fixture(repo, 'base')
            git(repo, 'branch', 'main')
            git(repo, 'switch', '-qc', 'feature')
            defines.write_text('<Civ4Defines><Define><DefineName>SAS_GAME_RECORD_REVISION</DefineName><iDefineIntVal>154</iDefineIntVal></Define><Define><DefineName>SAS_GAME_RECORD_LOG_LEVEL</DefineName><iDefineIntVal>1</iDefineIntVal></Define></Civ4Defines>', 'utf-8')
            commit_fixture(repo, 'recorder define change')
            self.assertEqual(history.check_progression(repo, 'main'), [])

    def test_progression_rejects_revision_only_bump(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            write_revision_define(repo, 153)
            commit_fixture(repo, 'base')
            git(repo, 'branch', 'main')
            git(repo, 'switch', '-qc', 'feature')
            write_revision_define(repo, 154)
            commit_fixture(repo, 'revision only')
            errors = history.check_progression(repo, 'main')
            self.assertTrue(any('without an automatically recognized qualifying' in error for error in errors), errors)

    def test_progression_accepts_unrelated_commit_without_bump(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            write_revision_define(repo, 153)
            commit_fixture(repo, 'base')
            git(repo, 'branch', 'main')
            git(repo, 'switch', '-qc', 'feature')
            other = repo / 'README.md'
            other.write_text('unrelated docs\n')
            commit_fixture(repo, 'unrelated')
            self.assertEqual(history.check_progression(repo, 'main'), [])

    def test_push_checks_bad_earlier_commit_even_when_main_is_already_at_tip(self):
        for revision in (153, 154):
            with self.subTest(revision=revision), tempfile.TemporaryDirectory() as temp:
                repo = Path(temp)
                fixture_git(repo)
                write_revision_define(repo, 153)
                source = repo / 'CvGameCoreDLL/SASGameRecordLog.cpp'
                source.parent.mkdir(parents=True)
                source.write_text('// base\n')
                before = commit_fixture(repo, 'base')
                git(repo, 'switch', '-qc', 'pushed')
                source.write_text('// recorder change\n')
                write_revision_define(repo, revision)
                changed = commit_fixture(repo, 'recorder change')
                (repo / 'README.md').write_text('unrelated final commit\n')
                after = commit_fixture(repo, 'docs')
                git(repo, 'branch', '-f', 'main', after)
                event = {'before': before, 'after': after}
                self.assertEqual(history.progression_commits(repo, after, 'push', event), [changed, after])
                errors = history.check_progression(repo, 'main', 'push', event)
                if revision == 153:
                    self.assertTrue(any('requires exactly one revision increment' in error for error in errors), errors)
                else:
                    self.assertEqual(errors, [])

    def test_pr_uses_explicit_head_and_push_keeps_real_merge(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            write_revision_define(repo, 153)
            source = repo / 'CvGameCoreDLL/SASGameRecordLog.cpp'
            source.parent.mkdir(parents=True)
            source.write_text('// base\n')
            commit_fixture(repo, 'base')
            git(repo, 'branch', 'main')
            git(repo, 'switch', '-qc', 'feature')
            source.write_text('// recorder change\n')
            write_revision_define(repo, 154)
            feature = commit_fixture(repo, 'revision 154')
            git(repo, 'switch', 'main')
            (repo / 'README.md').write_text('main changed separately\n')
            base = commit_fixture(repo, 'main docs')
            git(repo, 'checkout', '--detach', base)
            git(repo, 'merge', '--no-ff', 'feature', '-m', 'merge fixture')
            merged = git(repo, 'rev-parse', 'HEAD')
            pull = {'pull_request': {'base': {'sha': base}, 'head': {'sha': feature}}}
            self.assertEqual(history.progression_commits(repo, base, 'pull_request', pull), [feature])
            self.assertEqual(history.check_progression(repo, 'main', 'pull_request', pull), [])
            git(repo, 'branch', '-f', 'main', merged)
            push = {'before': base, 'after': merged}
            self.assertEqual(history.progression_commits(repo, merged, 'push', push), [merged])
            self.assertEqual(history.check_progression(repo, 'main', 'push', push), [])
            created = {'before': '0' * 40, 'after': feature}
            self.assertEqual(history.progression_commits(repo, base, 'push', created), [feature])

    def test_missing_event_boundary_or_history_is_reported(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            write_revision_define(repo, 153)
            commit_fixture(repo, 'base')
            for event, data, expected in (
                ('push', {}, 'before and after'),
                ('pull_request', {}, 'base and head'),
                ('push', {'before': 'f' * 40, 'after': git(repo, 'rev-parse', 'HEAD')}, 'fetch complete'),
            ):
                errors = history.check_progression(repo, event_name=event, event_data=data)
                self.assertTrue(any(expected in error for error in errors), errors)

    def test_missing_default_branch_abstains_from_ancestry(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            target = repo / history.HISTORY
            target.parent.mkdir(parents=True)
            target.write_text('### Revision 68 - fixture\n- **Git commit:** `0000000000000000000000000000000000000000`\n', 'utf-8')
            self.assertEqual(history.check_history(repo, "refs/remotes/origin/missing"), [])

if __name__ == "__main__":
    unittest.main()
