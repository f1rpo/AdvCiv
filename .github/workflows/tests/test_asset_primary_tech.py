# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
# <!-- custom: Restore blocking checks in the automated subject folders; regression fixtures import those same helpers. (GPT-6.1-Sol) -->
sys.path.insert(0, str(ROOT / ".github/workflows/build/xml"))
import asset_primary_tech as tech

class XmlFixture(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.repo = Path(self.temporary.name)
        # <!-- custom: Use invented resources/technologies with controlled graph relationships.
        # Real resource renames and balance changes must not invalidate these checker regressions. (GPT-6.1-Sol) -->
        fixtures = {
            'Technologies/CIV4TechInfos.xml': '<Civ4TechInfos><TechInfos><TechInfo><Type>TECH_CI_PRIMARY</Type><Era>ERA_CI</Era><iGridX>1</iGridX><AndPreReqs/><OrPreReqs/></TechInfo><TechInfo><Type>TECH_CI_PARALLEL</Type><Era>ERA_CI</Era><iGridX>1</iGridX><AndPreReqs/><OrPreReqs/></TechInfo><TechInfo><Type>TECH_CI_LATER</Type><Era>ERA_CI</Era><iGridX>4</iGridX><AndPreReqs><PrereqTech>TECH_CI_PRIMARY</PrereqTech></AndPreReqs><OrPreReqs/></TechInfo></TechInfos></Civ4TechInfos>',
            'GameInfo/CIV4EraInfos.xml': '<Civ4EraInfos><EraInfos><EraInfo><Type>ERA_CI</Type></EraInfo></EraInfos></Civ4EraInfos>',
            'Units/CIV4UnitInfos.xml': '<Civ4UnitInfos><UnitInfos><UnitInfo><Type>UNIT_CI</Type><PrereqTech>TECH_CI_PRIMARY</PrereqTech><TechTypes/></UnitInfo></UnitInfos></Civ4UnitInfos>',
            'Buildings/CIV4BuildingInfos.xml': '<Civ4BuildingInfos><BuildingInfos><BuildingInfo><Type>BUILDING_CI</Type><SpecialBuildingType>SPECIALBUILDING_CI</SpecialBuildingType><PrereqTech>NONE</PrereqTech><TechTypes/></BuildingInfo></BuildingInfos></Civ4BuildingInfos>',
            'Buildings/CIV4SpecialBuildingInfos.xml': '<Civ4SpecialBuildingInfos><SpecialBuildingInfos><SpecialBuildingInfo><Type>SPECIALBUILDING_CI</Type><TechPrereq>TECH_CI_LATER</TechPrereq></SpecialBuildingInfo></SpecialBuildingInfos></Civ4SpecialBuildingInfos>',
            'Units/CIV4BuildInfos.xml': '<Civ4BuildInfos><BuildInfos><BuildInfo><Type>BUILD_CI</Type><PrereqTech>TECH_CI_PRIMARY</PrereqTech><ImprovementType>IMPROVEMENT_CI</ImprovementType></BuildInfo></BuildInfos></Civ4BuildInfos>',
            'Terrain/CIV4ImprovementInfos.xml': '<Civ4ImprovementInfos><ImprovementInfos><ImprovementInfo><Type>IMPROVEMENT_CI</Type><bActsAsCity>0</bActsAsCity><BonusTypeStructs><BonusTypeStruct><BonusType>BONUS_CI_ALPHA</BonusType><bBonusTrade>1</bBonusTrade></BonusTypeStruct><BonusTypeStruct><BonusType>BONUS_CI_BETA</BonusType><bBonusTrade>1</bBonusTrade></BonusTypeStruct></BonusTypeStructs></ImprovementInfo></ImprovementInfos></Civ4ImprovementInfos>',
            'Terrain/CIV4BonusInfos.xml': '<Civ4BonusInfos><BonusInfos><BonusInfo><Type>BONUS_CI_ALPHA</Type><TechReveal>NONE</TechReveal><TechCityTrade>TECH_CI_PRIMARY</TechCityTrade></BonusInfo><BonusInfo><Type>BONUS_CI_BETA</Type><TechReveal>TECH_CI_LATER</TechReveal><TechCityTrade>TECH_CI_LATER</TechCityTrade></BonusInfo></BonusInfos></Civ4BonusInfos>',
        }
        for relative, text in fixtures.items():
            target = self.repo / 'Assets/XML' / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(text, 'utf-8')

    def replace(self, relative, old, new):
        path = self.repo / relative
        text = path.read_text('utf-8')
        self.assertTrue(old in text, "missing fixture text: " + old)
        path.write_text(text.replace(old, new, 1), 'utf-8')

class TechTests(XmlFixture):
    def test_generic_shared_and_reveal_requirements(self):
        self.assertEqual(tech.check(self.repo), [])

    def test_current_mod_rules(self):
        self.assertEqual(tech.check(ROOT), [])

    def test_later_additional_unit_requirement(self):
        self.replace('Assets/XML/Units/CIV4UnitInfos.xml', '<TechTypes/>', '<TechTypes><PrereqTech>TECH_CI_LATER</PrereqTech></TechTypes>')
        self.assertTrue(any('precedes required TECH_CI_LATER' in e for e in tech.check(self.repo)))

    def test_same_column_independent_trade_requirement(self):
        self.replace('Assets/XML/Terrain/CIV4BonusInfos.xml', '<TechCityTrade>TECH_CI_PRIMARY</TechCityTrade>', '<TechCityTrade>TECH_CI_PARALLEL</TechCityTrade>')
        self.assertTrue(any('not guaranteed' in e for e in tech.check(self.repo)))

    def test_delayed_trade_requirement_for_any_resource(self):
        self.replace('Assets/XML/Terrain/CIV4BonusInfos.xml', '<TechCityTrade>TECH_CI_PRIMARY</TechCityTrade>', '<TechCityTrade>TECH_CI_LATER</TechCityTrade>')
        self.assertTrue(any('BONUS_CI_ALPHA' in e and 'not guaranteed' in e for e in tech.check(self.repo)))

    def test_build_unlock_changes_are_followed(self):
        self.replace('Assets/XML/Units/CIV4BuildInfos.xml', '<PrereqTech>TECH_CI_PRIMARY</PrereqTech>', '<PrereqTech>TECH_CI_PARALLEL</PrereqTech>')
        self.assertTrue(tech.check(self.repo))
        self.replace('Assets/XML/Terrain/CIV4BonusInfos.xml', '<TechCityTrade>TECH_CI_PRIMARY</TechCityTrade>', '<TechCityTrade>TECH_CI_PARALLEL</TechCityTrade>')
        self.assertEqual(tech.check(self.repo), [])

    def test_renamed_resources_and_technologies_need_no_allowlist(self):
        for path in self.repo.rglob('*.xml'):
            path.write_text(path.read_text('utf-8').replace('_CI', '_RENAMED_TEST'), 'utf-8')
        self.assertEqual(tech.check(self.repo), [])

    def test_every_alternative_connection_path_must_satisfy_gate(self):
        self.replace('Assets/XML/Units/CIV4BuildInfos.xml', '</BuildInfos>', '<BuildInfo><Type>BUILD_CI_ALTERNATIVE</Type><PrereqTech>TECH_CI_PARALLEL</PrereqTech><ImprovementType>IMPROVEMENT_CI</ImprovementType></BuildInfo></BuildInfos>')
        self.assertTrue(any('BONUS_CI_ALPHA' in e and 'not guaranteed' in e for e in tech.check(self.repo)))
        self.replace('Assets/XML/Terrain/CIV4BonusInfos.xml', '<TechCityTrade>TECH_CI_PRIMARY</TechCityTrade>', '<TechCityTrade>NONE</TechCityTrade>')
        self.assertEqual(tech.check(self.repo), [])

    def test_missing_tech_fails(self):
        self.replace('Assets/XML/Units/CIV4BuildInfos.xml', '<PrereqTech>TECH_CI_PRIMARY</PrereqTech>', '<PrereqTech>UNKNOWN_SYNTHETIC_XML_TAG</PrereqTech>')
        # <!-- custom: Build prerequisite validation must reject unknown tags before graph comparisons; an empty guaranteed set alone can silently accept a resource with no trade gate. (GPT-6.1-Sol) -->
        self.assertTrue(any('UNKNOWN_SYNTHETIC_XML_TAG' in e for e in tech.check(self.repo)))


if __name__ == "__main__":
    unittest.main()
