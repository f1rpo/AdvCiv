# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Exercise the source-derived checker without importing Civ4 or maintaining a parallel mock engine; real provider declarations/XML/enums supply the API, and small source strings represent valid and broken consumers. (GPT-6.1-Sol) -->
from pathlib import Path
import sys
import unittest

# <!-- custom: Import the same source-category checker used by checks-source.yml; fixtures cover the replacement for the earlier namespace/font-specific guards. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[4] / ".github/workflows/build/source"))
import python_resource_api as api

ROOT = Path(__file__).resolve().parents[4]


class PythonResourceApiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.sources = {name: api.read(ROOT / "Assets/Python" / (name + ".py")) for name in api.PROVIDERS}
        cls.xml, cls.enums = api.data_sources(ROOT)

    def inspect(self, source, extra=None, sources=None):
        definitions = dict(sources or self.sources)
        definitions.update(extra or {})
        definitions["Consumer"] = source
        catalog = api.Catalog(definitions, self.xml, self.enums)
        return api.inspect_consumer("Consumer", catalog)

    def test_all_real_collection_families_reject_missing_members(self):
        catalog = api.Catalog(self.sources, self.xml, self.enums)
        tree = catalog.trees["SASVisualUtils"]
        families = [node.targets[0].id for node in tree.body
                    if isinstance(node, api.ast.Assign) and isinstance(node.targets[0], api.ast.Name)
                    and isinstance(node.value, api.ast.Call) and isinstance(node.value.func, api.ast.Name)
                    and ("SASVisualUtils", node.value.func.id) in catalog.classes]
        self.assertGreater(len(families), 10)
        for name in families:
            with self.subTest(collection=name):
                errors = self.inspect("import SASVisualUtils as V\nV.%s.NOT_A_REAL_MEMBER\n" % name)
                self.assertTrue(any("missing API member" in message for _line, message in errors), errors)

    def test_data_derived_and_inherited_valid_members(self):
        # <!-- custom: The fixture imports the resource provider for glyph/art collections and SASTextScale for font-tag objects; no gameplay modules are executed. (GPT-6.1-Sol) -->
        source = """import SASVisualUtils as V
import SASTextScale as Text
V.Font.HAPPY_CHAR
V.DynamicFont.WAR_CHAR
V.DynamicFont.YIELD_FOOD_CHAR
V.DynamicFont.COMMERCE_GOLD_CHAR
V.Yield.FOOD_CHAR
V.Commerce.RESEARCH_CHAR
V.Religion.holyCityChar(0)
V.Corporation.headquarterChar(0)
V.Bonus.ordinals()
V.Color.RED
V.Trait.getIcon(0)
V.GreatPeople.unitTypes()
V.Specialist.icons()
V.CityEmphasis.AVOID_GROWTH
V.CivilizationArt.button(0)
V.StaticArt.UNHAPPY_PERSON
Text.sasFontTagTitle.bold
Text.labelText("text")
"""
        for getter, family in (("getInterfaceArtInfo", "InterfaceArt"), ("getMovieArtInfo", "MovieArt")):
            source += "V.%s.%s\n" % (family, sorted(self.xml[getter])[0])
        self.assertEqual(self.inspect(source), [])

    def test_actual_reported_failure_expressions(self):
        # <!-- custom: Retain the exact broken assignment and F3 attribute expression from the reported failures, including a store target: validating only reads would miss the first startup NameError. (GPT-6.1-Sol) -->
        first = 'import SASVisualUtils as SASVisuals\nUNSASVisuals.StaticArt.HEALTHY_PERSON = SASVisuals.StaticArt.UNHEALTHY_PERSON\n'
        self.assertTrue(any("unbound namespace UNSASVisuals" in message for _line, message in self.inspect(first)))
        second = 'import CvForeignAdvisor\nCvForeignAdvisor.sasFontTagTitle.bold\n'
        self.assertTrue(any("missing API member CvForeignAdvisor.sasFontTagTitle" in message for _line, message in self.inspect(second, {"CvForeignAdvisor": "localText = None\n"})))

    def test_original_failures_and_other_namespace_spellings(self):
        for root in ("UNSASVisuals", "NotImported", "OldResources"):
            self.assertTrue(self.inspect(root + ".StaticArt.UNHAPPY_PERSON\n"))
        self.assertTrue(self.inspect("import Advisor\nAdvisor.sasFontTagTitle.bold\n", {"Advisor": "localText = None\n"}))
        self.assertTrue(self.inspect("import SASVisualUtils as V\nV.Font.ordinals()\n"))

    def test_import_aliases_from_imports_and_valid_reexports(self):
        self.assertEqual(self.inspect('from SASVisualUtils import Trait as Icons\nIcons.getIcon(0)\n'), [])
        self.assertEqual(self.inspect('import Advisor as A\nA.title.bold\n', {"Advisor": "from SASTextScale import sasFontTagTitle as title\n"}), [])
        self.assertEqual(self.inspect('from Bridge import *\nV.Trait.getIcon(0)\n', {"Bridge": "import SASVisualUtils as V\n"}), [])
        self.assertTrue(self.inspect('from Bridge import *\nV.Trait.notAMethod(0)\n', {"Bridge": "import SASVisualUtils as V\n"}))
        self.assertTrue(self.inspect('from SASVisualUtils import NotACollection\n'))
        self.assertTrue(self.inspect('import SASVisualUtils as V\nV.NotACollection.char(0)\n'))

    def test_wrong_calls_and_values(self):
        for expression in ('V.Trait.getIcon()', 'V.Commerce.char(0, 1)', 'V.Bonus.char(wrongKeyword=0)', 'V.Font.HAPPY_CHAR()', 'V.Trait()'):
            with self.subTest(expression=expression):
                self.assertTrue(self.inspect('import SASVisualUtils as V\n' + expression + '\n'))
        self.assertEqual(self.inspect('import SASVisualUtils as V\nV.Trait.getIcon(*args)\n'), [])

    def test_dynamic_data_rejects_absent_names(self):
        for expression in ('V.Color.NOT_A_COLOR', 'V.InterfaceArt.NOT_AN_ART_DEFINE', 'V.DynamicFont.NOT_A_REGISTERED_CHAR', 'V.DynamicFont.MAX_NUM_SYMBOLS'):
            with self.subTest(expression=expression):
                self.assertTrue(self.inspect('import SASVisualUtils as V\n' + expression + '\n'))

    def test_provider_edit_changes_contract_without_checker_edits(self):
        sources = dict(self.sources)
        sources["SASVisualUtils"] += '\nclass NewCollection:\n    def perform(self, value):\n        return value\nNewFamily = NewCollection()\n'
        self.assertEqual(self.inspect('import SASVisualUtils as V\nV.NewFamily.perform(1)\n', sources=sources), [])
        self.assertTrue(self.inspect('import SASVisualUtils as V\nV.NewFamily.perform()\n', sources=sources))
        sources["SASVisualUtils"] = sources["SASVisualUtils"].replace('def perform(self, value):', 'def replacement(self, value):')
        self.assertTrue(self.inspect('import SASVisualUtils as V\nV.NewFamily.perform(1)\n', sources=sources))

    def test_static_methods_do_not_consume_an_implicit_self_argument(self):
        sources = dict(self.sources)
        sources["SASVisualUtils"] += '\nclass ExtraResources:\n    @staticmethod\n    def format(value):\n        return value\nExtra = ExtraResources()\n'
        self.assertEqual(self.inspect('import SASVisualUtils as V\nV.Extra.format(1)\n', sources=sources), [])
        self.assertTrue(self.inspect('import SASVisualUtils as V\nV.Extra.format()\n', sources=sources))

    def test_literals_and_shadowed_imports_are_not_module_accesses(self):
        source = 'import SASVisualUtils as V\n# V.NotACollection\nexample = "V.NotACollection"\ndef use(V):\n    V.anything()\n'
        self.assertEqual(self.inspect(source), [])

    def test_incremental_dependency_selection(self):
        catalog = api.Catalog({"SASVisualUtils": "", "SASTextScale": "", "A": "import SASVisualUtils", "B": "from A import *", "Unrelated": ""})
        self.assertEqual(api.affected_modules(catalog, ['Assets/Python/A.py']), {"A", "B"})
        self.assertEqual(api.affected_modules(catalog, ['README.md']), set())
        self.assertEqual(api.affected_modules(catalog, ['Assets/Python/SASVisualUtils.py']), set(catalog.trees))
        self.assertEqual(api.affected_modules(catalog, ['Assets/XML/Art/CIV4ArtDefines_Interface.xml']), set(catalog.trees))


if __name__ == "__main__":
    unittest.main()
