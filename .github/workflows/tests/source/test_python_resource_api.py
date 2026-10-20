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

    # <!-- custom: Stable resource identity and changing gameplay values are different contracts: reject raw visual lookups but keep current gold/production/state reads legal.
    # Test source-derived new families too, rather than only today's spellings. (GPT-6.1-Sol) -->
    def test_raw_cached_resource_families(self):
        calls = [
            "CyGame().getSymbolID(FontSymbols.HAPPY_CHAR)",
            "gc.getGame().getSymbolID(FontSymbols.HAPPY_CHAR)",
            "gc.getYieldInfo(0).getChar()",
            "gc.getYieldInfo(0).getColorType()",
            "gc.getCommerceInfo(0).getChar()",
            "gc.getReligionInfo(0).getChar()",
            "gc.getReligionInfo(0).getHolyCityChar()",
            "gc.getCorporationInfo(0).getChar()",
            "gc.getCorporationInfo(0).getHeadquarterChar()",
            "gc.getBonusInfo(0).getChar()",
            "Art.getInterfaceArtInfo('SOME_ART').getPath()",
            "Art.getMovieArtInfo('SOME_MOVIE').getPath()",
            "Art.getCivilizationArtInfo('SOME_CIV').getButton()",
            "getInfoTypeOrFail('COLOR_RED')",
            "gc.getDefineINT('SAS_UI_FONT_BODY')",
        ]
        for call in calls:
            with self.subTest(call=call):
                source = "gc = CyGlobalContext()\nArt = CyArtFileMgr()\n" + call + "\n"
                self.assertTrue(any("bypasses shared cache" in message for _line, message in self.inspect(source)))
        errors = self.inspect("gc = CyGlobalContext()\ngc.getDefineINT('SAS_UI_FONT_BODY')\n")
        self.assertTrue(any("SASTextScale.getSASUIFontBody" in message for _line, message in errors))

    def test_resource_origins_follow_aliases_and_info_variables(self):
        source = "from CvPythonExtensions import CyGlobalContext as Context\nworld = Context()\ngetter = world.getCommerceInfo\ninfo = getter(0)\ninfo.getChar()\n"
        self.assertTrue(any("SASVisualUtils.Commerce.ordinal" in message for _line, message in self.inspect(source)))
        source = "from SASUtils import getInfoTypeOrFail as lookup\nlookup('COLOR_RED')\n"
        self.assertTrue(any("SASVisualUtils.Color.get" in message for _line, message in self.inspect(source)))

    def test_source_derived_legacy_wrapper_alias(self):
        wrappers = {"ColorHelper": "from SASUtils import getInfoTypeOrFail\ndef resolve(key):\n    result = getInfoTypeOrFail(key)\n    return result\n"}
        source = "import ColorHelper as Colors\nColors.resolve('COLOR_RED')\n"
        self.assertTrue(any("SASVisualUtils.Color.get" in message for _line, message in self.inspect(source, wrappers)))
        self.assertEqual(self.inspect("import ColorHelper as Colors\nColors.resolve('UNIT_SETTLER')\n", wrappers), [])
        wrappers["UnitHelper"] = "from SASUtils import getInfoTypeOrFail\ndef resolve(label):\n    return getInfoTypeOrFail('UNIT_SETTLER')\n"
        self.assertEqual(self.inspect("import UnitHelper\nUnitHelper.resolve('COLOR_RED')\n", wrappers), [])

    def test_module_qualified_engine_factory(self):
        source = "import CvPythonExtensions as Engine\nEngine.CyGame().getSymbolID(0)\n"
        self.assertTrue(any("SASVisualUtils.Font.ordinal" in message for _line, message in self.inspect(source)))

    def test_shared_cache_access_and_unrelated_dynamic_reads_are_valid(self):
        # <!-- custom: Fixture imports use SASVisualUtils for shared visual access and SASTextScale for the cached font-size accessor; they are parsed without importing the game. (GPT-6.1-Sol) -->
        source = """import SASVisualUtils as V
import SASTextScale as Text
gc = CyGlobalContext()
amount = gc.getPlayer(0).getGold()
gc.getYieldInfo(0).getDescription()
gc.getYieldInfo(0).getButton()
gc.getDefineINT('UNRELATED_GAMEPLAY_SETTING')
getInfoTypeOrFail('UNIT_SETTLER')
V.Commerce.char(0)
V.Yield.color(0)
V.InterfaceArt.getPath('SOME_ART')
Text.getSASUIFontBody()
"""
        self.assertEqual(self.inspect(source), [])

    def test_resource_name_shadowing_is_not_an_engine_lookup(self):
        for source in (
            "gc = CyGlobalContext()\ndef render(gc):\n    return gc.getCommerceInfo(0).getChar()\n",
            "gc = CyGlobalContext()\nread = lambda gc: gc.getCommerceInfo(0).getChar()\n",
            "from unrelated import CyGlobalContext\ngc = CyGlobalContext()\ngc.getCommerceInfo(0).getChar()\n",
            "gc = otherObject\ngc.getCommerceInfo(0).getChar()\n",
        ):
            with self.subTest(source=source):
                self.assertEqual(self.inspect(source), [])

    def test_direct_resource_exception_is_local_and_requires_reason(self):
        call = "gc.getCommerceInfo(0).getChar()\n"
        prefix = "gc = CyGlobalContext()\n"
        note = "# <!-- custom: resource-cache-direct: Compare the engine value with the shared cache in this diagnostic. (GPT-6.1-Sol) -->\n"
        self.assertEqual(self.inspect(prefix + note + call), [])
        self.assertTrue(self.inspect(prefix + note + call + call))
        self.assertTrue(self.inspect(prefix + "# <!-- custom: resource-cache-direct: -->\n" + call))
        self.assertTrue(self.inspect(prefix + "note = 'resource-cache-direct: fake exception'\n" + call))

    def test_shared_art_literals_and_provider_implementation(self):
        self.assertTrue(any("duplicated shared art path" in message for _line, message in self.inspect("path = 'Art/Interface/Buttons/Actions/FoundCity.dds'\n")))
        self.assertEqual(self.inspect("path = 'Art/Interface/Buttons/Unshared.dds'\n"), [])
        catalog = api.Catalog(self.sources, self.xml, self.enums)
        for module in api.PROVIDERS:
            self.assertEqual(api.inspect_consumer(module, catalog), [])

    def test_new_cached_family_is_derived_without_checker_edits(self):
        definitions = dict(self.sources)
        definitions['SASVisualUtils'] += """
class _NewResources:
    def __init__(self):
        self._cache = {}
    def symbol(self, index):
        if index not in self._cache:
            self._cache[index] = gc.getNewInfo(index).getSymbol()
        return self._cache[index]
NewResources = _NewResources()
"""
        source = "gc = CyGlobalContext()\ngc.getNewInfo(0).getSymbol()\n"
        errors = self.inspect(source, sources=definitions)
        self.assertTrue(any("SASVisualUtils.NewResources.symbol" in message for _line, message in errors), errors)

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
