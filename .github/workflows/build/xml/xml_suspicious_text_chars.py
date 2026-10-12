#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: suspicious/corrupted text characters in XML.
#
# <!-- custom: Keep this check focused on high-confidence corruption, not broad
# typography policy. For example, K?szeghy is probably a lost accented letter,
# while a sentence-ending question mark is normal. (ChatGPT-5.5) -->

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

ENCODINGS_TO_TRY = (
	"utf-8-sig",
	"cp1252",
	"latin1",
)

COMMENT_SPANS = (
	("<!--", "-->"),
)

# Anything surrounded by these characters is not considered to be inside one
# word-like token. Keep this broad enough for normal sentence punctuation and
# Civ4 placeholders, but not so broad that corrupted names such as K?szeghy are missed.
QUESTION_MARK_TOKEN_DELIMITERS = set(
	" \t\r\n<>/\\\"'`()[]{}.,;:!?&|=+*%#-"
)

# These inherited language tags contain many old non-English replacement chars in
# base/BUG text. They are hidden by default so this check stays useful for active
# English/SAS text, but can still be printed with --show-ignored.
IGNORED_REPLACEMENT_CHAR_LANGUAGE_TAGS = {
	"French",
	"German",
	"Italian",
	"Spanish",
}

MOJIBAKE_PATTERNS = (
	(
		"common UTF-8 mojibake fragment",
		re.compile(r"(?:Ã[^\s<]|Â[^\s<]|â[€„…†‡ˆ‰Š‹ŒŽ‘’“”•–—˜™š›œžŸ])"),
	),
	(
		"common emoji/symbol mojibake fragment",
		re.compile(r"(?:ðŸ[^\s<]|Ð[^\s<]|Ñ[^\s<])"),
	),
)

CONTROL_CHAR_PATTERN = re.compile(r"[\x00-\x08\x0b\x0c\x0e-\x1f\x7f-\x9f]")
TRACKING_QUERY_PATTERN = re.compile(r"[?&]utm_(?:source|medium|campaign|term|content)=", re.IGNORECASE)
LANGUAGE_LINE_PATTERN = re.compile(r"^\s*<([A-Za-z]+)(?:\s|>)")


def decode_xml_bytes(data: bytes) -> str:
	for encoding in ENCODINGS_TO_TRY:
		try:
			return data.decode(encoding)
		except UnicodeDecodeError:
			pass
	return data.decode("latin1", errors="replace")


def line_bounds(text: str, index: int) -> tuple[int, int]:
	line_start = text.rfind("\n", 0, index) + 1
	line_end = text.find("\n", index)
	if line_end < 0:
		line_end = len(text)
	return line_start, line_end


def line_number(text: str, index: int) -> int:
	return text.count("\n", 0, index) + 1


def excerpt_around_index(text: str, index: int, radius: int = 90) -> str:
	line_start, line_end = line_bounds(text, index)
	start = max(line_start, index - radius)
	end = min(line_end, index + radius + 1)
	prefix = "..." if start > line_start else ""
	suffix = "..." if end < line_end else ""
	return (prefix + text[start:end] + suffix).strip()


def language_tag_for_line(text: str, index: int) -> str | None:
	line_start, line_end = line_bounds(text, index)
	line = text[line_start:line_end]
	match = LANGUAGE_LINE_PATTERN.match(line)
	if match is None:
		return None
	return match.group(1)


def blank_preserving_lines(text: str, start: int, end: int) -> str:
	return "".join("\n" if char == "\n" else " " for char in text[start:end])


def blank_comments(text: str) -> str:
	result = text
	for start_token, end_token in COMMENT_SPANS:
		search_from = 0
		parts: list[str] = []
		while True:
			start = result.find(start_token, search_from)
			if start < 0:
				parts.append(result[search_from:])
				break
			end = result.find(end_token, start + len(start_token))
			if end < 0:
				end = len(result)
			else:
				end += len(end_token)
			parts.append(result[search_from:start])
			parts.append(blank_preserving_lines(result, start, end))
			search_from = end
		result = "".join(parts)
	return result


def is_delimiter(char: str) -> bool:
	return not char or char.isspace() or char in QUESTION_MARK_TOKEN_DELIMITERS


def is_suspicious_question_mark(text: str, index: int) -> bool:
	previous_char = text[index - 1] if index > 0 else ""
	next_char = text[index + 1] if index + 1 < len(text) else ""

	return not is_delimiter(previous_char) and not is_delimiter(next_char)


def should_scan_file(path: Path) -> bool:
	if path.name.endswith("Schema.xml"):
		return False
	return path.suffix.lower() == ".xml"


def finding(relative_path: Path, text: str, index: int, reason: str) -> str:
	return f"{relative_path}: line {line_number(text, index)}: {reason}: {excerpt_around_index(text, index)}"


def check_xml_text(relative_path: Path, text: str) -> tuple[list[str], list[str]]:
	failures: list[str] = []
	ignored: list[str] = []
	seen_positions: set[tuple[int, str]] = set()
	scanned_text = blank_comments(text)

	for index, char in enumerate(scanned_text):
		if char != "?":
			continue
		if not is_suspicious_question_mark(scanned_text, index):
			continue
		key = (index, "suspicious question mark inside token")
		if key in seen_positions:
			continue
		seen_positions.add(key)
		failures.append(finding(relative_path, text, index, "suspicious question mark inside token"))

	for index, char in enumerate(scanned_text):
		if char != "�":
			continue
		key = (index, "Unicode replacement character")
		if key in seen_positions:
			continue
		seen_positions.add(key)
		message = finding(relative_path, text, index, "Unicode replacement character")
		if language_tag_for_line(text, index) in IGNORED_REPLACEMENT_CHAR_LANGUAGE_TAGS:
			ignored.append(message)
		else:
			failures.append(message)

	for reason, pattern in MOJIBAKE_PATTERNS:
		for match in pattern.finditer(scanned_text):
			start = match.start()
			key = (start, reason)
			if key in seen_positions:
				continue
			seen_positions.add(key)
			failures.append(finding(relative_path, text, start, reason))

	for match in CONTROL_CHAR_PATTERN.finditer(scanned_text):
		start = match.start()
		key = (start, "raw control character")
		if key in seen_positions:
			continue
		seen_positions.add(key)
		codepoint = ord(scanned_text[start])
		failures.append(finding(relative_path, text, start, f"raw control character U+{codepoint:04X}"))

	for match in TRACKING_QUERY_PATTERN.finditer(scanned_text):
		start = match.start()
		failures.append(finding(relative_path, text, start, "web tracking query parameter in active XML text"))

	return failures, ignored


def check_xml_files(repo_root: Path) -> tuple[list[str], list[str]]:
	failures: list[str] = []
	ignored: list[str] = []

	for relative_root in XML_SCAN_RELATIVE_PATHS:
		root = repo_root / relative_root
		if not root.exists():
			failures.append(f"{relative_root}: missing directory")
			continue

		for path in sorted(root.rglob("*.xml")):
			if not should_scan_file(path):
				continue

			relative_path = path.relative_to(repo_root)
			text = decode_xml_bytes(path.read_bytes())
			file_failures, file_ignored = check_xml_text(relative_path, text)
			failures.extend(file_failures)
			ignored.extend(file_ignored)

	return failures, ignored


def main() -> int:
	parser = argparse.ArgumentParser(description="Check XML text for high-confidence corrupted characters.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	parser.add_argument("--show-ignored", action="store_true", help="also print known inherited non-English replacement-character findings")
	args = parser.parse_args()

	failures, ignored = check_xml_files(args.repo_root)
	if failures:
		print("FAIL XML suspicious text chars")
		for failure in failures:
			print(f"  - {failure}")
		if ignored and args.show_ignored:
			print("Ignored known inherited non-English replacement-character findings")
			for item in ignored:
				print(f"  - {item}")
		elif ignored:
			print(f"Ignored {len(ignored)} known inherited non-English replacement-character finding(s); rerun with --show-ignored to list them.")
		return 1

	if ignored and args.show_ignored:
		print("Ignored known inherited non-English replacement-character findings")
		for item in ignored:
			print(f"  - {item}")

	print("PASS XML suspicious text chars")
	if ignored and not args.show_ignored:
		print(f"Ignored {len(ignored)} known inherited non-English replacement-character finding(s); rerun with --show-ignored to list them.")
	return 0


if __name__ == "__main__":
	sys.exit(main())
