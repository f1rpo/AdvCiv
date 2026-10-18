# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Path and tempfile build isolated fake project trees; unittest runs the coverage regression cases. (GPT-6.1-Sol) -->
from pathlib import Path
import tempfile
import unittest

# <!-- custom: Exercise the actual coverage checker against those trees rather than reimplementing its matching logic in the tests. (GPT-6.1-Sol) -->
from build.repository import project_sources


class ProjectSourcesTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        source = self.root / "CvGameCoreDLL"
        (source / "Project").mkdir(parents=True)
        (source / "Unit.cpp").write_text("", encoding="utf-8")
        (source / "Unit.h").write_text("", encoding="utf-8")

    def project(self, entries):
        path = self.root / "CvGameCoreDLL/Project/AdvCiv.vcxproj"
        path.write_text('<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003"><ItemGroup>' + entries + '</ItemGroup></Project>', encoding="utf-8")

    def test_complete_coverage_uses_windows_case_rules(self):
        self.project('<ClCompile Include="..\\UNIT.CPP"/><ClInclude Include="..\\unit.H"/><ClInclude Include="resource.h"/>')
        self.assertEqual(project_sources.check(self.root), [])

    def test_reports_all_missing_entries(self):
        self.project("")
        failures = project_sources.check(self.root)
        self.assertEqual(len(failures), 2)
        self.assertTrue(any("Unit.cpp" in item for item in failures))
        self.assertTrue(any("Unit.h" in item for item in failures))

    def test_wrong_item_kind_and_stale_root_entry(self):
        self.project('<ClCompile Include="..\\Unit.cpp"/><ClCompile Include="..\\Unit.h"/><ClInclude Include="..\\Gone.h"/>')
        failures = project_sources.check(self.root)
        self.assertTrue(any("incorrectly classified" in item and "Unit.h" in item for item in failures))
        self.assertTrue(any("Gone.h" in item for item in failures))
        self.assertTrue(any("missing ClInclude" in item for item in failures))

    def test_generated_or_nested_sources_do_not_require_root_entries(self):
        source = self.root / "CvGameCoreDLL/Project/temp_files/Release"
        source.mkdir(parents=True)
        (source / "Generated.h").write_text("", encoding="utf-8")
        self.project('<ClCompile Include="..\\Unit.cpp"/><ClInclude Include="..\\Unit.h"/>')
        self.assertEqual(project_sources.check(self.root), [])


if __name__ == "__main__":
    unittest.main()
