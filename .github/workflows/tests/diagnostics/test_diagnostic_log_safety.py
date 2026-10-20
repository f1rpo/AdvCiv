# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Path/sys load the actual diagnostic checker; unittest exercises its scope without building or mocking the game. (GPT-6.1-Sol) -->
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[4] / ".github/workflows/build/diagnostics"))
# <!-- custom: Import the same checker as CI so the observed legacy-bridge false positive and real owned-writer violations exercise one implementation. (GPT-6.1-Sol) -->
import diagnostic_log_safety as policy


class DiagnosticSinkPolicyTests(unittest.TestCase):
    flags = "FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;\n"
    bridge = 'void logSASCiv4LegacyLogRow(char const* name) { gDLL->logMsg(name, "row"); }\n'

    def test_legacy_bridge_is_allowed_only_in_shared_utils(self):
        self.assertEqual(policy.check_source(policy.UTILS_SOURCE, self.flags + self.bridge), [])
        errors = policy.check_source(policy.BBAI_SOURCE, self.bridge + "logSASDiagnosticLiteralLine();")
        self.assertTrue(any("EXE logger" in error for error in errors))

    def test_owned_writer_and_unrelated_function_still_reject_exe_calls(self):
        for name in ("logSASDiagnosticLiteralLine", "otherFunction"):
            with self.subTest(name=name):
                source = self.flags + self.bridge + 'void %s() { gDLL -> logMsg ("BBAI.log", "row"); }' % name
                self.assertTrue(any("EXE logger" in error for error in policy.check_source(policy.UTILS_SOURCE, source)))

    def test_strings_and_comments_do_not_change_bridge_boundaries(self):
        bridge = 'void logSASCiv4LegacyLogRow() { if (ready) { const char* s = "}"; /* } */ gDLL->logMsg("xml.log", s); } }\n'
        source = self.flags + bridge + '// gDLL->logMsg("example", "text");\n'
        self.assertEqual(policy.check_source(policy.UTILS_SOURCE, source), [])
        source += 'void writer() { gDLL->logMsg("BBAI.log", "row"); }'
        self.assertTrue(any("EXE logger" in error for error in policy.check_source(policy.UTILS_SOURCE, source)))

    def test_unclosed_bridge_does_not_hide_subsequent_code(self):
        source = self.flags + 'void logSASCiv4LegacyLogRow() { gDLL->logMsg("xml.log", "row");'
        self.assertTrue(any("EXE logger" in error for error in policy.check_source(policy.UTILS_SOURCE, source)))

    def test_bridge_does_not_exempt_disk_flush_or_missing_sharing_flags(self):
        source = self.bridge.replace('gDLL->logMsg(name, "row");', 'FlushFileBuffers(handle);')
        errors = policy.check_source(policy.UTILS_SOURCE, source)
        self.assertTrue(any("physical-disk" in error for error in errors))
        self.assertTrue(any("sharing policy" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
