#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
#
# Build check: hardcoded WorldSizeTypes enum values must match CIV4WorldInfo.xml order.

from pathlib import Path
import argparse
import re
import sys
import xml.etree.ElementTree as ET


WORLD_INFO_RELATIVE_PATH = Path("Assets/XML/GameInfo/CIV4WorldInfo.xml")
CV_ENUMS_RELATIVE_PATH = Path("CvGameCoreDLL/CvEnums.h")
CY_ENUMS_INTERFACE_RELATIVE_PATH = Path("CvGameCoreDLL/CyEnumsInterface.cpp")
PYTHON_RUNTIME_PATHS = (Path("Assets/Python"), Path("PrivateMaps"))
OLD_MAGIC_PREFIX = "SAS_MAGIC_WORLDSIZE_"


def get_default_repo_root() -> Path:
	return Path(__file__).resolve().parents[4]


def local_name(tag: str) -> str:
	return tag.rsplit("}", 1)[-1]


def child_text_by_local_name(node: ET.Element, child_name: str) -> str | None:
	for child in list(node):
		if local_name(child.tag) == child_name:
			return (child.text or "").strip()
	return None


def read_worldinfo_types(repo_root: Path) -> list[str]:
	path = repo_root / WORLD_INFO_RELATIVE_PATH
	if not path.exists():
		raise RuntimeError(f"missing file: {path}")

	tree = ET.parse(path)
	world_types: list[str] = []

	for node in tree.getroot().iter():
		if local_name(node.tag) != "WorldInfo":
			continue

		type_text = child_text_by_local_name(node, "Type")
		if not type_text:
			world_types.append("<missing Type>")
			continue
		world_types.append(type_text)

	return world_types


def extract_worldsize_enum_block(repo_root: Path) -> str:
	path = repo_root / CV_ENUMS_RELATIVE_PATH
	if not path.exists():
		raise RuntimeError(f"missing file: {path}")

	text = path.read_text(encoding="utf-8")
	match = re.search(r"enum\s+WorldSizeTypes\s*\{(?P<body>.*?)\};", text, re.S)
	if not match:
		raise RuntimeError(f"{CV_ENUMS_RELATIVE_PATH}: missing enum WorldSizeTypes block")
	return match.group("body")


def read_cpp_worldsize_enum_types(repo_root: Path) -> list[str]:
	body = extract_worldsize_enum_block(repo_root)
	enum_types: list[str] = []

	for raw_line in body.splitlines():
		line = raw_line.split("//", 1)[0].strip().rstrip(",")
		if not line or line.startswith("NO_WORLDSIZE"):
			continue
		name = line.split("=", 1)[0].strip()
		if name:
			enum_types.append(name)

	return enum_types


def read_python_exposed_worldsize_types(repo_root: Path) -> list[str]:
	path = repo_root / CY_ENUMS_INTERFACE_RELATIVE_PATH
	if not path.exists():
		raise RuntimeError(f"missing file: {path}")

	text = path.read_text(encoding="utf-8")
	match = re.search(r"python::enum_<WorldSizeTypes>\(\"WorldSizeTypes\"\)(?P<body>.*?);", text, re.S)
	if not match:
		raise RuntimeError(f"{CY_ENUMS_INTERFACE_RELATIVE_PATH}: missing python::enum_<WorldSizeTypes> block")

	return [name for name in re.findall(r"\.value\(\"(WORLDSIZE_[A-Z0-9_]+)\"\s*,\s*\1\)", match.group("body"))]


def check_no_old_magic_worldsizes(repo_root: Path) -> list[str]:
	failures: list[str] = []
	for relative_path in PYTHON_RUNTIME_PATHS:
		root = repo_root / relative_path
		if not root.exists():
			continue
		for path in sorted(root.rglob("*.py")):
			text = path.read_text(encoding="utf-8", errors="replace")
			if OLD_MAGIC_PREFIX in text:
				failures.append(f"{path.relative_to(repo_root)}: old {OLD_MAGIC_PREFIX} constant reference remains")
	return failures


def check_worldsizes(repo_root: Path) -> list[str]:
	failures: list[str] = []

	world_types = read_worldinfo_types(repo_root)
	cpp_types = read_cpp_worldsize_enum_types(repo_root)
	python_types = read_python_exposed_worldsize_types(repo_root)

	if len(set(world_types)) != len(world_types):
		seen: set[str] = set()
		duplicates: list[str] = []
		for world_type in world_types:
			if world_type in seen and world_type not in duplicates:
				duplicates.append(world_type)
			seen.add(world_type)
		failures.append(f"{WORLD_INFO_RELATIVE_PATH}: duplicate world-size Type entries: {', '.join(duplicates)}")

	if not world_types:
		failures.append(f"{WORLD_INFO_RELATIVE_PATH}: no WorldInfo entries found")

	if cpp_types != world_types:
		failures.append(f"{CV_ENUMS_RELATIVE_PATH}: WorldSizeTypes order does not match {WORLD_INFO_RELATIVE_PATH}; expected {world_types}, found {cpp_types}")

	if python_types != world_types:
		failures.append(f"{CY_ENUMS_INTERFACE_RELATIVE_PATH}: exposed WorldSizeTypes order does not match {WORLD_INFO_RELATIVE_PATH}; expected {world_types}, found {python_types}")

	failures.extend(check_no_old_magic_worldsizes(repo_root))

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that AdvCiv-SAS WorldSizeTypes enum values match CIV4WorldInfo.xml order.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	failures = check_worldsizes(args.repo_root)

	if failures:
		print("FAIL world-size enum alignment")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS world-size enum alignment")
	return 0


if __name__ == "__main__":
	sys.exit(main())
