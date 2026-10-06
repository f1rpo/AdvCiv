# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Use real Git commits and binary mutations so the intent marker is required on each offending commit, including roots, merges, multi-commit pushes and amended-history events. (GPT-6.1-Sol) -->

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / ".github/workflows/build"))
import dll_commit_message as checker


class DLLCommitMessageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.repo = Path(self.temp.name)
        self.git('init', '-q')
        self.git('config', 'user.name', 'CI fixture')
        self.git('config', 'user.email', 'fixture@example.invalid')
        self.git('config', 'commit.gpgsign', 'false')
        self.git('config', 'core.autocrlf', 'false')
        self.write('source.cpp', b'int source;')
        self.base = self.commit('Start source repository')

    def git(self, *args, input=None):
        return subprocess.check_output(['git', *args], cwd=self.repo, input=input, stderr=subprocess.DEVNULL).decode().strip()

    def write(self, path, data):
        target = self.repo / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)

    def commit(self, message):
        self.git('add', '-A')
        self.git('commit', '-q', '-F', '-', input=message.encode('utf8'))
        return self.git('rev-parse', 'HEAD')

    def dll_commit(self, message, data=b'\0binary fixture'):
        self.write('Assets/CvGameCoreDLL.dll', data)
        return self.commit(message)

    def test_dll_addition_without_marker_fails(self):
        commit = self.dll_commit('Source improvements')
        errors, count = checker.check(self.repo, [commit])
        self.assertEqual(count, 1)
        self.assertIn('Assets/CvGameCoreDLL.dll', errors[0])
        self.assertIn(commit[:12], errors[0])

    def test_title_and_body_markers_pass(self):
        first = self.dll_commit('Update DLL: install Release build')
        second = self.dll_commit('Finish source work\n\n- Update DLL: intentional Release build', b'new binary')
        self.assertEqual(checker.check(self.repo, [first, second]), ([], 2))

    def test_modification_requires_own_marker(self):
        self.dll_commit('Update DLL')
        bad = self.dll_commit('More source', b'changed binary')
        self.assertEqual(len(checker.check(self.repo, [bad])[0]), 1)

    def test_later_marker_does_not_cover_earlier_commit(self):
        bad = self.dll_commit('Accidental binary')
        self.write('source.cpp', b'int changed;')
        later = self.commit('Update DLL')
        commits = checker.commits_in_range(self.repo, self.base, later)
        self.assertEqual(commits, [bad, later])
        self.assertEqual(len(checker.check(self.repo, commits)[0]), 1)

    def test_source_only_changes_pass_without_marker(self):
        self.write('source.cpp', b'int changed;')
        commit = self.commit('Source only')
        self.assertEqual(checker.check(self.repo, [commit]), ([], 0))

    def test_local_uncommitted_dll_is_not_checked(self):
        self.write('Assets/CvGameCoreDLL.dll', b'local debug build')
        self.assertEqual(checker.check(self.repo, [self.base]), ([], 0))

    def test_uppercase_extension_and_other_folder(self):
        self.write('Other/Alternate.DLL', b'binary')
        commit = self.commit('Accidental backup')
        self.assertEqual(len(checker.check(self.repo, [commit])[0]), 1)

    def test_delete_requires_marker(self):
        self.dll_commit('Update DLL')
        (self.repo / 'Assets/CvGameCoreDLL.dll').unlink()
        commit = self.commit('Remove file')
        self.assertEqual(len(checker.check(self.repo, [commit])[0]), 1)

    def test_rename_away_from_dll_suffix_requires_marker(self):
        self.dll_commit('Update DLL')
        self.git('mv', 'Assets/CvGameCoreDLL.dll', 'Assets/backup.bin')
        commit = self.commit('Rename binary')
        self.assertEqual(checker.dll_paths(self.repo, commit), ['Assets/CvGameCoreDLL.dll'])
        self.assertEqual(len(checker.check(self.repo, [commit])[0]), 1)

    def test_marker_is_exact_case_sensitive_phrase(self):
        for message in ('update dll', 'UpdateDLL', 'Update DLLs', 'Update old DLL'):
            self.assertIsNone(checker.MARKER.search(message))
        self.assertIsNotNone(checker.MARKER.search('- Update DLL: Release build'))

    def test_root_commit_detects_dll(self):
        self.git('checkout', '--orphan', 'root-binary')
        self.git('rm', '-rf', '.')
        commit = self.dll_commit('Root without marker')
        self.assertEqual(len(checker.check(self.repo, [commit])[0]), 1)

    def test_merge_commit_uses_first_parent(self):
        parent = self.git('branch', '--show-current')
        self.git('checkout', '-qb', 'feature')
        self.dll_commit('Update DLL')
        self.git('checkout', parent)
        self.git('merge', '--no-ff', '-m', 'Unmarked merge', 'feature')
        merge = self.git('rev-parse', 'HEAD')
        self.assertEqual(len(checker.check(self.repo, [merge])[0]), 1)

    def test_force_push_uses_new_commits_without_old_base(self):
        commit = self.dll_commit('Accidental binary')
        event = {'before': 'f' * 40, 'after': commit, 'commits': [{'id': commit}]}
        selected = checker.event_commits(self.repo, 'push', event)
        self.assertEqual(selected, [commit])
        self.assertTrue(checker.check(self.repo, selected)[0])

    def test_new_branch_push_uses_payload(self):
        commit = self.dll_commit('Update DLL')
        event = {'before': '0' * 40, 'after': commit, 'commits': [{'id': commit}]}
        self.assertEqual(checker.event_commits(self.repo, 'push', event), [commit])

    def test_push_missing_head_fails(self):
        commit = self.dll_commit('Update DLL')
        with self.assertRaises(RuntimeError):
            checker.event_commits(self.repo, 'push', {'after': commit, 'commits': [{'id': self.base}]})

    def test_large_payload_uses_complete_range(self):
        first = self.dll_commit('Accidental binary')
        second = self.dll_commit('Update DLL', b'new binary')
        event = {'before': self.base, 'after': second, 'commits': [{'id': second}]}
        with patch.object(checker, 'PUSH_PAYLOAD_LIMIT', 1):
            self.assertEqual(checker.event_commits(self.repo, 'push', event), [first, second])

    def test_missing_base_is_clear_failure_not_pass(self):
        with self.assertRaises(RuntimeError):
            checker.commits_in_range(self.repo, 'missing-base', 'HEAD')

    def test_pr_uses_actual_head_and_excludes_base_history(self):
        old = self.dll_commit('Old unmarked binary')
        self.write('source.cpp', b'int changed;')
        head = self.commit('Source only')
        event = {'pull_request': {'base': {'sha': old}, 'head': {'sha': head}}}
        commits = checker.event_commits(self.repo, 'pull_request', event)
        self.assertEqual(commits, [head])
        self.assertEqual(checker.check(self.repo, commits), ([], 0))

    def test_deleted_branch_or_no_new_commits(self):
        self.assertEqual(checker.event_commits(self.repo, 'push', {'deleted': True}), [])
        self.assertEqual(checker.event_commits(self.repo, 'push', {'commits': []}), [])

    def test_amended_commit_is_checked(self):
        original = self.dll_commit('Update DLL')
        self.git('commit', '--amend', '-qm', 'Marker accidentally removed')
        amended = self.git('rev-parse', 'HEAD')
        self.assertNotEqual(original, amended)
        self.assertTrue(checker.check(self.repo, [amended])[0])


if __name__ == '__main__':
    unittest.main()
