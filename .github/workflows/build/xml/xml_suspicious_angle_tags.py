#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: suspicious raw XML tag punctuation.
#
# <!-- custom: XML parsers accept some malformed-looking tag text as normal text. For example,
# <French>>text</French> is parsed as a valid <French> element whose text starts
# with an extra `>`. This raw-text check catches those high-confidence typos (ChatGPT-5.5) -->

from pathlib import Path
import argparse
import re
import sys

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root


XML_SCAN_RELATIVE_PATHS = (
	Path("Assets/XML"),
)

# These patterns intentionally target tag-like punctuation only, not ordinary text.
# Comments and CDATA are blanked before scanning so documentation examples do not fail.
SUSPICIOUS_PATTERNS = (
	(
		"double opening angle before tag",
		re.compile(r"<<\s*/?\s*[A-Za-z_][A-Za-z0-9_.:-]*(?:\s|/?>|$)[^\n<]*>?"),
	),
	(
		"extra closing angle after tag",
		re.compile(r"<(?![!?])/?[A-Za-z_][A-Za-z0-9_.:-]*(?:\s+[^<>\n]*?)?/?>" + r">"),
	),
)

ENCODINGS_TO_TRY = (
	"utf-8-sig",
	"cp1252",
	"latin1",
)

BLANKED_SPANS = (
	("<!--", "-->"),
	("<![CDATA[", "]]>"),
)


def decode_xml_bytes(data: bytes) -> str:
	for encoding in ENCODINGS_TO_TRY:
		try:
			return data.decode(encoding)
		except UnicodeDecodeError:
			pass
	return data.decode("latin1", errors="replace")


def line_number(text: str, index: int) -> int:
	return text.count("\n", 0, index) + 1


def excerpt_for_line(text: str, index: int) -> str:
	line_start = text.rfind("\n", 0, index) + 1
	line_end = text.find("\n", index)
	if line_end < 0:
		line_end = len(text)
	return text[line_start:line_end].strip()


def blank_preserving_lines(text: str, start: int, end: int) -> str:
	return "".join("\n" if char == "\n" else " " for char in text[start:end])


def blank_ignored_spans(text: str) -> str:
	result = text
	for start_token, end_token in BLANKED_SPANS:
		search_from = 0
		parts: list[str] = []
		while True:
			start = result.find(start_token, search_from)
			if start < 0:
				parts.append(result[search_from:])
				break
			end = result.find(end_token, start + len(start_token))
			if end < 0:
				# xml_comments.py reports unclosed comments. Do not duplicate that error here.
				end = len(result)
			else:
				end += len(end_token)
			parts.append(result[search_from:start])
			parts.append(blank_preserving_lines(result, start, end))
			search_from = end
		result = "".join(parts)
	return result


def check_xml_text(relative_path: Path, text: str) -> list[str]:
	failures: list[str] = []
	seen_positions: set[int] = set()
	scanned_text = blank_ignored_spans(text)

	for reason, pattern in SUSPICIOUS_PATTERNS:
		for match in pattern.finditer(scanned_text):
			start = match.start()
			if start in seen_positions:
				continue
			seen_positions.add(start)
			failures.append(
				f"{relative_path}: line {line_number(text, start)}: {reason}: {excerpt_for_line(text, start)}"
			)

	return failures


def check_xml_files(repo_root: Path) -> list[str]:
	failures: list[str] = []

	for relative_root in XML_SCAN_RELATIVE_PATHS:
		root = repo_root / relative_root
		if not root.exists():
			failures.append(f"{relative_root}: missing XML scan folder")
			continue

		for path in sorted(root.rglob("*.xml")):
			relative_path = path.relative_to(repo_root)
			text = decode_xml_bytes(path.read_bytes())
			failures.extend(check_xml_text(relative_path, text))

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check for suspicious malformed-looking XML tag punctuation.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	failures = check_xml_files(args.repo_root)

	if failures:
		print("FAIL XML suspicious angle tags")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS XML suspicious angle tags")
	return 0


if __name__ == "__main__":
	sys.exit(main())
