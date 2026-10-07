# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / ".github/workflows/build"))
import long_comments as comments


class LongCommentsTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.repo = Path(self.temp.name)
        self.archive_dir = self.repo / comments.ARCHIVE_DIR
        self.archive_dir.mkdir(parents=True)

    def write(self, relative, text):
        path = self.repo / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def archive(self, basename, entries):
        parts = ["Long comments fixture", ""]
        for number, source in entries:
            parts.extend((f"===== {source} =====", f"----- Comment #{number} -----", "archived text", ""))
        self.write(comments.ARCHIVE_DIR / basename, "\n".join(parts))

    def complete_archives(self, xml_entries=((1, r"\\Assets\\XML\\live.xml"),), py_entries=((1, r"\\Assets\\Python\\live.py"),)):
        self.archive("Long_Comments_XML.txt", xml_entries)
        self.archive("Long_Comments_py.txt", py_entries)

    def test_bidirectional_valid_markers_and_multiple_ids(self):
        self.complete_archives(xml_entries=((1, r"\\Assets\\XML\\live.xml"), (2, r"\\Assets\\XML\\live.xml")))
        self.write("Assets/XML/live.xml", "<!-- Long_Comments_XML.txt #1, #2 -->")
        self.write("Assets/Python/live.py", "# Long_Comments_py.txt #1")
        self.assertEqual(comments.check(self.repo), [])

    def test_orphaned_archive_entry_fails(self):
        self.complete_archives()
        self.write("Assets/Python/live.py", "# Long_Comments_py.txt #1")
        self.assertTrue(any("XML.txt #1: archived entry is orphaned" in error for error in comments.check(self.repo)))

    def test_missing_archive_entry_fails(self):
        self.complete_archives()
        self.write("Assets/XML/live.xml", "<!-- Long_Comments_XML.txt #2 -->")
        self.write("Assets/Python/live.py", "# Long_Comments_py.txt #1")
        self.assertTrue(any("XML.txt #2: live marker has no archived entry" in error for error in comments.check(self.repo)))

    def test_historical_reference_does_not_keep_entry_alive(self):
        self.complete_archives()
        self.write("_1_AdvCiv-SAS/Docs/git_logs/history.txt", "Long_Comments_XML.txt #1")
        self.write("Assets/Python/live.py", "# Long_Comments_py.txt #1")
        self.assertTrue(any("XML.txt #1: archived entry is orphaned" in error for error in comments.check(self.repo)))

    def test_duplicate_archive_id_fails(self):
        self.complete_archives(xml_entries=((1, "first.xml"), (1, "second.xml")))
        self.write("Assets/XML/live.xml", "<!-- Long_Comments_XML.txt #1 -->")
        self.write("Assets/Python/live.py", "# Long_Comments_py.txt #1")
        self.assertTrue(any("duplicate Comment #1" in error for error in comments.check(self.repo)))

    def test_invalid_utf8_archive_fails(self):
        self.complete_archives()
        self.write("Assets/XML/live.xml", "<!-- Long_Comments_XML.txt #1 -->")
        self.write("Assets/Python/live.py", "# Long_Comments_py.txt #1")
        (self.archive_dir / "Long_Comments_XML.txt").write_bytes(b"----- Comment #1 -----\ninvalid: \x9d\n")
        self.assertTrue(any("archive is not valid UTF-8" in error for error in comments.check(self.repo)))

    def test_c1_control_character_archive_fails(self):
        self.complete_archives()
        self.write("Assets/XML/live.xml", "<!-- Long_Comments_XML.txt #1 -->")
        self.write("Assets/Python/live.py", "# Long_Comments_py.txt #1")
        self.archive("Long_Comments_XML.txt", ((1, r"\Assets\XML\live.xml"),))
        path = self.archive_dir / "Long_Comments_XML.txt"
        path.write_text(path.read_text(encoding="utf-8") + "\u009d\n", encoding="utf-8")
        self.assertTrue(any("replacement/C1 control" in error for error in comments.check(self.repo)))

    def test_current_repository(self):
        self.assertEqual(comments.check(ROOT), [])


if __name__ == "__main__":
    unittest.main()
