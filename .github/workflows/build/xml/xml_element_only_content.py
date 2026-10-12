#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: schema-style XML elements must not contain stray text between child elements.

from pathlib import Path
import argparse
import sys
import xml.etree.ElementTree as ET

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root


XML_SCAN_RELATIVE_PATH = Path("Assets/XML")
XML_TEXT_FOLDER = "Text"


def local_name(tag: str) -> str:
	return tag.rsplit("}", 1)[-1]


def excerpt(text: str) -> str:
	value = " ".join(text.split())
	return value if len(value) <= 100 else value[:97] + "..."


def check_element_content(relative_path: Path, root: ET.Element) -> list[str]:
	failures: list[str] = []

	for element in root.iter():
		children = list(element)
		if children and element.text and element.text.strip():
			failures.append(f"{relative_path}: unexpected text inside <{local_name(element.tag)}> before its first child: `{excerpt(element.text)}`")

		for child in children:
			if child.tail and child.tail.strip():
				failures.append(f"{relative_path}: unexpected text after <{local_name(child.tag)}> inside <{local_name(element.tag)}>: `{excerpt(child.tail)}`")

	return failures


def check_xml_element_only_content(repo_root: Path) -> list[str]:
	failures: list[str] = []
	root_path = repo_root / XML_SCAN_RELATIVE_PATH
	if not root_path.exists():
		return [f"{XML_SCAN_RELATIVE_PATH}: missing XML scan folder"]

	for path in sorted(root_path.rglob("*.xml")):
		relative_path = path.relative_to(repo_root)
		if XML_TEXT_FOLDER in relative_path.parts:
			continue

		try:
			root = ET.parse(path).getroot()
		except ET.ParseError as error:
			failures.append(f"{relative_path}: XML parse error: {error}")
			continue

		failures.extend(check_element_content(relative_path, root))

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check schema-style AdvCiv-SAS XML for stray text between child elements.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	failures = check_xml_element_only_content(args.repo_root)
	if failures:
		print("FAIL XML element-only content")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS XML element-only content")
	return 0


if __name__ == "__main__":
	sys.exit(main())
