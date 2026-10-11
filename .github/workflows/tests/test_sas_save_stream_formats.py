# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Ensure shared writer markers cannot become diagnostic-only labels after a serializer reverts to a separate literal. (GPT-6.1-Sol) -->
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / ".github/workflows/build"))
import sas_game_record_log as recorder


class SaveStreamFormatTests(unittest.TestCase):
    def test_current_repository_bindings(self):
        self.assertEqual(recorder.check_save_component_formats(ROOT), [])

    def fixture_failures(self, writer_marker, extra_entry="", row_scope="CURRENT_WRITER_MARKERS", extra_sources=None):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            dll = root / "CvGameCoreDLL"
            dll.mkdir()
            (dll / "SASSaveStreamFormat.h").write_text("DO(CV_CITY, CvCity, 17)\n" + extra_entry, encoding="utf-8")
            (dll / "CvCity.cpp").write_text("void CvCity::write(FDataStreamBase* pStream)\n{\n uint uiFlag = " + writer_marker + ";\n pStream->Write(uiFlag);\n}\n", encoding="utf-8")
            (root / recorder.REVISION_SOURCE).write_text("SAS_SAVE_STREAM_FORMATS(SAS_SAVE_STREAM_MARKER_ROW)\nscope=" + row_scope + " markers=%s\n", encoding="utf-8")
            for name, source in (extra_sources or {}).items():
                (dll / name).write_text(source, encoding="utf-8")
            return recorder.check_save_component_formats(root)

    def test_literal_drift_is_rejected(self):
        self.assertEqual(self.fixture_failures("SASSaveStreamFormat::CV_CITY"), [])
        self.assertTrue(any("writer must use" in error for error in self.fixture_failures("17")))

    def test_duplicate_registry_labels_are_rejected(self):
        errors = self.fixture_failures("SASSaveStreamFormat::CV_CITY", "DO(OTHER, CvCity, 17)\n")
        self.assertTrue(any("duplicate" in error for error in errors))

    # <!-- custom: New serializers must not evade registry checks by existing outside the original entry list; XML-info cache writers intentionally stay separate. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
    def test_unregistered_writer_is_rejected(self):
        errors = self.fixture_failures("SASSaveStreamFormat::CV_CITY", extra_sources={
            "NewSubsystem.cpp": "void NewSubsystem::write(FDataStreamBase* pStream)\n{\n uint uiFlag = 18;\n pStream->Write(uiFlag);\n}\n"})
        self.assertTrue(any("NewSubsystem: writer marker missing" in error for error in errors))

    def test_unregistered_xml_info_cache_is_excluded(self):
        self.assertEqual(self.fixture_failures("SASSaveStreamFormat::CV_CITY", extra_sources={
            "CvInfo_Example.cpp": "void CvExampleInfo::write(FDataStreamBase* pStream)\n{\n uint uiFlag = 0;\n pStream->Write(uiFlag);\n}\n"}), [])

    def test_later_literal_override_is_rejected(self):
        errors = self.fixture_failures("SASSaveStreamFormat::CV_CITY; uiFlag = 17")
        self.assertTrue(any("writer must use" in error for error in errors))

    def test_current_writer_scope_cannot_claim_loaded_markers(self):
        errors = self.fixture_failures("SASSaveStreamFormat::CV_CITY", row_scope="LOADED_MARKERS")
        self.assertTrue(any("current-writer scope" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
