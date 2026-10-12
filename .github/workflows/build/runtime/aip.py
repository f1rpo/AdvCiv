#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: AI Personality Panel cache defaults must stay release-safe.

from pathlib import Path
import argparse
import sys

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root, read_global_define_ints, require_int_values


EXPECTED_AI_PERSONALITY_DEFINES = {
	"SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_ENABLE": 1,
	"SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_CACHE_USE_PREDUMPED": 1,
	"SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_CACHE_DUMP_TO_LOG": 0,
}

PREDUMP_RELATIVE_PATH = Path("Assets/Python/Contrib/Sevopedia/SevoPediaLeaderCachePredumped.py")
PREDUMP_MIN_SIZE_BYTES = 10000


def check_predump_file(repo_root: Path) -> list[str]:
	path = repo_root / PREDUMP_RELATIVE_PATH
	if not path.exists():
		return [f"{PREDUMP_RELATIVE_PATH}: missing predumped AI personality cache file"]

	if not path.is_file():
		return [f"{PREDUMP_RELATIVE_PATH}: exists but is not a file"]

	size = path.stat().st_size
	if size < PREDUMP_MIN_SIZE_BYTES:
		return [f"{PREDUMP_RELATIVE_PATH}: suspiciously small predump file ({size} bytes, expected at least {PREDUMP_MIN_SIZE_BYTES})"]

	return []


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that AdvCiv-SAS AI Personality Panel cache defaults are release-safe.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	defines = read_global_define_ints(args.repo_root)
	failures = require_int_values(defines, EXPECTED_AI_PERSONALITY_DEFINES)
	failures.extend(check_predump_file(args.repo_root))

	if failures:
		print("FAIL AI Personality Panel cache defaults")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS AI Personality Panel cache defaults")
	return 0


if __name__ == "__main__":
	sys.exit(main())
