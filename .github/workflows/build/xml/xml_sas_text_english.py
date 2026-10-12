#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: AdvCiv-SAS GameText XML should only contain English language text tags.

from pathlib import Path
import argparse
import sys
import xml.etree.ElementTree as ET

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root


TEXT_XML_RELATIVE_PATH = Path("Assets/XML/Text")
ADVCIV_SAS_TEXT_FILE_GLOB = "AdvCiv-SAS*.xml"
ALLOWED_LANGUAGE_TAGS = {
	"English",
}

# <!-- custom: Rare inherited/renamed diplomacy text entries kept with old non-English language fields.
# Keep this list narrow: new AdvCiv-SAS text should normally use <English> only. (ChatGPT-5.5) -->
ALLOWED_NON_ENGLISH_TEXT_KEYS = {
	"AI_DIPLO_FIRST_CONTACT_LEADER_MOCTEZUMA_1",
	"AI_DIPLO_REFUSE_TO_TALK_LEADER_MOCTEZUMA_1",
	"AI_DIPLO_GREETINGS_ATT_FUR_LEADER_MOCTEZUMA_1",
	"AI_DIPLO_DEMAND_TRIBUTE_POWER_STRONGER_MOCTEZUMA_1_CORRECTED",
}

NON_LANGUAGE_TEXT_CHILD_TAGS = {
	"Tag",
}


def local_name(tag: str) -> str:
	return tag.rsplit("}", 1)[-1]


def line_number(data: bytes, index: int) -> int:
	return data.count(b"\n", 0, index) + 1


def element_start_line(data: bytes, tag_name: str, search_from: int) -> int:
	token = ("<" + tag_name).encode("utf-8", errors="replace")
	index = data.find(token, max(0, search_from))
	if index < 0:
		return 1
	return line_number(data, index)


def child_text_by_local_name(node: ET.Element, child_name: str) -> str | None:
	for child in list(node):
		if local_name(child.tag) == child_name:
			return (child.text or "").strip()
	return None


def check_text_xml_file(repo_root: Path, path: Path) -> list[str]:
	failures: list[str] = []
	data = path.read_bytes()
	relative_path = path.relative_to(repo_root)

	try:
		root = ET.fromstring(data)
	except ET.ParseError as exc:
		return [f"{relative_path}: XML parse error: {exc}"]

	search_from = 0
	for text_node in root.iter():
		if local_name(text_node.tag) != "TEXT":
			continue

		text_key = child_text_by_local_name(text_node, "Tag") or "<missing Tag>"
		text_start = data.find(b"<TEXT", search_from)
		if text_start >= 0:
			search_from = text_start + len(b"<TEXT")
		else:
			text_start = 0

		language_counts: dict[str, int] = {}

		for child in list(text_node):
			child_name = local_name(child.tag)
			if child_name in NON_LANGUAGE_TEXT_CHILD_TAGS:
				continue

			if child_name not in ALLOWED_LANGUAGE_TAGS:
				if text_key not in ALLOWED_NON_ENGLISH_TEXT_KEYS:
					failures.append(f"{relative_path}: line {element_start_line(data, child_name, text_start)}: TEXT entry {text_key} has unsupported language tag <{child_name}>")
				continue

			language_counts[child_name] = language_counts.get(child_name, 0) + 1

		english_count = language_counts.get("English", 0)
		if english_count == 0:
			failures.append(f"{relative_path}: line {line_number(data, text_start)}: TEXT entry {text_key} has no <English> tag")
		elif english_count > 1:
			failures.append(f"{relative_path}: line {line_number(data, text_start)}: TEXT entry {text_key} has {english_count} <English> tags")

	return failures


def check_text_xml_english_only(repo_root: Path) -> list[str]:
	failures: list[str] = []
	text_dir = repo_root / TEXT_XML_RELATIVE_PATH

	if not text_dir.exists():
		failures.append(f"{TEXT_XML_RELATIVE_PATH}: missing text XML folder")
		return failures

	files = sorted(text_dir.glob(ADVCIV_SAS_TEXT_FILE_GLOB))
	if not files:
		failures.append(f"{TEXT_XML_RELATIVE_PATH}: no {ADVCIV_SAS_TEXT_FILE_GLOB} files found")
		return failures

	for path in files:
		failures.extend(check_text_xml_file(repo_root, path))

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that AdvCiv-SAS GameText XML only uses English language tags.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	failures = check_text_xml_english_only(args.repo_root)

	if failures:
		print("FAIL AdvCiv-SAS GameText English-only tags")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS AdvCiv-SAS GameText English-only tags")
	return 0


if __name__ == "__main__":
	sys.exit(main())
