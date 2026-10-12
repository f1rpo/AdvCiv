#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: SAS DefineTextVal values that look like Civ4 XML Type tags must reference real XML <Type> values.

from pathlib import Path
import argparse
import re
import sys
import xml.etree.ElementTree as ET

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root, read_global_define_texts


XML_RELATIVE_PATH = Path("Assets/XML")
IGNORED_XML_RELATIVE_DIRS = {
	Path("Assets/XML/Text"),
}

XML_TYPE_TAG_PREFIXES = (
	"BONUS_",
	"BUILDING_",
	"BUILDINGCLASS_",
	"CIVIC_",
	"CIVILIZATION_",
	"CORPORATION_",
	"ERA_",
	"FEATURE_",
	"HANDICAP_",
	"IMPROVEMENT_",
	"LEADER_",
	"PROJECT_",
	"PROMOTION_",
	"RELIGION_",
	"ROUTE_",
	"SPECIALIST_",
	"TECH_",
	"TERRAIN_",
	"TRAIT_",
	"UNIT_",
	"UNITCLASS_",
	"VICTORY_",
	"VOTE_",
	"VOTESOURCE_",
	"WORLDSIZE_",
)

XML_TYPE_TOKEN_RE = re.compile(r"\b[A-Z][A-Z0-9_]*_[A-Z0-9_]+\b")


def local_name(tag: str) -> str:
	return tag.rsplit("}", 1)[-1]


def read_all_xml_type_values(repo_root: Path) -> set[str]:
	xml_dir = repo_root / XML_RELATIVE_PATH
	if not xml_dir.exists():
		raise RuntimeError(f"missing folder: {xml_dir}")

	type_values: set[str] = set()

	for path in sorted(xml_dir.rglob("*.xml")):
		relative_path = path.relative_to(repo_root)
		if any(relative_path == ignored_dir or ignored_dir in relative_path.parents for ignored_dir in IGNORED_XML_RELATIVE_DIRS):
			continue

		try:
			tree = ET.parse(path)
		except ET.ParseError as exc:
			raise RuntimeError(f"{path}: XML parse error: {exc}") from exc

		for node in tree.getroot().iter():
			if local_name(node.tag) != "Type":
				continue

			value = (node.text or "").strip()
			if value:
				type_values.add(value)

	return type_values


def find_xml_type_like_tokens(value: str) -> list[str]:
	tokens: list[str] = []

	for match in XML_TYPE_TOKEN_RE.finditer(value):
		token = match.group(0)
		if token.startswith(XML_TYPE_TAG_PREFIXES) and token not in tokens:
			tokens.append(token)

	return tokens


def check_define_tag_refs(repo_root: Path, quiet: bool) -> list[str]:
	failures: list[str] = []
	xml_type_values = read_all_xml_type_values(repo_root)
	define_texts = read_global_define_texts(repo_root)

	for define_name in sorted(define_texts):
		tokens = find_xml_type_like_tokens(define_texts[define_name])
		if not tokens:
			continue

		for token in tokens:
			if token not in xml_type_values:
				failures.append(f"GlobalDefines_advciv_sas.xml: {define_name} references missing XML Type {token}")
				continue

			if not quiet:
				print(f"OK {define_name}: {token}")

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that AdvCiv-SAS DefineTextVal XML tag references exist.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	parser.add_argument("--quiet", action="store_true", help="only print PASS/FAIL and failures, not every successful define -> XML tag match")
	args = parser.parse_args()

	failures = check_define_tag_refs(args.repo_root, args.quiet)

	if failures:
		print("FAIL SAS define XML tag references")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS SAS define XML tag references")
	return 0


if __name__ == "__main__":
	sys.exit(main())
