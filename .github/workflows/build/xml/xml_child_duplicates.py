#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from pathlib import Path
import argparse
import sys
import io
import xml.sax

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root


XML_SCAN_RELATIVE_PATHS = (
	Path("Assets/XML"),
)

IGNORED_XML_RELATIVE_DIRS = {
	Path("Assets/XML/Text"),
}

# Duplicate child/list entries are more context-dependent than duplicate parent XML objects.
# The schemas below are split into:
# - priority: strong enough to fail CI when duplicated.
# - report-only: suspicious, but not yet strong enough to fail CI without manual review.
# - known allowed/noisy: hidden by default and shown only with --show-ignored.
PRIORITY_CHILD_KEY_SCHEMAS = {
	# Generic flavor / weighting lists. A duplicate flavor type usually double-counts or masks intent.
	("Flavors", "Flavor", ("FlavorType",)),
	("TechFlavors", "TechFlavor", ("FlavorType",)),

	# Leader personality arrays. A duplicate key makes the effective leader personality ambiguous.
	("ContactRands", "ContactRand", ("ContactType",)),
	("ContactDelays", "ContactDelay", ("ContactType",)),
	("NoWarAttitudeProbs", "NoWarAttitudeProb", ("AttitudeType",)),
	("MemoryDecays", "MemoryDecay", ("MemoryType",)),
	("MemoryAttitudePercents", "MemoryAttitudePercent", ("MemoryType",)),
	("UnitAIWeightModifiers", "UnitAIWeightModifier", ("UnitAIType",)),
	("ImprovementWeightModifiers", "ImprovementWeightModifier", ("ImprovementType",)),
	("Traits", "Trait", ("TraitType",)),

	# Leader diplomacy music by era. The same era should not be configured twice in one music list.
	("DiplomacyMusicPeace", "DiploMusicPeaceEra", ("EraType",)),
	("DiplomacyMusicWar", "DiploMusicWarEra", ("EraType",)),
	("DiplomacyIntroMusicPeace", "DiploMusicPeaceEra", ("EraType",)),
	("DiplomacyIntroMusicWar", "DiploMusicWarEra", ("EraType",)),

	# Civilization replacement / free-start lists.
	("Units", "Unit", ("UnitClassType",)),
	("Buildings", "Building", ("BuildingClassType",)),
	("FreeTechs", "FreeTech", ("TechType",)),
	("FreeBuildingClasses", "FreeBuildingClass", ("BuildingClassType",)),

	# Unit AI / upgrade / promotion lists.
	("UnitAIs", "UnitAI", ("UnitAIType",)),
	("NotUnitAIs", "UnitAI", ("UnitAIType",)),
	("UnitClassUpgrades", "UnitClassUpgrade", ("UnitClassUpgradeType",)),
	("FreePromotions", "FreePromotion", ("PromotionType",)),
	("FlankingStrikes", "FlankingStrike", ("FlankingStrikeUnitClass",)),

	# Promotion unit-combat lists.
	("UnitCombats", "UnitCombat", ("UnitCombatType",)),
	("UnitCombatMods", "UnitCombatMod", ("UnitCombatType",)),
	("FreePromotionUnitCombats", "FreePromotionUnitCombat", ("UnitCombatType",)),

	# Unit terrain / feature modifiers.
	("TerrainAttacks", "TerrainAttack", ("TerrainType",)),
	("TerrainDefenses", "TerrainDefense", ("TerrainType",)),
	("TerrainNatives", "TerrainNative", ("TerrainType",)),
	("FeatureAttacks", "FeatureAttack", ("FeatureType",)),
	("FeatureDefenses", "FeatureDefense", ("FeatureType",)),
	("FeatureDoubleMoves", "FeatureDoubleMove", ("FeatureType",)),
	("FeatureImpassables", "FeatureImpassable", ("FeatureType",)),

	# Building / civic / event effects.
	("BonusHappinessChanges", "BonusHappinessChange", ("BonusType",)),
	("BonusHealthChanges", "BonusHealthChange", ("BonusType",)),
	("BonusYieldModifiers", "BonusYieldModifier", ("BonusType",)),
	("BuildingHappinessChanges", "BuildingHappinessChange", ("BuildingType",)),
	("SpecialistCounts", "SpecialistCount", ("SpecialistType",)),
	("SpecialistValids", "SpecialistValid", ("SpecialistType",)),
	("ImprovementYieldChanges", "ImprovementYieldChange", ("ImprovementType",)),
	("DomainExtraMoves", "DomainExtraMove", ("DomainType",)),
	("UnitCombatFreeExperiences", "UnitCombatFreeExperience", ("UnitCombatType",)),
	("PlotExtraYields", "PlotExtraYield", ("YieldType",)),

	# Composite effect keys. Repeating only the first key can be valid when the second effect key differs.
	# Example: a Jewish Monastery can receive both culture and gold from the same event.
	("BuildingExtraCommerces", "BuildingExtraCommerce", ("BuildingClass", "CommerceType")),
	("UnitClassPromotions", "UnitClassPromotion", ("UnitClass", "UnitPromotion")),

	# Terrain / bonus / improvement validity.
	("BonusTypeStructs", "BonusTypeStruct", ("BonusType",)),
	("PrereqOrBonuses", "PrereqOrBonus", ("BonusType",)),
	("TerrainBooleans", "TerrainBoolean", ("TerrainType",)),
	("FeatureBooleans", "FeatureBoolean", ("FeatureType",)),
	("FeatureTerrainBooleans", "FeatureTerrainBoolean", ("TerrainType",)),
	("TerrainMakesValids", "TerrainMakesValid", ("TerrainType",)),
	("FeatureMakesValids", "FeatureMakesValid", ("FeatureType",)),
	("FeatureStructs", "FeatureStruct", ("FeatureType",)),

	# Misc structured lists.
	("Builds", "Build", ("BuildType",)),
	("CarrierUnitAIs", "CarrierUnitAI", ("UnitAIType",)),
	("CitySoundscapes", "CitySoundscape", ("CitySizeType",)),
	("SpeedThresholds", "SpeedThreshold", ("GameSpeedType",)),
	("StyleUnits", "StyleUnit", ("UnitType",)),

	# True-start placement helpers.
	("EncouragedCivs", "EncouragedCiv", ("CivilizationType",)),
	("DiscouragedCivs", "DiscouragedCiv", ("CivilizationType",)),
}

PRIORITY_DIRECT_TEXT_LISTS = {
	# Duplicate city names inside one civilization are nearly always accidental.
	("Cities", "City"),
}

REPORT_ONLY_CHILD_KEY_SCHEMAS = {
	# Event trigger text can reuse the same Text key across different eras,
	# but an exact Text + Era duplicate inside one trigger is worth reviewing.
	("TriggerTexts", "TriggerText", ("Text", "Era")),
	# Diplomacy text can intentionally repeat attitude buckets with different possible lines.
	("Attitudes", "Attitude", ("AttitudeType",)),

	# Unit art/audio data can intentionally reuse movement/footstep sound buckets.
	("FootstepSounds", "FootstepSound", ("FootstepAudioType",)),
}

REPORT_ONLY_DIRECT_TEXT_LISTS = {
	# Event-trigger prereq lists are suspicious if duplicated, but event semantics can be broad.
	("AndPreReqs", "PrereqTech"),
	("OrPreReqs", "PrereqTech"),
	("PrereqTechs", "PrereqTech"),
	("ObsoleteTechs", "ObsoleteTech"),
	("TechTypes", "PrereqTech"),
	("PrereqBonuses", "BonusType"),

	# Requirement lists are worth surfacing first before deciding whether all are fail-safe.
	("RoutesRequired", "RouteType"),
	("BuildingsRequired", "BuildingClass"),
	("ImprovementsRequired", "ImprovementType"),
	("TerrainsRequired", "TerrainType"),
	("UnitsRequired", "UnitClass"),
	("BonusesRequired", "BonusType"),
	("ReligionsRequired", "ReligionType"),
	("FeaturesRequired", "FeatureType"),

	# Event result lists may be legitimate, but duplicate event references are worth reviewing.
	("Events", "Event"),
	("UniqueNames", "UniqueName"),
}

# Known allowed/noisy duplicates are hidden by default. Use --show-ignored to print them.
KNOWN_ALLOWED_DIRECT_TEXT_LISTS = {
	# Goody hut outcomes intentionally repeat values to weight probabilities by handicap.
	("Goodies", "GoodyType"),

	# World picker UI art intentionally reuses size values and shared climate/water art paths.
	("Sizes", "Size"),
	("Climates", "ClimatePath"),
	("WaterLevelDecals", "WaterLevelDecalPath"),
	("WaterLevelGloss", "WaterLevelGlossPath"),
}

IGNORED_DIRECT_TEXT_VALUES = {
	"",
	"NONE",
	"NO_BONUS",
	"NO_TECH",
	"NO_UNIT",
	"NO_BUILDING",
	"NO_PROMOTION",
	"NO_FEATURE",
	"NO_TERRAIN",
	"NO_IMPROVEMENT",
	"NO_SPECIALIST",
}


class XmlNode:
	def __init__(self, tag: str, line: int, parent: "XmlNode | None" = None) -> None:
		self.tag = tag
		self.line = line
		self.parent = parent
		self.children: list[XmlNode] = []
		self.text_parts: list[str] = []

	@property
	def text(self) -> str:
		return "".join(self.text_parts).strip()


class LineNumberXmlHandler(xml.sax.handler.ContentHandler):
	def __init__(self) -> None:
		super().__init__()
		self.locator = None
		self.root: XmlNode | None = None
		self.stack: list[XmlNode] = []

	def setDocumentLocator(self, locator) -> None:
		self.locator = locator

	def startElement(self, name, attrs) -> None:
		line = self.locator.getLineNumber() if self.locator is not None else 0
		parent = self.stack[-1] if self.stack else None
		node = XmlNode(name, line, parent)
		if parent is not None:
			parent.children.append(node)
		else:
			self.root = node
		self.stack.append(node)

	def characters(self, content) -> None:
		if self.stack:
			self.stack[-1].text_parts.append(content)

	def endElement(self, name) -> None:
		self.stack.pop()


def read_xml_text(path: Path) -> str:
	data = path.read_bytes()

	for encoding in ("utf-8-sig", "cp1252", "latin1"):
		try:
			return data.decode(encoding)
		except UnicodeDecodeError:
			continue

	return data.decode("latin1", errors="replace")


def parse_xml_with_line_numbers(path: Path) -> XmlNode:
	handler = LineNumberXmlHandler()
	parser = xml.sax.make_parser()
	parser.setFeature(xml.sax.handler.feature_namespaces, False)
	parser.setContentHandler(handler)

	parser.parse(io.StringIO(read_xml_text(path)))

	if handler.root is None:
		raise ValueError("XML file has no root element")

	return handler.root


def local_name(tag: str) -> str:
	return tag.rsplit("}", 1)[-1]


def direct_child_by_local_name(node: XmlNode, child_name: str) -> XmlNode | None:
	for child in node.children:
		if local_name(child.tag) == child_name:
			return child
	return None


def direct_child_text_by_local_name(node: XmlNode, child_name: str) -> str | None:
	child = direct_child_by_local_name(node, child_name)
	if child is None:
		return None
	return child.text


def display_parent_key(node: XmlNode) -> str:
	for key_field in ("Type", "ScriptID", "SoundID", "Description", "Text"):
		value = direct_child_text_by_local_name(node, key_field)
		if value:
			return value
	return f"<{local_name(node.tag)}>"


def walk_nodes(root: XmlNode):
	yield root
	for child in root.children:
		for node in walk_nodes(child):
			yield node


def iter_child_containers(root: XmlNode, container_tag: str):
	for node in walk_nodes(root):
		if local_name(node.tag) == container_tag:
			yield node


def find_nearest_named_parent(container: XmlNode) -> str:
	node = container.parent
	while node is not None:
		key = display_parent_key(node)
		if key != f"<{local_name(node.tag)}>":
			return f"<{local_name(node.tag)}> {key}"
		node = node.parent

	return "<unknown parent>"


def format_lines(lines: list[int]) -> str:
	line_text = ", ".join(str(line) for line in sorted(lines))
	return f"lines {line_text}"


def format_key_value(key_fields: tuple[str, ...], key_values: tuple[str, ...]) -> str:
	return " + ".join(f"<{key_field}> {key_value}" for key_field, key_value in zip(key_fields, key_values))


def collect_keyed_duplicates(root: XmlNode, relative_path: Path, schemas: set[tuple[str, str, tuple[str, ...]]], kind: str) -> list[str]:
	findings: list[str] = []

	for container_tag, entry_tag, key_fields in sorted(schemas):
		for container in iter_child_containers(root, container_tag):
			values: dict[tuple[str, ...], list[int]] = {}
			for entry in container.children:
				if local_name(entry.tag) != entry_tag:
					continue

				key_values = []
				key_lines = []
				missing_key = False
				for key_field in key_fields:
					key_node = direct_child_by_local_name(entry, key_field)
					if key_node is None or not key_node.text:
						missing_key = True
						break
					key_values.append(key_node.text)
					key_lines.append(key_node.line)

				if missing_key:
					continue

				values.setdefault(tuple(key_values), []).append(min(key_lines))

			for key_values, lines in sorted(values.items()):
				if len(lines) <= 1:
					continue

				parent_text = find_nearest_named_parent(container)
				key_text = format_key_value(key_fields, key_values)
				findings.append(
					f"{relative_path}: {format_lines(lines)}: {kind} duplicate child key under {parent_text}: "
					f"<{container_tag}>/<{entry_tag}> duplicate {key_text} ({len(lines)} entries)"
				)

	return findings


def collect_direct_text_duplicates(root: XmlNode, relative_path: Path, schemas: set[tuple[str, str]], kind: str) -> list[str]:
	findings: list[str] = []

	for container_tag, entry_tag in sorted(schemas):
		for container in iter_child_containers(root, container_tag):
			values: dict[str, list[int]] = {}
			for entry in container.children:
				if local_name(entry.tag) != entry_tag:
					continue

				value = entry.text
				if value in IGNORED_DIRECT_TEXT_VALUES:
					continue

				values.setdefault(value, []).append(entry.line)

			for value, lines in sorted(values.items()):
				if len(lines) <= 1:
					continue

				parent_text = find_nearest_named_parent(container)
				findings.append(
					f"{relative_path}: {format_lines(lines)}: {kind} duplicate direct child text under {parent_text}: "
					f"<{container_tag}>/<{entry_tag}> duplicate value {value} ({len(lines)} entries)"
				)

	return findings


def check_xml_child_duplicate_report(repo_root: Path, show_ignored: bool) -> tuple[list[str], list[str], list[str], list[str]]:
	errors: list[str] = []
	priority_findings: list[str] = []
	report_only_findings: list[str] = []
	ignored_findings: list[str] = []

	for relative_root in XML_SCAN_RELATIVE_PATHS:
		root_dir = repo_root / relative_root
		if not root_dir.exists():
			errors.append(f"{relative_root}: missing XML scan folder")
			continue

		for path in sorted(root_dir.rglob("*.xml")):
			relative_path = path.relative_to(repo_root)
			if any(relative_path == ignored_dir or ignored_dir in relative_path.parents for ignored_dir in IGNORED_XML_RELATIVE_DIRS):
				continue

			try:
				root = parse_xml_with_line_numbers(path)
			except xml.sax.SAXParseException as exc:
				errors.append(f"{relative_path}: XML parse error at line {exc.getLineNumber()}, column {exc.getColumnNumber()}: {exc.getMessage()}")
				continue
			except Exception as exc:
				errors.append(f"{relative_path}: XML parse error: {exc}")
				continue

			priority_findings.extend(collect_keyed_duplicates(root, relative_path, PRIORITY_CHILD_KEY_SCHEMAS, "priority"))
			priority_findings.extend(collect_direct_text_duplicates(root, relative_path, PRIORITY_DIRECT_TEXT_LISTS, "priority"))
			report_only_findings.extend(collect_keyed_duplicates(root, relative_path, REPORT_ONLY_CHILD_KEY_SCHEMAS, "report-only"))
			report_only_findings.extend(collect_direct_text_duplicates(root, relative_path, REPORT_ONLY_DIRECT_TEXT_LISTS, "report-only"))

			if show_ignored:
				ignored_findings.extend(collect_direct_text_duplicates(root, relative_path, KNOWN_ALLOWED_DIRECT_TEXT_LISTS, "ignored-known-allowed"))

	return errors, priority_findings, report_only_findings, ignored_findings


def print_section(title: str, lines: list[str]) -> None:
	print(title)
	if not lines:
		print("  - none")
		return

	for line in lines:
		print(f"  - {line}")


def main() -> int:
	parser = argparse.ArgumentParser(description="Report or fail on configured duplicate child/list XML entries.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	parser.add_argument("--show-ignored", action="store_true", help="also print known allowed/noisy duplicate child lists, such as weighted goody huts and reused world-picker UI art paths")
	args = parser.parse_args()

	errors, priority_findings, report_only_findings, ignored_findings = check_xml_child_duplicate_report(args.repo_root, args.show_ignored)

	if errors:
		print_section("FAIL XML child duplicate setup/parse errors", errors)
		return 1

	if not priority_findings and not report_only_findings and not ignored_findings:
		print("PASS XML child duplicates")
		return 0

	if priority_findings:
		print("FAIL XML priority child duplicates")
		print("Priority duplicate child/list entries are treated as build errors.")
	else:
		print("REPORT XML child duplicate entries")
		print("Only report-only duplicate findings were found, so this exits 0.")

	print_section("Priority findings", priority_findings)
	print_section("Report-only findings", report_only_findings)

	if args.show_ignored:
		print_section("Ignored known-allowed findings", ignored_findings)

	return 1 if priority_findings else 0


if __name__ == "__main__":
	sys.exit(main())
