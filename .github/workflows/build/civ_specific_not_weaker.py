#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: civilization-specific unit and building replacements should not
# be weaker than the generic asset they replace.
#
# <!-- custom: This is intentionally strict for AdvCiv-SAS.
# Unique units and buildings may add flavor, timing, cost, or special rules, but they should not quietly lose the core player-facing strength/power baseline of the generic slot.
# Units are checked for both iCombat and iPower; buildings have no combat strength, so iPower is the closest comparable XML baseline. (ChatGPT-5.5) -->

from pathlib import Path
import argparse
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))

from xml_defines import get_default_repo_root, local_name, child_text_by_local_name


NONE_VALUES = {
	"",
	"NONE",
	"NO_UNIT",
	"NO_BUILDING",
}

ASSET_GROUPS = (
	{
		"name": "unit",
		"asset_infos_path": Path("Assets/XML/Units/CIV4UnitInfos.xml"),
		"class_infos_path": Path("Assets/XML/Units/CIV4UnitClassInfos.xml"),
		"asset_info_tag": "UnitInfo",
		"class_info_tag": "UnitClassInfo",
		"civ_overrides_parent_tag": "Units",
		"civ_override_tag": "Unit",
		"civ_class_tag": "UnitClassType",
		"civ_asset_tag": "UnitType",
		"default_asset_tag": "DefaultUnit",
		"lower_value_tags": ("iCombat", "iPower"),
	},
	{
		"name": "building",
		"asset_infos_path": Path("Assets/XML/Buildings/CIV4BuildingInfos.xml"),
		"class_infos_path": Path("Assets/XML/Buildings/CIV4BuildingClassInfos.xml"),
		"asset_info_tag": "BuildingInfo",
		"class_info_tag": "BuildingClassInfo",
		"civ_overrides_parent_tag": "Buildings",
		"civ_override_tag": "Building",
		"civ_class_tag": "BuildingClassType",
		"civ_asset_tag": "BuildingType",
		"default_asset_tag": "DefaultBuilding",
		"lower_value_tags": ("iPower",),
	},
)


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


def read_assets(repo_root: Path, group: dict[str, object]) -> dict[str, ET.Element]:
	path = repo_root / group["asset_infos_path"]
	tree = parse_xml(path)
	asset_by_type: dict[str, ET.Element] = {}

	for node in tree.getroot().iter():
		if local_name(node.tag) != group["asset_info_tag"]:
			continue

		asset_type = child_text_by_local_name(node, "Type")
		if asset_type is not None:
			asset_by_type[asset_type] = node

	return asset_by_type


def iter_named_direct_children(node: ET.Element, child_name: str):
	for child in list(node):
		if local_name(child.tag) == child_name:
			yield child


def read_civ_overrides(repo_root: Path, group: dict[str, object]) -> list[tuple[str, str, str]]:
	path = repo_root / "Assets/XML/Civilizations/CIV4CivilizationInfos.xml"
	tree = parse_xml(path)
	overrides: list[tuple[str, str, str]] = []

	for civ in tree.getroot().iter():
		if local_name(civ.tag) != "CivilizationInfo":
			continue

		civ_type = child_text_by_local_name(civ, "Type")
		if civ_type is None:
			continue

		for parent in iter_named_direct_children(civ, group["civ_overrides_parent_tag"]):
			for override in iter_named_direct_children(parent, group["civ_override_tag"]):
				asset_class = child_text_by_local_name(override, group["civ_class_tag"])
				asset_type = normalize_none(child_text_by_local_name(override, group["civ_asset_tag"]))
				if asset_class is not None and asset_type != "NONE":
					overrides.append((civ_type, asset_class, asset_type))

	return overrides


def read_int(node: ET.Element, tag: str) -> int:
	value = child_text_by_local_name(node, tag)
	if value is None or value == "":
		return 0
	try:
		return int(value)
	except ValueError as exc:
		asset_type = child_text_by_local_name(node, "Type") or "<unknown>"
		raise RuntimeError(f"{asset_type} has non-int {tag}: {value!r}") from exc


def audit_group(repo_root: Path, group: dict[str, object]) -> tuple[list[str], int]:
	failures: list[str] = []
	checked = 0

	default_by_class = read_class_defaults(repo_root, group)
	asset_by_type = read_assets(repo_root, group)
	overrides = read_civ_overrides(repo_root, group)

	for civ_type, asset_class, asset_type in sorted(overrides):
		default_asset = normalize_none(default_by_class.get(asset_class))
		if default_asset == "NONE" or asset_type == default_asset:
			continue

		checked += 1
		asset_info = asset_by_type.get(asset_type)
		default_info = asset_by_type.get(default_asset)

		if asset_info is None:
			failures.append(f"{civ_type}: {asset_type} overrides {asset_class}, but {group['asset_info_tag']} is missing")
			continue
		if default_info is None:
			failures.append(f"{civ_type}: default {group['name']} {default_asset} for {asset_class} is missing")
			continue

		for value_tag in group["lower_value_tags"]:
			asset_value = read_int(asset_info, value_tag)
			default_value = read_int(default_info, value_tag)
			if asset_value < default_value:
				failures.append(
					f"{civ_type}: {asset_type} replaces {default_asset} in {asset_class} but {value_tag} is lower: {asset_value} < {default_value}"
				)

	return failures, checked


def main() -> int:
	parser = argparse.ArgumentParser(description="Fail if civilization-specific units/buildings are weaker than the generic assets they replace.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	all_failures: list[str] = []
	checked_by_group: dict[str, int] = {}

	for group in ASSET_GROUPS:
		failures, checked = audit_group(args.repo_root, group)
		all_failures.extend(failures)
		checked_by_group[group["name"]] = checked

	if all_failures:
		print("FAIL civ-specific not-weaker check")
		print(f"Checked {checked_by_group['unit']} civ-specific unit replacements and {checked_by_group['building']} civ-specific building replacements.")
		for failure in all_failures:
			print(f"  - {failure}")
		return 1

	print(
		"PASS civ-specific not-weaker check: "
		f"checked {checked_by_group['unit']} unit replacements and {checked_by_group['building']} building replacements; "
		"no strength/power downgrades found."
	)
	return 0


if __name__ == "__main__":
	sys.exit(main())
