# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
import sys
import tempfile
import subprocess
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "LLM_Helpers"))
# <!-- custom: Workflow tests must add the repo-local helper directory before importing this script and its sibling refresh helper.
# The late import is required, so suppress only this E402 finding. (GPT-6.1-Sol) -->
import make_light_source_zip as light  # noqa: E402


class ReferenceHistoryFilteringTests(unittest.TestCase):
    def summarize(self, old, new=None):
        new = old if new is None else new
        label = 'a/' + old + ' b/' + new
        chunk = 'diff --git ' + label + '\n--- a/' + old + '\n+++ b/' + new + '\n-old\n+new\n'
        return light.should_summarize_historical_patch(label, len(chunk.encode()), chunk)

    def test_reference_payloads_in_current_and_former_paths(self):
        for path in ('_0_Common_Docs/BBAI_Doc/efficiency notes.txt', '_0_Common_Docs/BUG_Doc/Help.chm', '_1_AdvCiv-SAS/Docs/changelogs_web/5500.md', '_1_AdvCiv-SAS/Docs/Modding_Ressources/changelogs_web/5500.txt'):
            with self.subTest(path=path):
                self.assertEqual(self.summarize(path), (True, 'imported-reference/archive-document', 2))

    def test_indexes_source_and_maintained_docs_keep_patches(self):
        for path in ('_0_Common_Docs/README.md', '_0_Common_Docs/BBAI_Doc/readme.txt', '_0_Common_Docs/BUG_Doc/CvAltRoot.py', '_1_AdvCiv-SAS/Docs/changelogs_web/README.md', '_1_AdvCiv-SAS/Docs/Modding_Ressources/changelogs_web/README.md', '_1_AdvCiv-SAS/Docs/README_Known_Issues.md', '_1_AdvCiv-SAS/Docs/README_SASGameRecord_Revisions.md', 'LLM_Helpers/README.md'):
            with self.subTest(path=path):
                self.assertFalse(self.summarize(path)[0])

    def test_renames_across_policy_boundary_keep_patches(self):
        archived = '_1_AdvCiv-SAS/Docs/changelogs_web/5500.md'
        maintained = '_1_AdvCiv-SAS/Docs/guide.md'
        self.assertFalse(self.summarize(maintained, archived)[0])
        self.assertFalse(self.summarize(archived, maintained)[0])
        self.assertFalse(self.summarize(archived, '_1_AdvCiv-SAS/Docs/changelogs_web/README.md')[0])
        self.assertTrue(self.summarize('_1_AdvCiv-SAS/Docs/Modding_Ressources/changelogs_web/5500.md', archived)[0])

    def test_render_retains_summaries_path_index_and_cache_validation(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            def git(*args):
                return subprocess.check_output(['git', *args], cwd=repo, stderr=subprocess.DEVNULL).decode().strip()
            git('init', '-q')
            git('config', 'user.name', 'CI fixture')
            git('config', 'user.email', 'fixture@example.invalid')
            git('config', 'commit.gpgsign', 'false')
            git('config', 'core.autocrlf', 'false')
            files = {
                '_0_Common_Docs/BBAI_Doc/efficiency notes.txt': 'IMPORTED_REFERENCE_PAYLOAD\n',
                '_1_AdvCiv-SAS/Docs/changelogs_web/5500.md': 'PUBLISHED_ARCHIVE_PAYLOAD\n',
                '_1_AdvCiv-SAS/Docs/changelogs_web/README.md': '# Maintained index\n',
                '_0_Common_Docs/BUG_Doc/CvAltRoot.py': 'value = 1\n',
                '_1_AdvCiv-SAS/Docs/guide.md': '# Maintained guide\n',
            }
            for relative, content in files.items():
                destination = repo / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(content.encode())
            git('add', '.')
            git('commit', '-qm', 'Reference fixture')
            sha = git('rev-parse', 'HEAD')
            rendered, version, coverage = light.render_commit_diff(repo, {'hash': sha, 'parents': '', 'subject': 'Reference fixture', 'version': '1'})
            self.assertEqual(version, '1')
            self.assertEqual(coverage, 'partial')
            text = rendered.decode()
            self.assertNotIn('IMPORTED_REFERENCE_PAYLOAD', text)
            self.assertNotIn('PUBLISHED_ARCHIVE_PAYLOAD', text)
            self.assertIn('+# Maintained index', text)
            self.assertIn('+value = 1', text)
            self.assertIn('+# Maintained guide', text)
            paths = light.changed_paths_from_commit_diff(rendered)
            self.assertEqual(set(paths), set(files))
            path_index = light.build_path_history_index({path: [('SASBranch', '1', sha[:9])] for path in paths}).decode()
            for path in files:
                self.assertIn(path, path_index)
            cache = repo / 'cached.diff'
            cache.write_bytes(rendered)
            self.assertIsNotNone(light.cached_commit_info(cache, sha))
            with patch.object(light, 'COMMIT_DIFF_REFERENCE_DOCUMENT_DIRS', light.COMMIT_DIFF_REFERENCE_DOCUMENT_DIRS + ('extra_reference/',)):
                self.assertIsNone(light.cached_commit_info(cache, sha))


if __name__ == '__main__':
    unittest.main()
