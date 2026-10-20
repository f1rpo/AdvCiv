# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Keep Python visual-resource ownership centralized in SASVisualUtils so direct GameFont/XML-info/art/color lookups and duplicate per-screen caches do not gradually reappear after this refactor. (ChatGPT-5.6-Sol) -->

# <!-- custom: Use only Python's standard tokenizer/regex/unittest helpers so the guard can inspect Python-2.4 gameplay source without importing Civ4 modules under CI's modern Python. (ChatGPT-5.6-Sol) -->
import io
from pathlib import Path
import re
import tokenize
import unittest


REPO_ROOT = Path(__file__).resolve().parents[4]
PYTHON_ROOT = REPO_ROOT / "Assets" / "Python"
CENTRAL_RESOURCE = PYTHON_ROOT / "SASVisualUtils.py"
CV_ENUMS = REPO_ROOT / "CvGameCoreDLL" / "CvEnums.h"
TRAIT_XML = REPO_ROOT / "Assets" / "XML" / "Civilizations" / "CIV4TraitInfos.xml"
UNIT_XML = REPO_ROOT / "Assets" / "XML" / "Units" / "CIV4UnitInfos.xml"
SPECIALIST_XML = REPO_ROOT / "Assets" / "XML" / "GameInfo" / "CIV4SpecialistInfos.xml"
RETIRED_MODULES = (
    PYTHON_ROOT / "SASUIResources.py",
    PYTHON_ROOT / "SASFontUtils.py",
    PYTHON_ROOT / "BUG" / "FontUtil.py",
    PYTHON_ROOT / "BUG" / "TraitUtil.py",
    PYTHON_ROOT / "BUG" / "GGUtil.py",
)


def _masked_source(path, mask_strings):
    source = path.read_text(encoding="latin-1")
    tokens = []
    for token in tokenize.generate_tokens(io.StringIO(source).readline):
        token_type, token_text, start, end, line = token
        if token_type == tokenize.COMMENT or (mask_strings and token_type == tokenize.STRING):
            token_text = ""
        tokens.append((token_type, token_text, start, end, line))
    return tokenize.untokenize(tokens)


def _gameplay_python_files():
    for path in sorted(PYTHON_ROOT.rglob("*.py")):
        if path != CENTRAL_RESOURCE:
            yield path


def _quoted_names(block):
    return set(re.findall(r'["\']([A-Z][A-Z0-9_]+)["\']', block))


def _mapping_keys(block):
    return set(re.findall(r'^\s*["\']([A-Z][A-Z0-9_]+)["\']\s*:', block, re.M))


def _mapping_block(source, name):
    match = re.search(r"^%s\s*=\s*\{(.*?)^\}" % re.escape(name), source, re.M | re.S)
    if not match:
        raise AssertionError("Could not find mapping %s" % name)
    return match.group(1)


def _tuple_block(source, name):
    match = re.search(r"^%s\s*=\s*\((.*?)^\)" % re.escape(name), source, re.M | re.S)
    if not match:
        raise AssertionError("Could not find tuple %s" % name)
    return match.group(1)


def _xml_types(path, prefix):
    source = path.read_text(encoding="latin-1")
    return set(re.findall(r"<Type>(%s[A-Z0-9_]+)</Type>" % re.escape(prefix), source))


class PythonVisualResourceTests(unittest.TestCase):
    def test_raw_visual_lookup_stays_in_sas_visual_utils(self):
        # <!-- custom: The central module may perform raw engine/XML/art lookup; ordinary UI modules consume its shared cached collections instead. (ChatGPT-5.6-Sol) -->
        patterns = (
            ("direct GameFont symbol lookup", re.compile(r"\bgetSymbolID\s*\(")),
            ("direct holy-city/HQ char lookup", re.compile(r"\.\s*get(?:HolyCityChar|HeadquarterChar)\s*\(")),
            ("direct interface/movie/civilization art-manager lookup", re.compile(r"(?:ArtFileMgr|CyArtFileMgr\(\))\s*\.\s*get(?:Interface|Movie|Civilization)ArtInfo\s*\(")),
            ("direct COLOR_* XML lookup", re.compile(r"\b(?:getInfoTypeOrFail|(?:gc\.)?getInfoTypeForString)\s*\(\s*[uUrR]*[\"\']COLOR_")),
            ("direct literal ColorUtil lookup", re.compile(r"\bColorUtil\.keyToType\s*\(\s*[uUrR]*[\"\']COLOR_")),
            ("direct yield color lookup", re.compile(r"\bgetYieldInfo\s*\([^)]*\)\s*\.\s*getColorType\s*\(")),
        )
        failures = []
        for path in _gameplay_python_files():
            source = _masked_source(path, False)
            # <!-- custom: DynamicFont.getChar(symbol) is the intentional runtime-variable BUG-symbol API; any other raw .getChar() bypasses the central indexed collections. (ChatGPT-5.6-Sol) -->
            raw_char_source = source.replace("SASVisuals.DynamicFont.getChar(", "")
            if re.search(r"\.\s*getChar\s*\(", raw_char_source):
                failures.append("%s: direct info/font .getChar() lookup" % path.relative_to(REPO_ROOT))
            for label, pattern in patterns:
                if pattern.search(source):
                    failures.append("%s: %s" % (path.relative_to(REPO_ROOT), label))
        self.assertEqual(failures, [], "\n".join(failures))


    def test_display_code_uses_cached_chars_not_ordinals(self):
        # <!-- custom: UI rendering should consume the cached Unicode glyph itself.
        # Keep ordinal() for the small set of integer contracts (translator/XML %c arguments, offset/index arithmetic, diagnostics), not for immediately formatting a glyph back through Python %c. (ChatGPT-5.6-Sol) -->
        failures = []
        display_pattern = re.compile(r"%c[^\r\n]*SASVisuals\.|SASVisuals\.[^\r\n]*%c")
        for path in _gameplay_python_files():
            source = _masked_source(path, False)
            if display_pattern.search(source):
                failures.append("%s: SASVisuals display glyph formatted through %%c instead of cached char" % path.relative_to(REPO_ROOT))
        self.assertEqual(failures, [], "\n".join(failures))

        # <!-- custom: Keep all remaining ordinal consumers intentionally narrow and reviewable; new display code should use char()/named *_CHAR attributes instead. (ChatGPT-5.6-Sol) -->
        allowed = {
            "Contrib/GameFontDisplay.py": re.compile(r"\biMax\s*=.*SASVisuals\.Font\.ordinal\(FontSymbols\.HAPPY_CHAR\)"),
            "Screens/CvMainInterface.py": re.compile(r"(?:localText\.getText|\bcAtt\s*=).*SASVisuals\.(?:Font|Commerce)\.ordinal\("),
            "EntryPoints/CvRandomEventInterface.py": re.compile(r"localText\.getText.*SASVisuals\.Religion\.ordinal\("),
        }
        failures = []
        ordinal_pattern = re.compile(r"SASVisuals\.[A-Za-z]+\.ordinal\(")
        for path in _gameplay_python_files():
            rel = str(path.relative_to(PYTHON_ROOT)).replace("\\", "/")
            source = _masked_source(path, True)
            for line_number, line in enumerate(source.splitlines(), 1):
                if not ordinal_pattern.search(line):
                    continue
                allow_pattern = allowed.get(rel)
                if allow_pattern is None or not allow_pattern.search(line):
                    failures.append("%s:%d: ordinal() is not an approved integer-only visual contract" % (path.relative_to(REPO_ROOT), line_number))
        self.assertEqual(failures, [], "\n".join(failures))


    def test_old_per_screen_visual_char_caches_stay_retired(self):
        # <!-- custom: These were deterministic arrays rebuilt by individual screens despite the same tuples/ordinals already being owned by SASVisualUtils.
        # Keep the concrete legacy cache names retired. (ChatGPT-5.6-Sol) -->
        pattern = re.compile(r"\bself\.(?:COMMERCE_CHARS|YIELD_CHARS)\b")
        failures = []
        for path in _gameplay_python_files():
            if pattern.search(_masked_source(path, False)):
                failures.append(str(path.relative_to(REPO_ROOT)))
        self.assertEqual(failures, [], "\n".join(failures))

    def test_literal_interface_art_stays_central(self):
        # <!-- custom: Literal Art/Interface paths are visual resource identities too.
        # Keep them in StaticArt/trait maps rather than scattering path strings or aliases through screens. (ChatGPT-5.6-Sol) -->
        pattern = re.compile(r"[uUrR]*[\"\']Art/Interface/")
        failures = []
        for path in _gameplay_python_files():
            if pattern.search(_masked_source(path, False)):
                failures.append(str(path.relative_to(REPO_ROOT)))
        self.assertEqual(failures, [], "\n".join(failures))

    def test_sas_visual_utils_is_imported_through_sasvisuals_alias(self):
        # <!-- custom: Generic collection names such as Yield/Color/Trait remain safely namespaced at call sites, avoiding collisions with inherited modules/globals. (ChatGPT-5.6-Sol) -->
        bad_pattern = re.compile(r"(?:^|\n)\s*(?:from\s+SASVisualUtils\s+import|import\s+SASVisualUtils(?!\s+as\s+SASVisuals\b))")
        failures = []
        for path in _gameplay_python_files():
            if bad_pattern.search(_masked_source(path, True)):
                failures.append(str(path.relative_to(REPO_ROOT)))
        self.assertEqual(failures, [], "\n".join(failures))

    def test_retired_visual_helper_modules_stay_retired(self):
        for path in RETIRED_MODULES:
            self.assertFalse(path.exists(), str(path.relative_to(REPO_ROOT)))
        pattern = re.compile(r"(?:^|\n)\s*(?:from\s+(?:SASUIResources|SASFontUtils|FontUtil|TraitUtil|GGUtil)\s+import|import\s+(?:SASUIResources|SASFontUtils|FontUtil|TraitUtil|GGUtil)\b)")
        failures = []
        for path in _gameplay_python_files():
            if pattern.search(_masked_source(path, True)):
                failures.append(str(path.relative_to(REPO_ROOT)))
        self.assertEqual(failures, [], "\n".join(failures))

    def test_builtin_font_symbol_inventory_matches_dll_enum(self):
        source = CENTRAL_RESOURCE.read_text(encoding="latin-1")
        central = _quoted_names(_tuple_block(source, "BUILTIN_FONT_SYMBOL_NAMES"))
        enums = CV_ENUMS.read_text(encoding="latin-1")
        block = re.search(r"enum\s+FontSymbols\s*\{(.*?)\};", enums, re.S).group(1)
        block = re.sub(r"/\*.*?\*/|//[^\r\n]*", "", block, flags=re.S)
        dll = set(re.findall(r"^\s*([A-Z][A-Z0-9_]*_CHAR)\b", block, re.M))
        self.assertEqual(central, dll)

    def test_trait_visual_map_matches_live_trait_xml(self):
        source = CENTRAL_RESOURCE.read_text(encoding="latin-1")
        central = _mapping_keys(_mapping_block(source, "_TRAIT_RESOURCE_SPECS"))
        self.assertEqual(central, _xml_types(TRAIT_XML, "TRAIT_"))

    def test_great_person_inventory_matches_live_unit_xml(self):
        source = CENTRAL_RESOURCE.read_text(encoding="latin-1")
        units = _xml_types(UNIT_XML, "UNIT_GREAT_")
        listed = _quoted_names(_tuple_block(source, "GREAT_PERSON_UNIT_TYPES"))
        icon_map = _mapping_keys(_mapping_block(source, "_GP_UNIT_ICON_SPECS"))
        color_map = _mapping_keys(_mapping_block(source, "_GP_COLOR_SPECS"))
        self.assertEqual(listed, units)
        self.assertEqual(icon_map, units)
        self.assertEqual(color_map, units)

    def test_specialist_visual_map_matches_live_specialist_xml(self):
        source = CENTRAL_RESOURCE.read_text(encoding="latin-1")
        central = _mapping_keys(_mapping_block(source, "_SPECIALIST_ICON_SPECS"))
        self.assertEqual(central, _xml_types(SPECIALIST_XML, "SPECIALIST_"))


if __name__ == "__main__":
    unittest.main()
