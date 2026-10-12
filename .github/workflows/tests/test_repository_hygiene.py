# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / ".github/workflows/build/hygiene"))
import repository_hygiene as hygiene


def git(repo, *args):
    return subprocess.check_output(["git", *args], cwd=repo, stderr=subprocess.DEVNULL).decode().strip()


def fixture_git(repo):
    git(repo, "init", "-q")
    git(repo, "config", "user.name", "CI fixture")
    git(repo, "config", "user.email", "fixture@example.invalid")
    git(repo, "config", "commit.gpgsign", "false")
    git(repo, "config", "core.autocrlf", "false")

class HygieneTests(unittest.TestCase):
    def test_all_bom_signatures_and_long_paths(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            (repo / 'clean.txt').write_bytes(b'clean\n')
            git(repo, 'add', 'clean.txt')
            self.assertEqual(hygiene.check(repo, 'C:/Mods/SAS'), [])
            for prefix, _ in hygiene.BOMS:
                (repo / 'clean.txt').write_bytes(prefix + b'content')
                self.assertTrue(any('BOM' in e for e in hygiene.check(repo, 'C:/Mods/SAS')))
            (repo / 'clean.txt').write_bytes(b'clean\n')
            self.assertTrue(any('installed path' in e for e in hygiene.check(repo, 'C:/' + 'x' * 250)))
            self.assertTrue(hygiene.check(repo, 'C:/' + '\U0001f600' * 126))


    def test_filename_limit_counts_extensions_utf16_and_collects_all_failures(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            accepted = ('a' * 52 + '.txt', '\U0001f600' * 26 + '.txt')
            rejected = ('b' * 53 + '.txt', '\U0001f600' * 27 + '.txt')
            for name in accepted:
                (repo / name).write_bytes(b'clean\n')
                git(repo, 'add', name)
            self.assertEqual(hygiene.check(repo, 'C:/Mods/SAS'), [])
            for name in rejected:
                (repo / name).write_bytes(b'clean\n')
                git(repo, 'add', name)
            errors = hygiene.check(repo, 'C:/Mods/SAS')
            self.assertEqual(len(errors), 2)
            for name in rejected:
                self.assertTrue(any(error.startswith(name + ': filename uses ') for error in errors))
            self.assertTrue(all('including extension (maximum 56)' in error for error in errors))


    def test_visual_studio_utf8_boms_are_preserved(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            for name in ('OtherSolution.sln', 'OtherProject.vcxproj'):
                path = repo / name
                path.write_bytes(b'\xef\xbb\xbfcontent')
                git(repo, 'add', name)
            self.assertEqual(hygiene.check(repo, 'C:/Mods/SAS'), [])
            for signature, encoding in hygiene.BOMS:
                if encoding == 'UTF-8':
                    continue
                (repo / 'OtherProject.vcxproj').write_bytes(signature + b'content')
                self.assertTrue(any('BOM' in error for error in hygiene.check(repo, 'C:/Mods/SAS')))


if __name__ == "__main__":
    unittest.main()
