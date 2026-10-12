#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: local DDS button/image paths must not contain whitespace outside NIF folders.

from pathlib import Path
import argparse
import sys

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root


ART_RELATIVE_PATH = Path("Assets/Art")
BUTTON_IMAGE_SUFFIXES = {".dds"}
EXCLUDED_DIRECTORY_NAMES = {"nif"}


def is_excluded_model_path(relative_path: Path) -> bool:
	return any(part.lower() in EXCLUDED_DIRECTORY_NAMES for part in relative_path.parts[:-1])


def has_path_whitespace(relative_path: Path) -> bool:
	return any(any(character.isspace() for character in part) for part in relative_path.parts)


def check_art_button_paths(repo_root: Path) -> list[str]:
	failures: list[str] = []
	art_root = repo_root / ART_RELATIVE_PATH
	if not art_root.exists():
		return [f"{ART_RELATIVE_PATH}: missing Art folder"]

	for path in sorted(art_root.rglob("*")):
		if not path.is_file() or path.suffix.lower() not in BUTTON_IMAGE_SUFFIXES:
			continue

		relative_art_path = path.relative_to(art_root)
		if is_excluded_model_path(relative_art_path):
			continue
		if has_path_whitespace(relative_art_path):
			failures.append(f"{path.relative_to(repo_root)}: local DDS button/image path contains whitespace and can fail in Civ4 <img> markup")

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check local AdvCiv-SAS DDS button/image paths for whitespace outside NIF folders.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	failures = check_art_button_paths(args.repo_root)
	if failures:
		print("FAIL Art button/image paths")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS Art button/image paths")
	return 0


if __name__ == "__main__":
	sys.exit(main())
