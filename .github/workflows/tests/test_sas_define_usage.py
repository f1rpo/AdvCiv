# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Mutate declarations and lookup sites independently so the audit rejects drift, comment-only references and missing dynamic-family members while accepting real forwarding helpers and aliases. (GPT-6.1-Sol) -->

from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / ".github/workflows/build"))
import sas_define_usage as usage


class DefineUsageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.repo = Path(self.temp.name)
        for name, consumer in usage.METADATA.items():
            self.write(consumer, 'COVERAGE = ["' + name + '"]')

    def write(self, path, text):
        target = self.repo / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text, encoding="utf-8")

    def declarations(self, *names):
        entries = ''.join('<Define><DefineName>' + name + '</DefineName><iDefineIntVal>1</iDefineIntVal></Define>' for name in (*names, *usage.METADATA))
        self.write(usage.DEFINES_PATH, '<GlobalDefines xmlns="fixture">' + entries + '</GlobalDefines>')

    def errors(self):
        return usage.check(self.repo)[0]

    def test_current_repository(self):
        self.assertEqual(usage.check(ROOT)[0], [])

    def test_direct_lookup_and_non_define_sas_names(self):
        self.declarations("SAS_ACTIVE")
        self.write('Assets/Python/live.py', 'gc.getDefineINT("SAS_ACTIVE")\nwidget = "SAS_WIDGET"\nSAS_LOCAL_CONSTANT = 1\n')
        self.assertEqual(self.errors(), [])

    def test_missing_lookup_declaration(self):
        self.declarations()
        self.write('PrivateMaps/live.py', 'gc.getDefineFLOAT("SAS_MISSING")')
        self.assertTrue(any('SAS_MISSING has no declaration' in e for e in self.errors()))

    def test_comments_docs_and_tests_do_not_satisfy_usage(self):
        self.declarations("SAS_UNUSED")
        self.write('Assets/Python/live.py', '# gc.getDefineINT("SAS_UNUSED")\nx = "# still a string"')
        self.write('CvGameCoreDLL/live.cpp', '// GC.getDefineBOOL("SAS_UNUSED");\n/* GC.getDefineBOOL("SAS_UNUSED"); */')
        self.write('README.md', 'GC.getDefineINT("SAS_UNUSED")')
        self.write('.github/workflows/tests/noise.py', 'gc.getDefineINT("SAS_UNUSED")')
        self.assertTrue(any('SAS_UNUSED has no runtime' in e for e in self.errors()))

    def test_cpp_forwarding_helper_discovered_in_header(self):
        self.declarations("SAS_ACTIVE")
        self.write('CvGameCoreDLL/helper.h', 'int lookup(char const* key) { return GC.getDefineINT(key); }')
        self.write('CvGameCoreDLL/live.cpp', 'int x = lookup("SAS_ACTIVE");')
        self.assertEqual(self.errors(), [])

    def test_python_forwarding_and_chained_aliases(self):
        self.declarations("SAS_ACTIVE")
        self.write('Assets/Python/helper.py', 'def lookup(key):\n    return gc.getDefineSTRING(key)\n')
        self.write('Assets/Python/live.py', 'FIRST = "SAS_ACTIVE"\nSECOND = FIRST\nx = lookup(SECOND)\n')
        self.assertEqual(self.errors(), [])

    def test_misspelled_helper_lookup(self):
        self.declarations()
        self.write('Assets/Python/live.py', 'def lookup(key):\n    return gc.getDefineINT(key)\nx = lookup("SAS_MISSPELLED")\n')
        self.assertTrue(any('SAS_MISSPELLED has no declaration' in e for e in self.errors()))

    def dynamic_source(self):
        return 'def lookup(level):\n    if level < 2 or level > 3:\n        return 100\n    name = "SAS_FONT_%d" % level\n    return gc.getDefineINT(name)\n'

    def test_dynamic_family_from_source_bounds(self):
        self.declarations("SAS_FONT_2", "SAS_FONT_3")
        self.write('Assets/Python/live.py', self.dynamic_source())
        self.assertEqual(self.errors(), [])

    def test_missing_dynamic_member(self):
        self.declarations("SAS_FONT_2")
        self.write('Assets/Python/live.py', self.dynamic_source())
        self.assertTrue(any('SAS_FONT_3 has no declaration' in e for e in self.errors()))

    def test_unbounded_dynamic_lookup_fails(self):
        self.declarations("SAS_FONT_2")
        self.write('Assets/Python/live.py', 'name = "SAS_FONT_%d" % level\nx = gc.getDefineINT(name)\n')
        self.assertTrue(any('unresolved SAS define-name' in e for e in self.errors()))

    def test_commented_guard_cannot_resolve_dynamic_lookup(self):
        self.declarations("SAS_FONT_2")
        self.write('Assets/Python/live.py', '# if level < 2 or level > 3: return 100\nname = "SAS_FONT_%d" % level\nx = gc.getDefineINT(name)\n')
        self.assertTrue(any('unresolved SAS define-name' in e for e in self.errors()))

    def test_metadata_comment_is_not_consumer(self):
        self.declarations()
        for name, consumer in usage.METADATA.items():
            self.write(consumer, '# "' + name + '"')
        self.assertTrue(any('metadata consumer' in e for e in self.errors()))

    def test_temp_sources_excluded(self):
        self.declarations("SAS_UNUSED")
        self.write('CvGameCoreDLL/Project/temp_files/Debug-opt/live.cpp', 'GC.getDefineINT("SAS_UNUSED");')
        self.assertTrue(any('SAS_UNUSED has no runtime' in e for e in self.errors()))

    def test_duplicate_declaration(self):
        self.declarations("SAS_ACTIVE", "SAS_ACTIVE")
        self.write('PrivateMaps/live.py', 'gc.getDefineINT("SAS_ACTIVE")')
        self.assertTrue(any('duplicate SAS_ACTIVE' in e for e in self.errors()))

    def test_all_xml_value_types(self):
        self.declarations()
        target = self.repo / usage.DEFINES_PATH
        text = target.read_text()
        for name, tag in [('SAS_FLOAT', 'fDefineFloatVal'), ('SAS_STRING', 'DefineTextVal'), ('SAS_BOOL', 'bDefineBoolVal')]:
            text = text.replace('</GlobalDefines>', '<Define><DefineName>' + name + '</DefineName><' + tag + '>1</' + tag + '></Define></GlobalDefines>')
        target.write_text(text)
        self.write('PrivateMaps/live.py', 'gc.getDefineFLOAT("SAS_FLOAT")\ngc.getDefineSTRING("SAS_STRING")\ngc.getDefineBOOL("SAS_BOOL")')
        self.assertEqual(self.errors(), [])

    def test_metadata_consumer_required(self):
        self.declarations()
        for consumer in usage.METADATA.values():
            (self.repo / consumer).unlink()
        self.assertTrue(any('metadata consumer' in e for e in self.errors()))


if __name__ == "__main__":
    unittest.main()
