#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: DetailManager city billboard scale must match the committed
# SAS font/default billboard-scale settings and the runtime scaler base values.

from pathlib import Path
import argparse
import ast
import re
import sys
import xml.etree.ElementTree as ET

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root, read_global_define_ints, local_name, child_text_by_local_name


DETAIL_MANAGER_REL_PATH = Path("Assets") / "XML" / "Misc" / "CIV4DetailManager.xml"
BILLBOARD_SCALE_REL_PATH = Path("Assets") / "Python" / "SASBillboardScale.py"
BILLBOARD_FADER_NAME = "CITYBILLBOARD_SCALE"
BASE_KEYS_VARIABLE_NAME = "_BASE_BILLBOARD_KEYS"


def read_base_billboard_keys(repo_root: Path) -> list[tuple[int, float]]:
	path = repo_root / BILLBOARD_SCALE_REL_PATH
	if not path.exists():
		raise RuntimeError(f"missing file: {path}")

	module = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))

	for node in module.body:
		if not isinstance(node, ast.Assign):
			continue
		for target in node.targets:
			if isinstance(target, ast.Name) and target.id == BASE_KEYS_VARIABLE_NAME:
				value = ast.literal_eval(node.value)
				return normalize_keys(value, f"{path}:{BASE_KEYS_VARIABLE_NAME}")

	raise RuntimeError(f"{BASE_KEYS_VARIABLE_NAME} not found in {path}")


def normalize_keys(value: object, source: str) -> list[tuple[int, float]]:
	if not isinstance(value, (list, tuple)):
		raise RuntimeError(f"{source} must be a list/tuple of (distance, scale) pairs")

	keys: list[tuple[int, float]] = []
	for index, item in enumerate(value):
		if not isinstance(item, (list, tuple)) or len(item) != 2:
			raise RuntimeError(f"{source}[{index}] must be a (distance, scale) pair")
		distance, scale = item
		if not isinstance(distance, int):
			raise RuntimeError(f"{source}[{index}] distance must be int, found {distance!r}")
		if not isinstance(scale, (int, float)):
			raise RuntimeError(f"{source}[{index}] scale must be numeric, found {scale!r}")
		keys.append((distance, float(scale)))

	if not keys:
		raise RuntimeError(f"{source} must not be empty")

	return keys


def read_detail_manager_billboard_keys(repo_root: Path) -> list[tuple[int, str]]:
	path = repo_root / DETAIL_MANAGER_REL_PATH
	if not path.exists():
		raise RuntimeError(f"missing file: {path}")

	tree = ET.parse(path)
	for fader in tree.getroot().iter():
		if local_name(fader.tag) != "Fader":
			continue
		if child_text_by_local_name(fader, "Name") != BILLBOARD_FADER_NAME:
			continue

		keys: list[tuple[int, str]] = []
		for child in list(fader):
			if local_name(child.tag) != "Key":
				continue
			text = (child.text or "").strip()
			match = re.match(r"^(-?\d+)\s*,\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+))\s*$", text)
			if match is None:
				raise RuntimeError(f"invalid {BILLBOARD_FADER_NAME} <Key> value in {path}: {text!r}")
			keys.append((int(match.group(1)), format(float(match.group(2)), ".2f")))

		if not keys:
			raise RuntimeError(f"{BILLBOARD_FADER_NAME} has no <Key> values in {path}")
		return keys

	raise RuntimeError(f"{BILLBOARD_FADER_NAME} fader not found in {path}")


def get_active_billboard_percent(defines: dict[str, int]) -> tuple[int, int]:
	font_name = "SAS_UI_FONT_BODY"
	if font_name not in defines:
		raise RuntimeError(f"{font_name}: missing define")

	font_body = defines[font_name]
	if font_body < 1 or font_body > 4:
		raise RuntimeError(f"{font_name}: expected 1..4, found {font_body}")

	percent_name = f"SAS_BILLBOARD_SCALE_PERCENT_FONT_{font_body}"
	if percent_name not in defines:
		raise RuntimeError(f"{percent_name}: missing define")

	percent = defines[percent_name]
	if percent < 10 or percent > 500:
		raise RuntimeError(f"{percent_name}: expected 10..500, found {percent}")

	return font_body, percent


def expected_keys(base_keys: list[tuple[int, float]], percent: int) -> list[tuple[int, str]]:
	multiplier = percent / 100.0
	return [(distance, format(scale * multiplier, ".2f")) for distance, scale in base_keys]


def key_lines(keys: list[tuple[int, str]]) -> list[str]:
	return [f"<Key>{distance},    {scale}</Key>" for distance, scale in keys]


def compare_keys(expected: list[tuple[int, str]], actual: list[tuple[int, str]]) -> list[str]:
	failures: list[str] = []

	if len(expected) != len(actual):
		failures.append(f"expected {len(expected)} CITYBILLBOARD_SCALE keys, found {len(actual)}")

	for index, (expected_key, actual_key) in enumerate(zip(expected, actual), start=1):
		if expected_key != actual_key:
			failures.append(f"key {index}: expected {expected_key[0]}, {expected_key[1]}; found {actual_key[0]}, {actual_key[1]}")

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that CIV4DetailManager CITYBILLBOARD_SCALE matches SAS font/default billboard-scale settings.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	try:
		defines = read_global_define_ints(args.repo_root)
		font_body, percent = get_active_billboard_percent(defines)
		base_keys = read_base_billboard_keys(args.repo_root)
		expected = expected_keys(base_keys, percent)
		actual = read_detail_manager_billboard_keys(args.repo_root)
		failures = compare_keys(expected, actual)
	except RuntimeError as exc:
		print("FAIL DetailManager CITYBILLBOARD_SCALE")
		print(f"  - {exc}")
		return 1

	if failures:
		print("FAIL DetailManager CITYBILLBOARD_SCALE")
		print(f"  - SAS_UI_FONT_BODY={font_body}; SAS_BILLBOARD_SCALE_PERCENT_FONT_{font_body}={percent}")
		for failure in failures:
			print(f"  - {failure}")
		print("  - expected keys:")
		for line in key_lines(expected):
			print(f"      {line}")
		print("  - found keys:")
		for line in key_lines(actual):
			print(f"      {line}")
		print("  - If these are stale after temporary UI upscaling, launch the mod once with the committed font defaults or restore CIV4DetailManager.xml to the expected keys.")
		return 1

	print(f"PASS DetailManager CITYBILLBOARD_SCALE matches SAS_UI_FONT_BODY={font_body} / {percent}% billboard scale")
	return 0


if __name__ == "__main__":
	sys.exit(main())
