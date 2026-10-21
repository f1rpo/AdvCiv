# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: pathlib/sys locate the production checker, tempfile isolates XML fixtures, and unittest verifies real field lengths and loader file families. (GPT-6.1-Sol) -->
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / ".github/workflows/build/xml"))
# <!-- custom: Import global_define_lengths so boundary fixtures exercise the same counting and file discovery used by CI. (GPT-6.1-Sol) -->
import global_define_lengths as lengths


class GlobalDefineLengthTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.repo = Path(self.temporary.name)

    def write_define(self, relative: str, name: str, text: str = "value") -> None:
        target = self.repo / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(
            '<Civ4Defines><Define><DefineName>' + name + '</DefineName><DefineTextVal>' + text + '</DefineTextVal></Define></Civ4Defines>',
            encoding="utf-8",
        )

    def errors(self) -> list[str]:
        return lengths.check(self.repo)[0]

    def test_current_repository(self):
        self.assertEqual(lengths.check(ROOT)[0], [])

    def test_exact_255_byte_name_and_text_pass(self):
        self.write_define("Assets/XML/GlobalDefines.xml", "N" * 255, "T" * 255)
        self.assertEqual(self.errors(), [])

    def test_256_byte_name_fails(self):
        self.write_define("Assets/XML/GlobalDefines.xml", "N" * 256)
        self.assertTrue(any("DefineName is 256 UTF-8 bytes" in error for error in self.errors()))

    def test_256_byte_text_fails(self):
        self.write_define("Assets/XML/GlobalDefines.xml", "SAFE_NAME", "T" * 256)
        self.assertTrue(any("DefineTextVal is 256 UTF-8 bytes" in error for error in self.errors()))

    def test_counts_utf8_bytes_not_characters(self):
        self.write_define("Assets/XML/GlobalDefines.xml", "SAFE_NAME", "é" * 128)
        self.assertTrue(any("DefineTextVal is 256 UTF-8 bytes" in error for error in self.errors()))

    def test_surrounding_whitespace_counts_for_both_fields(self):
        self.write_define("Assets/XML/GlobalDefines.xml", " " + "N" * 255, "T" * 255 + " ")
        errors = self.errors()
        self.assertTrue(any("DefineName is 256 UTF-8 bytes" in error for error in errors))
        self.assertTrue(any("DefineTextVal is 256 UTF-8 bytes" in error for error in errors))

    def test_python_callback_defines_are_checked(self):
        self.write_define("Assets/XML/PythonCallbackDefines.xml", "N" * 256)
        self.assertTrue(any("PythonCallbackDefines.xml" in error for error in self.errors()))

    def test_modular_define_files_are_checked(self):
        self.write_define("Assets/Modules/Test/Synthetic_GlobalDefines.xml", "SAFE_NAME", "T" * 256)
        self.write_define("Assets/Modules/Test/Synthetic_PythonCallbackDefines.xml", "SAFE_NAME_2", "ok")
        errors = self.errors()
        self.assertTrue(any("Synthetic_GlobalDefines.xml" in error for error in errors))
        self.assertEqual(len(lengths.loaded_define_paths(self.repo)), 2)

    # <!-- custom: Generic prefixes reproduce the missing _PART_1 layout without coupling the guard to SAS map lists; valid cross-file families and repeated override names remain legal. (GPT-6.1-Sol) -->
    def test_numbered_family_errors(self):
        for names, expected in (
            (("EXAMPLE_LIST", "EXAMPLE_LIST_PART_2"), ("unsuffixed base", "missing EXAMPLE_LIST_PART_1")),
            (("EXAMPLE_LIST", "EXAMPLE_LIST_PART_1"), ("unsuffixed base",)),
            (("EXAMPLE_LIST_PART_1",), ("only one numbered key",)),
            (("EXAMPLE_LIST_PART_2",), ("only one numbered key",)),
            (("EXAMPLE_LIST_PART_1", "EXAMPLE_LIST_PART_3"), ("missing EXAMPLE_LIST_PART_2",)),
            (("EXAMPLE_LIST_PART_2", "EXAMPLE_LIST_PART_3"), ("missing EXAMPLE_LIST_PART_1",)),
            (("EXAMPLE_LIST", "EXAMPLE_LIST_PART_1", "EXAMPLE_LIST_PART_2"), ("unsuffixed base",)),
            (("EXAMPLE_LIST_PART_0",), ("positive, unpadded",)),
            (("EXAMPLE_LIST_PART_01",), ("positive, unpadded",)),
        ):
            with self.subTest(names=names):
                errors = lengths.part_family_errors({name: Path("GlobalDefines.xml") for name in names})
                for message in expected:
                    self.assertTrue(any(message in error for error in errors), errors)

    def test_valid_families_across_files_and_overrides(self):
        self.write_define("Assets/XML/GlobalDefinesAlt.xml", "EXAMPLE_LIST_PART_1")
        self.write_define("Assets/Modules/Test/Synthetic_GlobalDefines.xml", "EXAMPLE_LIST_PART_2")
        self.write_define("Assets/XML/GlobalDefines_devel.xml", "EXAMPLE_LIST_PART_1")
        self.assertEqual(self.errors(), [])

    def test_unrelated_names_are_not_grouped(self):
        names = ("EXAMPLE_LIST", "EXAMPLE_LIST_ENABLE", "EXAMPLE_LIST_PART_LABEL", "OTHER_PART_1", "OTHER_PART_2")
        self.assertEqual(lengths.part_family_errors({name: Path("GlobalDefines.xml") for name in names}), [])

    def test_single_value_uses_unsuffixed_name(self):
        self.assertEqual(lengths.part_family_errors({"EXAMPLE_LIST": Path("GlobalDefines.xml")}), [])


if __name__ == "__main__":
    unittest.main()
