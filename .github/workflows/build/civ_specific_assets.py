#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: civilization/leader references and civilization-specific asset
# mappings should resolve consistently. Civilization-specific assets should not
# be pushed to a later starting tech column or an earlier obsolete tech column
# than the generic asset they replace.
#
# <!-- custom: Same-column tech variation is allowed.
# For example, a unique building or unit using a parallel tech at the same iGridX can be useful flavor.
# Requiring a later iGridX prereq tech is likely to be clunky because the replacement can arrive after the generic asset slot it is meant to replace.
# Obsoleting earlier is likewise suspicious because the civ-specific asset can disappear before the generic asset would. (ChatGPT-5.5) -->

from pathlib import Path
import argparse
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))

from xml_defines import get_default_repo_root, local_name, child_text_by_local_name


TECH_INFOS_REL_PATH = Path("Assets/XML/Technologies/CIV4TechInfos.xml")
CIVILIZATION_INFOS_REL_PATH = Path("Assets/XML/Civilizations/CIV4CivilizationInfos.xml")
LEADER_INFOS_REL_PATH = Path("Assets/XML/Civilizations/CIV4LeaderHeadInfos.xml")

REFERENCE_GROUPS = (
	{
		"name": "civilization art",
		"infos_path": Path("Assets/XML/Art/CIV4ArtDefines_Civilization.xml"),
		"info_tag": "CivilizationArtInfo",
	},
	{
		"name": "leader art",
		"infos_path": Path("Assets/XML/Art/CIV4ArtDefines_Leaderhead.xml"),
		"info_tag": "LeaderheadArtInfo",
	},
	{
		"name": "trait",
		"infos_path": Path("Assets/XML/Civilizations/CIV4TraitInfos.xml"),
		"info_tag": "TraitInfo",
	},
	{
		"name": "civic",
		"infos_path": Path("Assets/XML/GameInfo/CIV4CivicInfos.xml"),
		"info_tag": "CivicInfo",
	},
	{
		"name": "religion",
		"infos_path": Path("Assets/XML/GameInfo/CIV4ReligionInfo.xml"),
		"info_tag": "ReligionInfo",
	},
)

NONE_VALUES = {
	"",
	"NONE",
	"NO_TECH",
	"TECH_NONE",
	"NO_BUILDING",
	"NO_UNIT",
}
REFERENCE_TEMPLATE_TYPES = {
	# <!-- custom: LEADER_DEFAULTS is an XML tuning template, not a selectable leader with art. (GPT-5.6-Sol) -->
	"LEADER_DEFAULTS",
}

ASSET_GROUPS = (
	{
		"name": "building",
		"asset_infos_path": Path("Assets/XML/Buildings/CIV4BuildingInfos.xml"),
		"class_infos_path": Path("Assets/XML/Buildings/CIV4BuildingClassInfos.xml"),
		"asset_info_tag": "BuildingInfo",
		"class_info_tag": "BuildingClassInfo",
		"asset_type_tag": "Type",
		"asset_class_tag": "BuildingClass",
		"default_asset_tag": "DefaultBuilding",
		"civilization_container_tag": "Buildings",
		"civilization_mapping_tag": "Building",
		"civilization_class_tag": "BuildingClassType",
		"civilization_asset_tag": "BuildingType",
	},
	{
		"name": "unit",
		"asset_infos_path": Path("Assets/XML/Units/CIV4UnitInfos.xml"),
		"class_infos_path": Path("Assets/XML/Units/CIV4UnitClassInfos.xml"),
		"asset_info_tag": "UnitInfo",
		"class_info_tag": "UnitClassInfo",
		"asset_type_tag": "Type",
		"asset_class_tag": "Class",
		"default_asset_tag": "DefaultUnit",
		"civilization_container_tag": "Units",
		"civilization_mapping_tag": "Unit",
		"civilization_class_tag": "UnitClassType",
		"civilization_asset_tag": "UnitType",
	},
)

# Keep empty by default. Add only narrow, documented exceptions if a later-prereq
# or earlier-obsolete replacement is truly intentional.
ALLOWED_CIV_SPECIFIC_ASSET_TECH_EXCEPTIONS: dict[str, str] = {
}


def normalize_none(value: str | None) -> str:
	if value is None:
		return "NONE"
	value = value.strip()
	if value in NONE_VALUES:
		return "NONE"
	return value


def parse_xml(path: Path) -> ET.ElementTree:
	if not path.exists():
		raise RuntimeError(f"missing file: {path}")
	return ET.parse(path)


def read_info_types(repo_root: Path, rel_path: Path, info_tag: str) -> set[str]:
	tree = parse_xml(repo_root / rel_path)
	return {
		info_type
		for node in tree.getroot().iter()
		if local_name(node.tag) == info_tag
		for info_type in [child_text_by_local_name(node, "Type")]
		if info_type is not None
	}


def check_reference(owner_type: str, field_name: str, value: str | None, target_name: str, target_types: set[str], allow_none: bool = True) -> list[str]:
	value = normalize_none(value)
	if value == "NONE" and allow_none:
		return []
	if value in target_types:
		return []
	return [f"{owner_type}: {field_name} references missing {target_name} {value}"]


def check_civilization_and_leader_references(repo_root: Path) -> list[str]:
	reference_types = {
		group["name"]: read_info_types(repo_root, group["infos_path"], group["info_tag"])
		for group in REFERENCE_GROUPS
	}
	civilization_types = read_info_types(repo_root, CIVILIZATION_INFOS_REL_PATH, "CivilizationInfo")
	leader_types = read_info_types(repo_root, LEADER_INFOS_REL_PATH, "LeaderHeadInfo")
	failures: list[str] = []

	civilization_tree = parse_xml(repo_root / CIVILIZATION_INFOS_REL_PATH)
	for node in civilization_tree.getroot().iter():
		if local_name(node.tag) != "CivilizationInfo":
			continue
		civilization_type = child_text_by_local_name(node, "Type") or "UNKNOWN_CIVILIZATION"
		failures.extend(check_reference(civilization_type, "ArtDefineTag", child_text_by_local_name(node, "ArtDefineTag"), "civilization art", reference_types["civilization art"], False))
		failures.extend(check_reference(civilization_type, "DerivativeCiv", child_text_by_local_name(node, "DerivativeCiv"), "civilization", civilization_types))
		for child in node.iter():
			child_tag = local_name(child.tag)
			if child_tag == "LeaderName":
				failures.extend(check_reference(civilization_type, "LeaderName", child.text, "leader", leader_types, False))
			elif child_tag == "CivicType":
				failures.extend(check_reference(civilization_type, "InitialCivics/CivicType", child.text, "civic", reference_types["civic"], False))

	leader_tree = parse_xml(repo_root / LEADER_INFOS_REL_PATH)
	for node in leader_tree.getroot().iter():
		if local_name(node.tag) != "LeaderHeadInfo":
			continue
		leader_type = child_text_by_local_name(node, "Type") or "UNKNOWN_LEADER"
		if leader_type in REFERENCE_TEMPLATE_TYPES:
			continue
		failures.extend(check_reference(leader_type, "ArtDefineTag", child_text_by_local_name(node, "ArtDefineTag"), "leader art", reference_types["leader art"], False))
		failures.extend(check_reference(leader_type, "FavoriteCivic", child_text_by_local_name(node, "FavoriteCivic"), "civic", reference_types["civic"]))
		failures.extend(check_reference(leader_type, "FavoriteReligion", child_text_by_local_name(node, "FavoriteReligion"), "religion", reference_types["religion"]))
		for child in node.iter():
			if local_name(child.tag) == "TraitType":
				failures.extend(check_reference(leader_type, "Traits/TraitType", child.text, "trait", reference_types["trait"]))

	return failures


def read_class_defaults(repo_root: Path, group: dict[str, object]) -> dict[str, str]:
	path = repo_root / group["class_infos_path"]
	tree = parse_xml(path)
	default_by_class: dict[str, str] = {}

	for node in tree.getroot().iter():
		if local_name(node.tag) != group["class_info_tag"]:
			continue

		asset_class = child_text_by_local_name(node, "Type")
		default_asset = normalize_none(child_text_by_local_name(node, group["default_asset_tag"]))
		if asset_class is None:
			continue
		default_by_class[asset_class] = default_asset

	return default_by_class


def read_assets(repo_root: Path, group: dict[str, object]) -> dict[str, dict[str, str]]:
	path = repo_root / group["asset_infos_path"]
	tree = parse_xml(path)
	assets: dict[str, dict[str, str]] = {}

	for node in tree.getroot().iter():
		if local_name(node.tag) != group["asset_info_tag"]:
			continue

		asset_type = child_text_by_local_name(node, group["asset_type_tag"])
		if asset_type is None:
			continue

		asset_class = child_text_by_local_name(node, group["asset_class_tag"])
		prereq_tech = normalize_none(child_text_by_local_name(node, "PrereqTech"))
		obsolete_tech = normalize_none(child_text_by_local_name(node, "ObsoleteTech"))

		assets[asset_type] = {
			"class": asset_class or "NONE",
			"prereq_tech": prereq_tech,
			"obsolete_tech": obsolete_tech,
		}

	return assets


def check_civilization_mappings(repo_root: Path, group: dict[str, object], assets: dict[str, dict[str, str]]) -> list[str]:
	path = repo_root / CIVILIZATION_INFOS_REL_PATH
	tree = parse_xml(path)
	failures: list[str] = []

	for civilization_node in tree.getroot().iter():
		if local_name(civilization_node.tag) != "CivilizationInfo":
			continue

		civilization_type = child_text_by_local_name(civilization_node, "Type") or "UNKNOWN_CIVILIZATION"
		for container in civilization_node:
			if local_name(container.tag) != group["civilization_container_tag"]:
				continue
			for mapping in container:
				if local_name(mapping.tag) != group["civilization_mapping_tag"]:
					continue

				mapped_class = normalize_none(child_text_by_local_name(mapping, group["civilization_class_tag"]))
				asset_type = normalize_none(child_text_by_local_name(mapping, group["civilization_asset_tag"]))
				if asset_type == "NONE":
					continue
				if asset_type not in assets:
					failures.append(f"{civilization_type}: maps {mapped_class} to missing {group['name']} {asset_type}")
					continue

				declared_class = assets[asset_type]["class"]
				if mapped_class != declared_class:
					failures.append(
						f"{civilization_type}: maps {mapped_class} to {asset_type}, but that {group['name']} declares {declared_class}"
					)

	return failures


def read_tech_grid_positions(repo_root: Path) -> dict[str, tuple[int, int]]:
	path = repo_root / TECH_INFOS_REL_PATH
	tree = parse_xml(path)
	tech_grid_by_type: dict[str, tuple[int, int]] = {}

	for node in tree.getroot().iter():
		if local_name(node.tag) != "TechInfo":
			continue

		tech_type = child_text_by_local_name(node, "Type")
		if tech_type is None:
			continue

		grid_x_text = child_text_by_local_name(node, "iGridX")
		grid_y_text = child_text_by_local_name(node, "iGridY")
		try:
			grid_x = int(grid_x_text or "-1")
			grid_y = int(grid_y_text or "-1")
		except ValueError as exc:
			raise RuntimeError(f"{TECH_INFOS_REL_PATH}: {tech_type} has invalid iGridX/iGridY: {grid_x_text!r}/{grid_y_text!r}") from exc

		tech_grid_by_type[tech_type] = (grid_x, grid_y)

	return tech_grid_by_type


def tech_label(tech_type: str, tech_grid_by_type: dict[str, tuple[int, int]]) -> str:
	if tech_type == "NONE":
		return "NONE"
	if tech_type not in tech_grid_by_type:
		return f"{tech_type} (missing grid)"
	grid_x, grid_y = tech_grid_by_type[tech_type]
	return f"{tech_type} (GridX={grid_x}, GridY={grid_y})"


def check_prereq_tech(
	asset_type: str,
	asset_class: str,
	group_name: str,
	default_asset: str,
	replacement_tech: str,
	default_tech: str,
	tech_grid_by_type: dict[str, tuple[int, int]],
) -> list[str]:
	failures: list[str] = []

	if replacement_tech == "NONE":
		return failures

	if default_tech == "NONE":
		failures.append(
			f"{asset_type}: {group_name} class {asset_class} replacement requires {tech_label(replacement_tech, tech_grid_by_type)}, "
			f"but default {default_asset} has no PrereqTech"
		)
		return failures

	if replacement_tech not in tech_grid_by_type:
		failures.append(f"{asset_type}: replacement PrereqTech {replacement_tech} is missing from TechInfos")
		return failures
	if default_tech not in tech_grid_by_type:
		failures.append(f"{asset_type}: default {default_asset} PrereqTech {default_tech} is missing from TechInfos")
		return failures

	replacement_grid_x, replacement_grid_y = tech_grid_by_type[replacement_tech]
	default_grid_x, default_grid_y = tech_grid_by_type[default_tech]

	if replacement_grid_x > default_grid_x:
		failures.append(
			f"{asset_type}: {group_name} class {asset_class} replacement uses later PrereqTech column than default {default_asset}: "
			f"{replacement_tech} (GridX={replacement_grid_x}, GridY={replacement_grid_y}) after "
			f"{default_tech} (GridX={default_grid_x}, GridY={default_grid_y})"
		)

	return failures


def check_obsolete_tech(
	asset_type: str,
	asset_class: str,
	group_name: str,
	default_asset: str,
	replacement_tech: str,
	default_tech: str,
	tech_grid_by_type: dict[str, tuple[int, int]],
) -> list[str]:
	failures: list[str] = []

	if replacement_tech == "NONE":
		return failures

	if default_tech == "NONE":
		failures.append(
			f"{asset_type}: {group_name} class {asset_class} replacement obsoletes at {tech_label(replacement_tech, tech_grid_by_type)}, "
			f"but default {default_asset} has no ObsoleteTech"
		)
		return failures

	if replacement_tech not in tech_grid_by_type:
		failures.append(f"{asset_type}: replacement ObsoleteTech {replacement_tech} is missing from TechInfos")
		return failures
	if default_tech not in tech_grid_by_type:
		failures.append(f"{asset_type}: default {default_asset} ObsoleteTech {default_tech} is missing from TechInfos")
		return failures

	replacement_grid_x, replacement_grid_y = tech_grid_by_type[replacement_tech]
	default_grid_x, default_grid_y = tech_grid_by_type[default_tech]

	if replacement_grid_x < default_grid_x:
		failures.append(
			f"{asset_type}: {group_name} class {asset_class} replacement uses earlier ObsoleteTech column than default {default_asset}: "
			f"{replacement_tech} (GridX={replacement_grid_x}, GridY={replacement_grid_y}) before "
			f"{default_tech} (GridX={default_grid_x}, GridY={default_grid_y})"
		)

	return failures


def check_asset_group(repo_root: Path, group: dict[str, object], tech_grid_by_type: dict[str, tuple[int, int]]) -> list[str]:
	default_by_class = read_class_defaults(repo_root, group)
	assets = read_assets(repo_root, group)
	failures = check_civilization_mappings(repo_root, group, assets)
	group_name = group["name"]

	for asset_type in sorted(assets):
		asset = assets[asset_type]
		asset_class = asset["class"]
		if asset_class == "NONE":
			continue

		default_asset = default_by_class.get(asset_class, "NONE")
		if default_asset == "NONE":
			continue
		if asset_type == default_asset:
			continue
		if default_asset not in assets:
			failures.append(f"{asset_type}: {group_name} class {asset_class} default asset {default_asset} is missing from asset infos")
			continue

		if asset_type in ALLOWED_CIV_SPECIFIC_ASSET_TECH_EXCEPTIONS:
			continue

		default = assets[default_asset]

		failures.extend(
			check_prereq_tech(
				asset_type,
				asset_class,
				group_name,
				default_asset,
				asset["prereq_tech"],
				default["prereq_tech"],
				tech_grid_by_type,
			)
		)

		failures.extend(
			check_obsolete_tech(
				asset_type,
				asset_class,
				group_name,
				default_asset,
				asset["obsolete_tech"],
				default["obsolete_tech"],
				tech_grid_by_type,
			)
		)

	return failures


def check_civilization_assets(repo_root: Path) -> list[str]:
	tech_grid_by_type = read_tech_grid_positions(repo_root)
	failures = check_civilization_and_leader_references(repo_root)

	for group in ASSET_GROUPS:
		failures.extend(check_asset_group(repo_root, group, tech_grid_by_type))

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check civilization/leader references, civilization-specific building/unit mappings, and asset tech timing.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	try:
		failures = check_civilization_assets(args.repo_root)
	except RuntimeError as exc:
		print("FAIL civilization-specific assets")
		print(f"  - {exc}")
		return 1

	if failures:
		print("FAIL civilization-specific assets")
		for failure in failures:
			print(f"  - {failure}")
		print("  - Same-GridX parallel tech variation is allowed.")
		print("  - PrereqTech fails only when a replacement starts later than its generic asset.")
		print("  - ObsoleteTech fails only when a replacement obsoletes earlier than its generic asset.")
		return 1

	print("PASS civilization-specific assets")
	return 0


if __name__ == "__main__":
	sys.exit(main())
