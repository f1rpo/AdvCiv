#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Check world-wonder culture progression and building Great Person point consistency.
# Early/mid wonders share column-based flat culture; late culture-focused wonders use either flat culture or a percentage modifier, while other late wonders preserve their strategic identity without automatic culture.
# World-wonder GPP establish a nondecreasing ceiling that national and ordinary buildings must not exceed. (GPT-5.6-Sol) -->

from pathlib import Path
import argparse
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))

from xml_defines import get_default_repo_root, local_name, child_text_by_local_name


TECH_INFOS_REL_PATH = Path("Assets/XML/Technologies/CIV4TechInfos.xml")
BUILDING_CLASS_INFOS_REL_PATH = Path("Assets/XML/Buildings/CIV4BuildingClassInfos.xml")
BUILDING_INFOS_REL_PATH = Path("Assets/XML/Buildings/CIV4BuildingInfos.xml")
SPECIAL_BUILDING_INFOS_REL_PATH = Path("Assets/XML/Buildings/CIV4SpecialBuildingInfos.xml")
CULTURE_COMMERCE_INDEX = 2
EARLY_CULTURE_ERA = "ERA_ANCIENT"
EARLY_CULTURE_COLUMN_COUNT = 2
MID_CULTURE_ERA = "ERA_MEDIEVAL"
LATE_IDENTITY_CULTURE_ERA = "ERA_INDUSTRIAL"


def parse_xml(path: Path) -> ET.ElementTree:
	if not path.exists():
		raise RuntimeError(f"missing file: {path}")
	return ET.parse(path)


def read_tech_columns(repo_root: Path) -> dict[str, int]:
	path = repo_root / TECH_INFOS_REL_PATH
	columns: dict[str, int] = {}

	for node in parse_xml(path).getroot().iter():
		if local_name(node.tag) != "TechInfo":
			continue
		tech_type = child_text_by_local_name(node, "Type")
		grid_x_text = child_text_by_local_name(node, "iGridX")
		if tech_type is None:
			continue
		try:
			columns[tech_type] = int(grid_x_text or "-1")
		except ValueError as exc:
			raise RuntimeError(f"{TECH_INFOS_REL_PATH}: {tech_type} has invalid iGridX {grid_x_text!r}") from exc

	return columns


def read_era_start_columns(repo_root: Path) -> dict[str, int]:
	path = repo_root / TECH_INFOS_REL_PATH
	starts: dict[str, int] = {}

	for node in parse_xml(path).getroot().iter():
		if local_name(node.tag) != "TechInfo":
			continue
		era_type = child_text_by_local_name(node, "Era")
		grid_x_text = child_text_by_local_name(node, "iGridX")
		if era_type is None:
			continue
		try:
			grid_x = int(grid_x_text or "-1")
		except ValueError as exc:
			raise RuntimeError(f"{TECH_INFOS_REL_PATH}: tech in {era_type} has invalid iGridX {grid_x_text!r}") from exc
		starts[era_type] = min(starts.get(era_type, grid_x), grid_x)

	for required_era in (EARLY_CULTURE_ERA, MID_CULTURE_ERA, LATE_IDENTITY_CULTURE_ERA):
		if required_era not in starts:
			raise RuntimeError(f"{TECH_INFOS_REL_PATH}: no tech establishes the start of required culture-progression era {required_era}")

	return starts


def read_world_wonder_classes(repo_root: Path) -> set[str]:
	path = repo_root / BUILDING_CLASS_INFOS_REL_PATH
	world_wonder_classes: set[str] = set()

	for node in parse_xml(path).getroot().iter():
		if local_name(node.tag) != "BuildingClassInfo":
			continue
		class_type = child_text_by_local_name(node, "Type")
		max_global_text = child_text_by_local_name(node, "iMaxGlobalInstances")
		if class_type is None:
			continue
		try:
			max_global = int(max_global_text or "-1")
		except ValueError as exc:
			raise RuntimeError(f"{BUILDING_CLASS_INFOS_REL_PATH}: {class_type} has invalid iMaxGlobalInstances {max_global_text!r}") from exc
		if max_global == 1:
			world_wonder_classes.add(class_type)

	return world_wonder_classes


def read_building_class_categories(repo_root: Path) -> dict[str, str]:
	path = repo_root / BUILDING_CLASS_INFOS_REL_PATH
	categories: dict[str, str] = {}

	for node in parse_xml(path).getroot().iter():
		if local_name(node.tag) != "BuildingClassInfo":
			continue
		class_type = child_text_by_local_name(node, "Type")
		if class_type is None:
			continue
		max_global = read_int(class_type, node, "iMaxGlobalInstances", -1, BUILDING_CLASS_INFOS_REL_PATH)
		max_player = read_int(class_type, node, "iMaxPlayerInstances", -1, BUILDING_CLASS_INFOS_REL_PATH)
		if max_global == 1:
			categories[class_type] = "world wonder"
		elif max_player == 1:
			categories[class_type] = "national wonder"
		else:
			categories[class_type] = "ordinary building"

	return categories


def read_special_building_techs(repo_root: Path) -> dict[str, str]:
	path = repo_root / SPECIAL_BUILDING_INFOS_REL_PATH
	techs: dict[str, str] = {}

	for node in parse_xml(path).getroot().iter():
		if local_name(node.tag) != "SpecialBuildingInfo":
			continue
		special_type = child_text_by_local_name(node, "Type")
		prereq_tech = child_text_by_local_name(node, "TechPrereq")
		if special_type is not None and prereq_tech is not None:
			techs[special_type] = prereq_tech

	return techs


def read_int(building_type: str, node: ET.Element, field_name: str, default: int = 0, source_path: Path = BUILDING_INFOS_REL_PATH) -> int:
	value_text = child_text_by_local_name(node, field_name)
	if value_text is None or value_text == "":
		return default
	try:
		return int(value_text)
	except ValueError as exc:
		raise RuntimeError(f"{source_path}: {building_type} has invalid {field_name} {value_text!r}") from exc


def read_commerce_value(building_type: str, node: ET.Element, field_name: str, commerce_index: int) -> int:
	for child in list(node):
		if local_name(child.tag) != field_name:
			continue
		values = [(value.text or "").strip() for value in list(child) if local_name(value.tag) == "iCommerce"]
		if not values:
			return 0
		if len(values) <= commerce_index:
			return 0
		try:
			return int(values[commerce_index])
		except ValueError as exc:
			raise RuntimeError(f"{BUILDING_INFOS_REL_PATH}: {building_type} has invalid {field_name} value {values[commerce_index]!r}") from exc
	return 0


def read_constructible_world_wonders(repo_root: Path, tech_columns: dict[str, int], world_wonder_classes: set[str]) -> list[dict[str, int | str]]:
	path = repo_root / BUILDING_INFOS_REL_PATH
	wonders: list[dict[str, int | str]] = []

	for node in parse_xml(path).getroot().iter():
		if local_name(node.tag) != "BuildingInfo":
			continue
		building_type = child_text_by_local_name(node, "Type")
		building_class = child_text_by_local_name(node, "BuildingClass")
		if building_type is None or building_class not in world_wonder_classes:
			continue
		# <!-- custom: Great People found shrines and corporation headquarters with iCost=-1, so they are not normally constructible world wonders. (GPT-5.6-Sol) -->
		if read_int(building_type, node, "iCost", -1) < 0:
			continue

		prereq_tech = child_text_by_local_name(node, "PrereqTech")
		if prereq_tech not in tech_columns:
			raise RuntimeError(f"{BUILDING_INFOS_REL_PATH}: {building_type} has missing or unknown PrereqTech {prereq_tech!r}")

		wonders.append({
			"type": building_type,
			"tech": prereq_tech,
			"grid_x": tech_columns[prereq_tech],
			"advisor": child_text_by_local_name(node, "Advisor") or "NONE",
			"culture": read_commerce_value(building_type, node, "ObsoleteSafeCommerceChanges", CULTURE_COMMERCE_INDEX),
			"culture_modifier": read_commerce_value(building_type, node, "CommerceModifiers", CULTURE_COMMERCE_INDEX),
			"global_culture_modifier": read_commerce_value(building_type, node, "GlobalCommerceModifiers", CULTURE_COMMERCE_INDEX),
			"gpp": read_int(building_type, node, "iGreatPeopleRateChange"),
		})

	return wonders


def read_constructible_gp_buildings(repo_root: Path, tech_columns: dict[str, int], class_categories: dict[str, str], special_building_techs: dict[str, str]) -> list[dict[str, int | str]]:
	path = repo_root / BUILDING_INFOS_REL_PATH
	buildings: list[dict[str, int | str]] = []

	for node in parse_xml(path).getroot().iter():
		if local_name(node.tag) != "BuildingInfo":
			continue
		building_type = child_text_by_local_name(node, "Type")
		building_class = child_text_by_local_name(node, "BuildingClass")
		if building_type is None or building_class not in class_categories:
			continue
		gpp = read_int(building_type, node, "iGreatPeopleRateChange")
		if gpp <= 0 or read_int(building_type, node, "iCost", -1) < 0:
			continue

		prereq_tech = child_text_by_local_name(node, "PrereqTech")
		if prereq_tech not in tech_columns:
			special_type = child_text_by_local_name(node, "SpecialBuildingType")
			prereq_tech = special_building_techs.get(special_type or "")
		if prereq_tech not in tech_columns:
			raise RuntimeError(f"{BUILDING_INFOS_REL_PATH}: GP-producing {building_type} has missing or unknown direct/special-building prerequisite tech {prereq_tech!r}")

		buildings.append({
			"type": building_type,
			"tech": prereq_tech,
			"grid_x": tech_columns[prereq_tech],
			"category": class_categories[building_class],
			"gpp": gpp,
		})

	return buildings


def format_wonders(wonders: list[dict[str, int | str]]) -> str:
	return ", ".join(f"{wonder['type']}({wonder['tech']})" for wonder in sorted(wonders, key=lambda item: str(item["type"])))


def check_metric(by_column: dict[int, list[dict[str, int | str]]], key: str, label: str) -> list[str]:
	failures: list[str] = []
	column_values: dict[int, int] = {}

	for grid_x in sorted(by_column):
		by_value: dict[int, list[dict[str, int | str]]] = {}
		for wonder in by_column[grid_x]:
			by_value.setdefault(int(wonder[key]), []).append(wonder)
		if len(by_value) != 1:
			parts = [f"{label}={value}: {format_wonders(by_value[value])}" for value in sorted(by_value)]
			failures.append(f"iGridX={grid_x} has multiple world-wonder {label} values: " + "; ".join(parts))
			continue
		column_values[grid_x] = next(iter(by_value))

	previous_column = None
	previous_value = None
	for grid_x in sorted(column_values):
		value = column_values[grid_x]
		if previous_value is not None and value < previous_value:
			failures.append(f"world-wonder {label} decreases from iGridX={previous_column} ({label}={previous_value}) to iGridX={grid_x} ({label}={value})")
		previous_column = grid_x
		previous_value = value

	return failures


def check_non_world_gpp_ceiling(wonders: list[dict[str, int | str]], buildings: list[dict[str, int | str]]) -> list[str]:
	failures: list[str] = []
	wonder_gpp_by_column: dict[int, int] = {}
	for wonder in wonders:
		grid_x = int(wonder["grid_x"])
		wonder_gpp_by_column[grid_x] = max(wonder_gpp_by_column.get(grid_x, 0), int(wonder["gpp"]))

	for building in buildings:
		if building["category"] == "world wonder":
			continue
		available_rates = [rate for grid_x, rate in wonder_gpp_by_column.items() if grid_x <= int(building["grid_x"])]
		if not available_rates:
			failures.append(f"{building['category']} {building['type']}({building['tech']}, iGridX={building['grid_x']}) generates GPP before any constructible world wonder establishes a GPP rate")
			continue
		world_wonder_ceiling = max(available_rates)
		if int(building["gpp"]) > world_wonder_ceiling:
			failures.append(f"{building['category']} {building['type']}({building['tech']}, iGridX={building['grid_x']}) has iGreatPeopleRateChange={building['gpp']}; latest world-wonder ceiling is {world_wonder_ceiling}")

	return failures


def expected_flat_culture(wonder: dict[str, int | str], era_start_columns: dict[str, int]) -> int:
	grid_x = int(wonder["grid_x"])
	if int(wonder["culture_modifier"]) != 0 or int(wonder["global_culture_modifier"]) != 0:
		return 0
	if grid_x < era_start_columns[EARLY_CULTURE_ERA] + EARLY_CULTURE_COLUMN_COUNT:
		return 4
	if grid_x < era_start_columns[MID_CULTURE_ERA]:
		return 6
	if grid_x < era_start_columns[LATE_IDENTITY_CULTURE_ERA]:
		return 8
	if wonder["advisor"] == "ADVISOR_CULTURE":
		return 10
	return 0


def check_culture(wonders: list[dict[str, int | str]], era_start_columns: dict[str, int]) -> list[str]:
	failures: list[str] = []
	for wonder in wonders:
		flat_culture = int(wonder["culture"])
		culture_modifier = int(wonder["culture_modifier"])
		global_culture_modifier = int(wonder["global_culture_modifier"])
		expected = expected_flat_culture(wonder, era_start_columns)
		if flat_culture != 0 and (culture_modifier != 0 or global_culture_modifier != 0):
			failures.append(f"{wonder['type']}({wonder['tech']}) stacks flat culture={flat_culture} with local/global culture modifiers={culture_modifier}/{global_culture_modifier}")
			continue
		if flat_culture != expected:
			failures.append(f"{wonder['type']}({wonder['tech']}, iGridX={wonder['grid_x']}, {wonder['advisor']}) has flat culture={flat_culture}; expected {expected}")
	return failures


def check_wonder_culture_gpp_columns(repo_root: Path) -> list[str]:
	tech_columns = read_tech_columns(repo_root)
	era_start_columns = read_era_start_columns(repo_root)
	world_wonder_classes = read_world_wonder_classes(repo_root)
	class_categories = read_building_class_categories(repo_root)
	special_building_techs = read_special_building_techs(repo_root)
	wonders = read_constructible_world_wonders(repo_root, tech_columns, world_wonder_classes)
	buildings = read_constructible_gp_buildings(repo_root, tech_columns, class_categories, special_building_techs)
	by_column: dict[int, list[dict[str, int | str]]] = {}

	for wonder in wonders:
		by_column.setdefault(int(wonder["grid_x"]), []).append(wonder)

	return check_culture(wonders, era_start_columns) + check_metric(by_column, "gpp", "iGreatPeopleRateChange") + check_non_world_gpp_ceiling(wonders, buildings)


def main() -> int:
	parser = argparse.ArgumentParser(description="Check constructible world-wonder flat culture and building Great Person point rates by tech-tree column.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	try:
		failures = check_wonder_culture_gpp_columns(args.repo_root)
	except RuntimeError as exc:
		print("FAIL world-wonder culture/building GPP column consistency")
		print(f"  - {exc}")
		return 1

	if failures:
		print("FAIL world-wonder culture/building GPP column consistency")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS world-wonder culture/building GPP column consistency")
	return 0


if __name__ == "__main__":
	sys.exit(main())
