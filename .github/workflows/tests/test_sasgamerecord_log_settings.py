# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Prove the iterable scalar recorder-settings profile stays synchronized with integer SAS_GAME_RECORD_* XML Defines without dragging validated text palettes into the same registry. (ChatGPT-5.6-Sol) -->
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / ".github/workflows/build"))
import sas_game_record_log as recorder
from xml_defines import read_global_define_ints


class SASGameRecordLogSettingsTests(unittest.TestCase):
    def test_current_repository_registry_covers_all_scalar_defines(self):
        defines = read_global_define_ints(ROOT)
        self.assertEqual(recorder.check_log_settings_registry(ROOT, defines), [])

    def fixture_failures(self, registry_rows, defines):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / recorder.REVISION_SOURCE
            source.parent.mkdir(parents=True)
            source.write_text(
                "static CvString getSASGameRecordLogSettingFields()\n"
                "{\n SASGameRecordLogSetting const aSettings[] =\n {\n" + registry_rows + "\n };\n return CvString();\n}\n"
                "static void logSASGameRecordLogSettings()\n"
                "{\n logSASGameRecord(\"GAME_RECORD_LOG_SETTINGS %s\", getSASGameRecordLogSettingFields().GetCString());\n}\n"
                "// <!-- custom: One session-start profile\n",
                encoding="utf-8",
            )
            return recorder.check_log_settings_registry(root, defines)

    def test_missing_scalar_define_is_rejected(self):
        errors = self.fixture_failures('{ "SAS_GAME_RECORD_A", 1 },', {"SAS_GAME_RECORD_A": 1, "SAS_GAME_RECORD_B": 2})
        self.assertTrue(any("SAS_GAME_RECORD_B" in error and "missing" in error for error in errors))

    def test_unknown_or_duplicate_registry_entry_is_rejected(self):
        duplicate = self.fixture_failures('{ "SAS_GAME_RECORD_A", 1 },\n{ "SAS_GAME_RECORD_A", 1 },', {"SAS_GAME_RECORD_A": 1})
        self.assertTrue(any("duplicate" in error for error in duplicate))
        unknown = self.fixture_failures('{ "SAS_GAME_RECORD_A", 1 },\n{ "SAS_GAME_RECORD_OLD", 1 },', {"SAS_GAME_RECORD_A": 1})
        self.assertTrue(any("SAS_GAME_RECORD_OLD" in error and "unknown" in error for error in unknown))


if __name__ == "__main__":
    unittest.main()
