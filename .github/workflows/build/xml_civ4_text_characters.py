#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: characters that Civ4 GameText renders as visible artifacts.

from pathlib import Path
import argparse
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))

from xml_defines import get_default_repo_root


TEXT_ROOT = Path("Assets/XML/Text")
ENCODINGS_TO_TRY = (
	"utf-8-sig",
	"cp1252",
	"latin1",
)

# <!-- custom: Civ4 renders these Unicode typography characters as artifacts in GameText.
# Keep readable accented letters valid and use simple ASCII punctuation instead. (GPT-5.6-Sol) -->
UNSUPPORTED_CHARACTERS = {
	"\u00a0": "non-breaking space",
	"\u2010": "Unicode hyphen",
	"\u2011": "non-breaking hyphen",
	"\u2012": "figure dash",
	"\u2013": "en dash",
	"\u2014": "em dash",
	"\u2018": "left curly single quote",
	"\u2019": "right curly single quote",
	"\u201a": "single low quote",
	"\u201c": "left curly double quote",
	"\u201d": "right curly double quote",
	"\u201e": "double low quote",
	"\u2026": "ellipsis character",
}
REPEATED_QUESTION_MARK_PATTERN = re.compile(r"\?{2,}")
COMMENT_PATTERN = re.compile(r"<!--.*?-->", re.DOTALL)


def decode_xml_bytes(data: bytes) -> str:
	for encoding in ENCODINGS_TO_TRY:
		try:
			return data.decode(encoding)
		except UnicodeDecodeError:
			pass
	return data.decode("latin1", errors="replace")


def blank_comments(text: str) -> str:
	return COMMENT_PATTERN.sub(lambda match: "".join("\n" if char == "\n" else " " for char in match.group(0)), text)


def line_number(text: str, index: int) -> int:
	return text.count("\n", 0, index) + 1


def excerpt(text: str, index: int, radius: int = 90) -> str:
	line_start = text.rfind("\n", 0, index) + 1
	line_end = text.find("\n", index)
	if line_end < 0:
		line_end = len(text)
	start = max(line_start, index - radius)
	end = min(line_end, index + radius + 1)
	return (("..." if start > line_start else "") + text[start:end] + ("..." if end < line_end else "")).strip()


def check_file(relative_path: Path, text: str) -> list[str]:
	failures: list[str] = []
	active_text = blank_comments(text)

	for index, char in enumerate(active_text):
		reason = UNSUPPORTED_CHARACTERS.get(char)
		if reason is not None:
			failures.append(f"{relative_path}: line {line_number(text, index)}: unsupported Civ4 GameText character U+{ord(char):04X} ({reason}): {excerpt(text, index)}")

	for match in REPEATED_QUESTION_MARK_PATTERN.finditer(active_text):
		index = match.start()
		failures.append(f"{relative_path}: line {line_number(text, index)}: repeated question marks, likely lost characters: {excerpt(text, index)}")

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check active XML GameText for characters that Civ4 renders as artifacts.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	root = args.repo_root / TEXT_ROOT
	if not root.exists():
		print(f"FAIL Civ4 GameText characters\n  - {TEXT_ROOT}: missing directory")
		return 1

	failures: list[str] = []
	for path in sorted(root.rglob("*.xml")):
		if path.name.endswith("Schema.xml"):
			continue
		relative_path = path.relative_to(args.repo_root)
		failures.extend(check_file(relative_path, decode_xml_bytes(path.read_bytes())))

	if failures:
		print("FAIL Civ4 GameText characters")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS Civ4 GameText characters")
	return 0


if __name__ == "__main__":
	sys.exit(main())
