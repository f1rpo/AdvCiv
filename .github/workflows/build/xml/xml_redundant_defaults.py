#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from pathlib import Path
import sys
import xml.etree.ElementTree as ET

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root


XML_ROOT = Path("Assets/XML")
IGNORED_DIRS = {Path("Assets/XML/Text")}
NO_TECH_VALUES = {"", "NONE", "NO_TECH"}


def local_name(tag: str) -> str:
	return tag.rsplit("}", 1)[-1]


def child_text(node: ET.Element, name: str) -> str:
	for child in node:
		if local_name(child.tag) == name:
			return (child.text or "").strip()
	return ""


def object_key(stack: list[ET.Element]) -> str:
	for node in reversed(stack):
		for key_name in ("Type", "DefineName", "Tag", "ScriptID", "SoundID"):
			value = child_text(node, key_name)
			if value:
				return value
	return "(unknown object)"


def scan_file(path: Path, display_path: Path) -> list[str]:
	errors: list[str] = []
	try:
		root = ET.parse(path).getroot()
	except (ET.ParseError, UnicodeDecodeError) as exc:
		return [f"{display_path}: could not parse while checking redundant defaults: {exc}"]

	def visit(node: ET.Element, stack: list[ET.Element]) -> None:
		name = local_name(node.tag)
		if name == "Flavor":
			value_text = child_text(node, "iFlavor")
			try:
				value = int(value_text)
			except ValueError:
				value = None
			if value == 0:
				flavor_type = child_text(node, "FlavorType") or "(unknown flavor)"
				errors.append(
					f"{display_path}: {object_key(stack)} has redundant {flavor_type} iFlavor=0; "
					"omit the Flavor entry because a missing flavor already evaluates to 0"
				)

		if name == "TechTypes" and len(node):
			children = list(node)
			if all(local_name(child.tag) == "PrereqTech" for child in children):
				values = [(child.text or "").strip() for child in children]
				if values and all(value in NO_TECH_VALUES for value in values):
					errors.append(
					f"{display_path}: {object_key(stack)} has a non-empty TechTypes list containing "
					"only NONE/NO_TECH prerequisites; use <TechTypes/> instead"
				)

		for child in node:
			visit(child, stack + [node])

	visit(root, [])
	return errors


def main() -> int:
	repo_root = get_default_repo_root()
	errors: list[str] = []
	for path in sorted((repo_root / XML_ROOT).rglob("*.xml")):
		relative = path.relative_to(repo_root)
		if any(relative == ignored or ignored in relative.parents for ignored in IGNORED_DIRS):
			continue
		errors.extend(scan_file(path, relative))

	if errors:
		print("Redundant XML default/list entries found:")
		for error in errors:
			print(f"- {error}")
		return 1

	print("PASS: no checked redundant XML default/list entries found.")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
