#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: techs in the same tech-tree column should use the same core
# column values.
#
# <!-- custom: This checks parallel tech timing by iGridX, not iGridY.
# iGridY is only the vertical row/layout position; iGridX is the tech-tree column/progress position.
# Different rows in the same column should normally share values such as iCost and iAsset so that parallel same-timing techs do not get inconsistent research cost, asset/trade valuation, or AI valuation pressure. (ChatGPT-5.5) -->

from pathlib import Path
import argparse
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))

from xml_defines import get_default_repo_root, local_name, child_text_by_local_name


TECH_INFOS_REL_PATH = Path("Assets/XML/Technologies/CIV4TechInfos.xml")
COLUMN_VALUE_TAGS = (
	"iCost",
	"iAsset",
)

# Keep empty by default. Add only narrow, documented exceptions if a same-column
# value mismatch is truly intentional.
ALLOWED_TECH_COLUMN_VALUE_EXCEPTIONS: dict[str, str] = {
}


def parse_xml(path: Path) -> ET.ElementTree:
	if not path.exists():
		raise RuntimeError(f"missing file: {path}")
	return ET.parse(path)


def read_techs(repo_root: Path) -> list[dict[str, int | str]]:
	path = repo_root / TECH_INFOS_REL_PATH
	tree = parse_xml(path)
	techs: list[dict[str, int | str]] = []

	for node in tree.getroot().iter():
		if local_name(node.tag) != "TechInfo":
			continue

		tech_type = child_text_by_local_name(node, "Type")
		if tech_type is None:
			continue

		grid_x_text = child_text_by_local_name(node, "iGridX")
		grid_y_text = child_text_by_local_name(node, "iGridY")

		try:
			tech: dict[str, int | str] = {
				"type": tech_type,
				"grid_x": int(grid_x_text or "-1"),
				"grid_y": int(grid_y_text or "-1"),
			}

			for tag in COLUMN_VALUE_TAGS:
				tech[tag] = int(child_text_by_local_name(node, tag) or "0")
		except ValueError as exc:
			values = "/".join([grid_x_text or "", grid_y_text or ""] + [child_text_by_local_name(node, tag) or "" for tag in COLUMN_VALUE_TAGS])
			raise RuntimeError(f"{TECH_INFOS_REL_PATH}: {tech_type} has invalid numeric column value(s): {values!r}") from exc

		techs.append(tech)

	return techs


def format_tech_list(techs: list[dict[str, int | str]]) -> str:
	return ", ".join(
		f"{tech['type']}(GridY={tech['grid_y']})"
		for tech in sorted(techs, key=lambda item: (int(item["grid_y"]), str(item["type"])))
	)


def check_column_field(
	grid_x: int,
	column_techs: list[dict[str, int | str]],
	field_name: str,
) -> list[str]:
	failures: list[str] = []
	by_value: dict[int, list[dict[str, int | str]]] = {}

	for tech in column_techs:
		by_value.setdefault(int(tech[field_name]), []).append(tech)

	if len(by_value) <= 1:
		return failures

	parts = []
	for value in sorted(by_value):
		parts.append(f"{field_name}={value}: {format_tech_list(by_value[value])}")

	failures.append(f"iGridX={grid_x} has multiple {field_name} values: " + "; ".join(parts))
	return failures


def check_tech_column_values(repo_root: Path) -> list[str]:
	techs = read_techs(repo_root)
	by_grid_x: dict[int, list[dict[str, int | str]]] = {}

	for tech in techs:
		tech_type = str(tech["type"])
		if tech_type in ALLOWED_TECH_COLUMN_VALUE_EXCEPTIONS:
			continue
		by_grid_x.setdefault(int(tech["grid_x"]), []).append(tech)

	failures: list[str] = []
	for grid_x in sorted(by_grid_x):
		column_techs = by_grid_x[grid_x]
		for field_name in COLUMN_VALUE_TAGS:
			failures.extend(check_column_field(grid_x, column_techs, field_name))

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that techs in the same iGridX column share the same core column values.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	try:
		failures = check_tech_column_values(args.repo_root)
	except RuntimeError as exc:
		print("FAIL tech column value consistency")
		print(f"  - {exc}")
		return 1

	if failures:
		print("FAIL tech column value consistency")
		for failure in failures:
			print(f"  - {failure}")
		print("  - Same iGridX means the techs are in the same tech-tree column; iGridY row differences are allowed, but core column values should normally match within that column.")
		return 1

	print("PASS tech column value consistency")
	return 0


if __name__ == "__main__":
	sys.exit(main())
