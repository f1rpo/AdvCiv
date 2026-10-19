# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Use real Git commits and binary mutations so the intent marker is required on each offending commit, including roots, merges, multi-commit pushes and amended-history events. (GPT-6.1-Sol) -->

from pathlib import Path
# <!-- custom: Construct matching full build reports for real Git fixtures, including stale hashes and rejected Debug-opt evidence. (GPT-6.1-Sol) -->
import hashlib
import importlib
import json
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / ".github/workflows/build/repository"))
# <!-- custom: Load the subject-folder checker after adding its path, without placing an import statement below executable setup. (GPT-6.1-Sol) -->
checker = importlib.import_module("dll_commit_message")


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

    def summary_commit(self, summary_changes=None, include_summary=True, data=b'shipped DLL'):
        self.write('CvGameCoreDLL/Project/build_with_report.ps1', b'# fixture wrapper')
        self.write('Assets/CvGameCoreDLL.dll', data)
        if include_summary:
            summary = dict(configuration='Release', status='SUCCEEDED', request='BUILD',
                           startedUtc='2026-10-12T10:00:00.000Z', endedUtc='2026-10-12T10:01:00.000Z',
                           durationMilliseconds=60000, warningCount=0, errorCount=0, exitCode=0, logFile='AdvCiv.log',
                           dllAfter=dict(sha256=hashlib.sha256(data).hexdigest(), sizeBytes=len(data)),
                           branch='fixture-branch', commit=self.git('rev-parse', 'HEAD'))
            summary.update(summary_changes or {})
            self.write('Assets/CvGameCoreDLL.dll.build.json', json.dumps(summary).encode())
        return self.commit('Update DLL')

    def test_current_shipped_evidence_required_without_binary_changes(self):
        self.summary_commit()
        (self.repo / 'Assets/CvGameCoreDLL.dll.build.json').unlink()
        head = self.commit('Remove JSON without changing DLL')
        self.assertEqual(checker.dll_paths(self.repo, head), [])
        self.assertTrue(checker.shipped_build_errors(self.repo, head))

    def test_source_only_commit_with_matching_evidence_passes(self):
        self.summary_commit()
        self.write('source.cpp', b'int changed;')
        head = self.commit('Source only')
        self.assertEqual(checker.shipped_build_errors(self.repo, head), [])

    def test_local_report_cannot_repair_committed_missing_evidence(self):
        head = self.summary_commit(include_summary=False)
        self.write('Assets/CvGameCoreDLL.dll.build.json', b'{}')
        self.write('Assets/AdvCiv.log', b'local output')
        self.assertTrue(checker.shipped_build_errors(self.repo, head))

    def test_matching_release_summary_passes(self):
        commit = self.summary_commit()
        self.assertEqual(checker.check(self.repo, [commit]), ([], 1))

    def test_successful_release_with_warnings_fails(self):
        commit = self.summary_commit({'warningCount': 2})
        errors, updated = checker.check(self.repo, [commit])
        self.assertEqual(updated, 1)
        self.assertTrue(any('zero warnings (reported 2)' in error for error in errors))
        self.write('source.cpp', b'int changed;')
        head = self.commit('Source only retains warning-bearing DLL')
        self.assertTrue(any('zero warnings' in error for error in checker.shipped_build_errors(self.repo, head)))

    def test_invalid_warning_counts_fail(self):
        for count in (True, -1, '0', None):
            with self.subTest(count=count):
                commit = self.summary_commit({'warningCount': count}, data=repr(count).encode())
                self.assertTrue(any('warningCount must be a nonnegative integer' in error
                                    for error in checker.check(self.repo, [commit])[0]))

    def test_missing_summary_fails(self):
        commit = self.summary_commit(include_summary=False)
        self.assertTrue(checker.check(self.repo, [commit])[0])

    def test_stale_summary_fails_and_working_tree_cannot_repair_it(self):
        self.summary_commit()
        commit = self.dll_commit('Update DLL', b'different DLL')
        self.write('Assets/CvGameCoreDLL.dll', b'shipped DLL')
        self.assertIn('does not match', checker.check(self.repo, [commit])[0][0])

    def test_debug_failed_bad_time_and_invalid_counts_fail(self):
        for fields in ({'configuration': 'Debug-opt'}, {'configuration': '_Release-fast'}, {'configuration': 'Final_Release-fast'}, {'status': 'FAILED'}, {'errorCount': 1},
                       {'durationMilliseconds': True}, {'startedUtc': 'invalid'},
                       {'endedUtc': '2026-10-12T09:00:00Z'}, {'startedUtc': '2026-10-12T10:00:00'}):
            with self.subTest(fields=fields):
                # <!-- custom: Vary both artifacts so these cases isolate invalid build evidence rather than the separate pair-change requirement. (GPT-6.1-Sol) -->
                commit = self.summary_commit(fields, data=json.dumps(fields).encode())
                self.assertTrue(checker.check(self.repo, [commit])[0])

    def test_json_without_committed_full_log_passes(self):
        head = self.summary_commit()
        self.assertFalse((self.repo / 'Assets/AdvCiv.log').exists())
        self.assertEqual(checker.check(self.repo, [head]), ([], 1))
        self.assertEqual(checker.shipped_build_errors(self.repo, head), [])

    def test_deletion_needs_no_summary(self):
        self.summary_commit()
        (self.repo / 'Assets/CvGameCoreDLL.dll').unlink()
        (self.repo / 'Assets/CvGameCoreDLL.dll.build.json').unlink()
        commit = self.commit('Update DLL: remove binary')
        self.assertEqual(checker.check(self.repo, [commit]), ([], 1))

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


    def test_blobless_checkout_preserves_ranges_without_fetching_old_binaries(self):
        # <!-- custom: Full commit/tree history must still catch an unmarked earlier DLL and an unmarked merge without hydrating historical blobs.
        # Exercise a real filtered file-transport clone, including checkout, rather than mocking Git's filtering behavior. (GPT-6.1-Sol) -->
        parent = self.git('branch', '--show-current')
        bad = self.dll_commit('Accidental binary', b'old unmarked binary')
        old_blob = self.git('rev-parse', bad + ':Assets/CvGameCoreDLL.dll')
        marked = self.dll_commit('Update DLL', b'intentional intermediate binary')
        self.git('checkout', '-qb', 'feature')
        feature = self.dll_commit('Update DLL', b'current binary')
        self.git('checkout', parent)
        self.git('merge', '--no-ff', '-m', 'Unmarked merge', 'feature')
        merge = self.git('rev-parse', 'HEAD')
        self.git('config', 'uploadpack.allowFilter', 'true')
        self.git('config', 'uploadpack.allowAnySHA1InWant', 'true')
        clone = self.repo / 'filtered-checkout'
        subprocess.check_call(['git', 'clone', '-q', '--no-local', '--filter=blob:none', '--no-checkout', self.repo.as_uri(), str(clone)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        subprocess.check_call(['git', 'checkout', '-q', 'HEAD'], cwd=clone, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        self.assertEqual((clone / 'Assets/CvGameCoreDLL.dll').read_bytes(), b'current binary')
        missing_before = checker.git(clone, 'rev-list', '--objects', '--all', '--missing=print')
        self.assertIn(('?' + old_blob).encode(), missing_before)
        expected = [bad, marked, feature, merge]
        commits = checker.commits_in_range(clone, self.base, merge)
        self.assertEqual(set(commits), set(expected))
        errors, count = checker.check(clone, commits)
        self.assertEqual(count, 4)
        self.assertEqual(len(errors), 2)
        self.assertTrue(any(bad[:12] in error for error in errors))
        self.assertTrue(any(merge[:12] in error for error in errors))
        pr = {'pull_request': {'base': {'sha': self.base}, 'head': {'sha': merge}}}
        self.assertEqual(checker.event_commits(clone, 'pull_request', pr), commits)
        push = {'before': 'f' * 40, 'after': merge, 'commits': [{'id': commit} for commit in expected]}
        self.assertEqual(checker.event_commits(clone, 'push', push), expected)
        new_branch = dict(push, before='0' * 40)
        self.assertEqual(checker.event_commits(clone, 'push', new_branch), expected)
        truncated = dict(push, before=self.base)
        with patch.object(checker, 'PUSH_PAYLOAD_LIMIT', 1):
            self.assertEqual(checker.event_commits(clone, 'push', truncated), commits)
        missing_after = checker.git(clone, 'rev-list', '--objects', '--all', '--missing=print')
        self.assertEqual(missing_after, missing_before)


if __name__ == '__main__':
    unittest.main()
