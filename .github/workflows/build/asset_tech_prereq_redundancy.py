#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Flag stale additional unit/building tech requirements when the primary tech already guarantees them through the tech graph.
# Limit the rule to prerequisites from an earlier era that remain attached at least two columns into the primary tech's era, so close foundational requirements can remain explicit without preserving very old no-effect requirements after tech-tree moves. (GPT-5.6-Sol) -->

from pathlib import Path
import argparse
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))

from xml_defines import get_default_repo_root, local_name, child_text_by_local_name


TECH_INFOS_REL_PATH = Path("Assets/XML/Technologies/CIV4TechInfos.xml")
ERA_INFOS_REL_PATH = Path("Assets/XML/GameInfo/CIV4EraInfos.xml")
ASSET_INFOS = (
	(Path("Assets/XML/Units/CIV4UnitInfos.xml"), "UnitInfo", "unit"),
	(Path("Assets/XML/Buildings/CIV4BuildingInfos.xml"), "BuildingInfo", "building"),
)
STALE_PRIMARY_ERA_COLUMN_OFFSET = 2


def parse_xml(path: Path) -> ET.ElementTree:
	if not path.exists():
		raise RuntimeError(f"missing file: {path}")
	return ET.parse(path)


def read_prereq_list(node: ET.Element, field_name: str) -> list[str]:
	for child in list(node):
		if local_name(child.tag) == field_name:
			return [(entry.text or "").strip() for entry in list(child) if local_name(entry.tag) == "PrereqTech" and (entry.text or "").strip() not in ("", "NONE")]
	return []


def read_era_order(repo_root: Path) -> dict[str, int]:
	path = repo_root / ERA_INFOS_REL_PATH
	order: dict[str, int] = {}
	for node in parse_xml(path).getroot().iter():
		if local_name(node.tag) != "EraInfo":
			continue
		era_type = child_text_by_local_name(node, "Type")
		if era_type is not None:
			order[era_type] = len(order)
	return order


def read_techs(repo_root: Path, era_order: dict[str, int]) -> tuple[dict[str, dict[str, int | str | list[str]]], dict[str, int]]:
	path = repo_root / TECH_INFOS_REL_PATH
	techs: dict[str, dict[str, int | str | list[str]]] = {}
	era_starts: dict[str, int] = {}

	for node in parse_xml(path).getroot().iter():
		if local_name(node.tag) != "TechInfo":
			continue
		tech_type = child_text_by_local_name(node, "Type")
		era_type = child_text_by_local_name(node, "Era")
		grid_x_text = child_text_by_local_name(node, "iGridX")
		if tech_type is None or era_type not in era_order:
			raise RuntimeError(f"{TECH_INFOS_REL_PATH}: tech has missing Type or unknown Era {era_type!r}")
		try:
			grid_x = int(grid_x_text or "-1")
		except ValueError as exc:
			raise RuntimeError(f"{TECH_INFOS_REL_PATH}: {tech_type} has invalid iGridX {grid_x_text!r}") from exc
		techs[tech_type] = {
			"era": era_type,
			"grid_x": grid_x,
			"or_prereqs": read_prereq_list(node, "OrPreReqs"),
			"and_prereqs": read_prereq_list(node, "AndPreReqs"),
		}
		era_starts[era_type] = min(era_starts.get(era_type, grid_x), grid_x)

	return techs, era_starts


def guaranteed_prereqs(tech_type: str, techs: dict[str, dict[str, int | str | list[str]]], cache: dict[str, set[str]], active: set[str]) -> set[str]:
	if tech_type in cache:
		return cache[tech_type]
	if tech_type in active:
		raise RuntimeError(f"{TECH_INFOS_REL_PATH}: prerequisite cycle reaches {tech_type}")
	if tech_type not in techs:
		raise RuntimeError(f"{TECH_INFOS_REL_PATH}: unknown prerequisite tech {tech_type}")

	result: set[str] = set()
	next_active = active | {tech_type}
	for prereq in techs[tech_type]["and_prereqs"]:
		prereq_type = str(prereq)
		result.add(prereq_type)
		result.update(guaranteed_prereqs(prereq_type, techs, cache, next_active))

	or_branches: list[set[str]] = []
	for prereq in techs[tech_type]["or_prereqs"]:
		prereq_type = str(prereq)
		or_branches.append({prereq_type} | guaranteed_prereqs(prereq_type, techs, cache, next_active))
	if or_branches:
		result.update(set.intersection(*or_branches))

	cache[tech_type] = result
	return result


def check_asset_tech_prereq_redundancy(repo_root: Path) -> list[str]:
	failures: list[str] = []
	era_order = read_era_order(repo_root)
	techs, era_starts = read_techs(repo_root, era_order)
	guaranteed_cache: dict[str, set[str]] = {}

	for relative_path, node_name, asset_label in ASSET_INFOS:
		for node in parse_xml(repo_root / relative_path).getroot().iter():
			if local_name(node.tag) != node_name:
				continue
			asset_type = child_text_by_local_name(node, "Type")
			primary_tech = child_text_by_local_name(node, "PrereqTech")
			if asset_type is None or primary_tech not in techs:
				continue
			primary_era = str(techs[primary_tech]["era"])
			primary_grid_x = int(techs[primary_tech]["grid_x"])
			if primary_grid_x < era_starts[primary_era] + STALE_PRIMARY_ERA_COLUMN_OFFSET:
				continue
			guaranteed = guaranteed_prereqs(primary_tech, techs, guaranteed_cache, set())
			for additional_tech in read_prereq_list(node, "TechTypes"):
				if additional_tech not in techs:
					raise RuntimeError(f"{relative_path}: {asset_type} has unknown additional prerequisite {additional_tech}")
				additional_era = str(techs[additional_tech]["era"])
				if additional_tech in guaranteed and era_order[additional_era] < era_order[primary_era]:
					failures.append(f"{asset_label} {asset_type}: additional {additional_tech} ({additional_era}, iGridX={techs[additional_tech]['grid_x']}) is already guaranteed by primary {primary_tech} ({primary_era}, iGridX={primary_grid_x})")

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check for stale redundant additional unit/building tech prerequisites.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	try:
		failures = check_asset_tech_prereq_redundancy(args.repo_root)
	except RuntimeError as exc:
		print("FAIL asset tech prerequisite redundancy")
		print(f"  - {exc}")
		return 1

	if failures:
		print("FAIL asset tech prerequisite redundancy")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS asset tech prerequisite redundancy")
	return 0


if __name__ == "__main__":
	sys.exit(main())
