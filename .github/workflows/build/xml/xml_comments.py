#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: XML comments must not contain double hyphen.

from pathlib import Path
import argparse
import sys

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root


XML_SCAN_RELATIVE_PATHS = (
	Path("Assets/XML"),
)


def line_number(data: bytes, index: int) -> int:
	return data.count(b"\n", 0, index) + 1


def check_xml_comment_bytes(relative_path: Path, data: bytes) -> list[str]:
	failures: list[str] = []
	search_from = 0

	while True:
		start = data.find(b"<!--", search_from)
		if start < 0:
			break

		content_start = start + len(b"<!--")
		end = data.find(b"-->", content_start)
		if end < 0:
			failures.append(f"{relative_path}: line {line_number(data, start)}: unclosed XML comment")
			break

		content = data[content_start:end]
		double_hyphen = content.find(b"--")
		if double_hyphen >= 0:
			failures.append(f"{relative_path}: line {line_number(data, content_start + double_hyphen)}: XML comment contains illegal double hyphen `--`")

		search_from = end + len(b"-->")

	return failures


def check_xml_comments(repo_root: Path) -> list[str]:
	failures: list[str] = []

	for relative_root in XML_SCAN_RELATIVE_PATHS:
		root = repo_root / relative_root
		if not root.exists():
			failures.append(f"{relative_root}: missing XML scan folder")
			continue

		for path in sorted(root.rglob("*.xml")):
			relative_path = path.relative_to(repo_root)
			failures.extend(check_xml_comment_bytes(relative_path, path.read_bytes()))

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that AdvCiv-SAS XML comments do not contain illegal double hyphen.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	failures = check_xml_comments(args.repo_root)

	if failures:
		print("FAIL XML comment safety")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS XML comment safety")
	return 0


if __name__ == "__main__":
	sys.exit(main())
