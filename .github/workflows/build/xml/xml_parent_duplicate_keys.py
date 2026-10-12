#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from pathlib import Path
import argparse
import sys
import xml.etree.ElementTree as ET

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root


XML_SCAN_RELATIVE_PATHS = (
	Path("Assets/XML"),
)

IGNORED_XML_RELATIVE_DIRS = {
	Path("Assets/XML/Text"),
}

# <!-- custom: Parent-style XML objects that do not use an *Info/<Type> shape.
# Child/list duplicate semantics are intentionally not checked here. (ChatGPT-5.5) -->
PARENT_KEY_FIELDS_BY_TAG = {
	"Script2DSound": "ScriptID",
	"Script3DSound": "ScriptID",
	"SoundData": "SoundID",
}


def local_name(tag: str) -> str:
	return tag.rsplit("}", 1)[-1]


def child_text_by_local_name(node: ET.Element, child_name: str) -> str | None:
	for child in list(node):
		if local_name(child.tag) == child_name:
			return (child.text or "").strip()
	return None


def parent_key_field_for_tag(tag_name: str) -> str | None:
	if tag_name.endswith("Info"):
		return "Type"
	return PARENT_KEY_FIELDS_BY_TAG.get(tag_name)


def collect_parent_key_entries(relative_path: Path, root: ET.Element) -> list[tuple[str, str, str, str]]:
	entries: list[tuple[str, str, str, str]] = []

	for node in root.iter():
		tag_name = local_name(node.tag)
		key_field = parent_key_field_for_tag(tag_name)
		if key_field is None:
			continue

		key_value = child_text_by_local_name(node, key_field)
		if not key_value:
			continue

		entries.append((tag_name, key_field, key_value, str(relative_path)))

	return entries


def check_duplicate_parent_keys(all_entries: list[tuple[str, str, str, str]]) -> list[str]:
	failures: list[str] = []
	locations_by_key: dict[tuple[str, str, str], list[str]] = {}

	for tag_name, key_field, key_value, relative_path in all_entries:
		locations_by_key.setdefault((tag_name, key_field, key_value), []).append(relative_path)

	for (tag_name, key_field, key_value), locations in sorted(locations_by_key.items()):
		if len(locations) <= 1:
			continue

		locations_text = ", ".join(sorted(set(locations)))
		failures.append(f"{locations_text}: duplicate parent key <{key_field}> {key_value} for <{tag_name}> ({len(locations)} entries)")

	return failures


def check_xml_parent_duplicate_keys(repo_root: Path) -> list[str]:
	failures: list[str] = []
	all_entries: list[tuple[str, str, str, str]] = []

	for relative_root in XML_SCAN_RELATIVE_PATHS:
		root_dir = repo_root / relative_root
		if not root_dir.exists():
			failures.append(f"{relative_root}: missing XML scan folder")
			continue

		for path in sorted(root_dir.rglob("*.xml")):
			relative_path = path.relative_to(repo_root)
			if any(relative_path == ignored_dir or ignored_dir in relative_path.parents for ignored_dir in IGNORED_XML_RELATIVE_DIRS):
				continue

			try:
				root = ET.parse(path).getroot()
			except ET.ParseError as exc:
				failures.append(f"{relative_path}: XML parse error: {exc}")
				continue

			all_entries.extend(collect_parent_key_entries(relative_path, root))

	failures.extend(check_duplicate_parent_keys(all_entries))
	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that parent-style Civ4 XML objects are not defined twice with the same key.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	failures = check_xml_parent_duplicate_keys(args.repo_root)

	if failures:
		print("FAIL XML duplicate parent keys")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS XML duplicate parent keys")
	return 0


if __name__ == "__main__":
	sys.exit(main())
