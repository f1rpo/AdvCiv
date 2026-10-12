#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Check that normally constructible world wonders in the same technology-cost tier have the same production cost and that later, more expensive technology tiers have higher wonder costs. (GPT-5.6-Sol) -->

from pathlib import Path
import argparse
import sys
import xml.etree.ElementTree as ET

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root, local_name, child_text_by_local_name


TECH_INFOS_REL_PATH = Path("Assets/XML/Technologies/CIV4TechInfos.xml")
BUILDING_CLASS_INFOS_REL_PATH = Path("Assets/XML/Buildings/CIV4BuildingClassInfos.xml")
BUILDING_INFOS_REL_PATH = Path("Assets/XML/Buildings/CIV4BuildingInfos.xml")

# <!-- custom: The UN directly enables a diplomatic victory and is intentionally much more expensive to keep games from ending too soon; its XML comment records this exception. (GPT-5.6-Sol) -->
ALLOWED_WORLD_WONDER_COST_EXCEPTIONS: dict[str, str] = {
	"BUILDING_UNITED_NATIONS": "intentionally expensive diplomatic-victory wonder",
}


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


def read_tech_column_costs(repo_root: Path) -> dict[int, int]:
	path = repo_root / TECH_INFOS_REL_PATH
	by_column: dict[int, set[int]] = {}

	for node in parse_xml(path).getroot().iter():
		if local_name(node.tag) != "TechInfo":
			continue
		tech_type = child_text_by_local_name(node, "Type")
		grid_x_text = child_text_by_local_name(node, "iGridX")
		cost_text = child_text_by_local_name(node, "iCost")
		if tech_type is None:
			continue
		try:
			grid_x = int(grid_x_text or "-1")
			cost = int(cost_text or "-1")
		except ValueError as exc:
			raise RuntimeError(f"{TECH_INFOS_REL_PATH}: {tech_type} has invalid iGridX/iCost {grid_x_text!r}/{cost_text!r}") from exc
		by_column.setdefault(grid_x, set()).add(cost)

	column_costs: dict[int, int] = {}
	for grid_x, costs in by_column.items():
		if len(costs) != 1:
			raise RuntimeError(f"{TECH_INFOS_REL_PATH}: iGridX={grid_x} has multiple technology costs: {sorted(costs)}")
		column_costs[grid_x] = next(iter(costs))
	return column_costs


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
		if building_type in ALLOWED_WORLD_WONDER_COST_EXCEPTIONS:
			continue

		cost_text = child_text_by_local_name(node, "iCost")
		try:
			cost = int(cost_text or "-1")
		except ValueError as exc:
			raise RuntimeError(f"{BUILDING_INFOS_REL_PATH}: {building_type} has invalid iCost {cost_text!r}") from exc
		# <!-- custom: Great People found shrines and corporation headquarters with iCost=-1, so they are not normally constructible production comparisons. (GPT-5.6-Sol) -->
		if cost < 0:
			continue

		prereq_tech = child_text_by_local_name(node, "PrereqTech")
		if prereq_tech not in tech_columns:
			raise RuntimeError(f"{BUILDING_INFOS_REL_PATH}: {building_type} has missing or unknown PrereqTech {prereq_tech!r}")

		wonders.append({
			"type": building_type,
			"tech": prereq_tech,
			"grid_x": tech_columns[prereq_tech],
			"cost": cost,
		})

	return wonders


def format_wonders(wonders: list[dict[str, int | str]]) -> str:
	return ", ".join(f"{wonder['type']}({wonder['tech']})" for wonder in sorted(wonders, key=lambda item: str(item["type"])))


def check_wonder_cost_columns(repo_root: Path) -> list[str]:
	tech_columns = read_tech_columns(repo_root)
	tech_column_costs = read_tech_column_costs(repo_root)
	world_wonder_classes = read_world_wonder_classes(repo_root)
	wonders = read_constructible_world_wonders(repo_root, tech_columns, world_wonder_classes)
	by_column: dict[int, list[dict[str, int | str]]] = {}

	for wonder in wonders:
		by_column.setdefault(int(wonder["grid_x"]), []).append(wonder)

	failures: list[str] = []
	column_costs: dict[int, int] = {}
	for grid_x in sorted(by_column):
		by_cost: dict[int, list[dict[str, int | str]]] = {}
		for wonder in by_column[grid_x]:
			by_cost.setdefault(int(wonder["cost"]), []).append(wonder)
		if len(by_cost) != 1:
			parts = [f"iCost={cost}: {format_wonders(by_cost[cost])}" for cost in sorted(by_cost)]
			failures.append(f"iGridX={grid_x} has multiple constructible world-wonder costs: " + "; ".join(parts))
			continue
		column_costs[grid_x] = next(iter(by_cost))

	previous_column = None
	previous_cost = None
	for grid_x in sorted(column_costs):
		cost = column_costs[grid_x]
		if previous_cost is not None:
			shared_timing_tier = tech_column_costs[grid_x] == tech_column_costs[previous_column]
			if shared_timing_tier and cost != previous_cost:
				failures.append(f"world-wonder cost differs across shared technology-cost tier iGridX={previous_column}/{grid_x}: iCost={previous_cost}/{cost}")
			elif not shared_timing_tier and cost <= previous_cost:
				failures.append(f"world-wonder cost does not increase from iGridX={previous_column} (iCost={previous_cost}) to iGridX={grid_x} (iCost={cost})")
		previous_column = grid_x
		previous_cost = cost

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check constructible world-wonder production costs by tech-tree column.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	try:
		failures = check_wonder_cost_columns(args.repo_root)
	except RuntimeError as exc:
		print("FAIL world-wonder cost column consistency")
		print(f"  - {exc}")
		return 1

	if failures:
		print("FAIL world-wonder cost column consistency")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS world-wonder cost column consistency")
	return 0


if __name__ == "__main__":
	sys.exit(main())
