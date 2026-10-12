#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: turn-valued SAS integer defines must state whether their value is
# expressed at Normal game speed and scaled by code, or intentionally unscaled.

from pathlib import Path
import argparse
import re
import sys

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root, read_global_define_ints


NORMAL_PATTERN = re.compile(r"_(?:TURN|TURNS)_NORMAL_GAMESPEED(?:_|$)")
UNSCALED_PATTERN = re.compile(r"_(?:TURN|TURNS)_UNSCALED_GAMESPEED(?:_|$)")


def has_temporal_turn_token(name: str) -> bool:
	"""Return True for standalone turn counts/thresholds, not scopes or per-turn rates."""
	tokens = name.split("_")
	for index, token in enumerate(tokens):
		if token not in {"TURN", "TURNS"}:
			continue

		# SAS_DO_TURN_* names identify the function/scope, not a turn-valued define.
		if token == "TURN" and index > 0 and tokens[index - 1] == "DO":
			continue

		# Values expressed per turn are rates/weights rather than a number of turns.
		if token == "TURN" and index > 0 and tokens[index - 1] == "PER":
			continue
		if token == "TURN" and index > 1 and tokens[index - 2:index] == ["PER", "ANARCHY"]:
			continue

		# e.g. PATH_TURN_VALUE_PENALTY: TURN describes the unit of a score penalty.
		if token == "TURN" and index + 1 < len(tokens) and tokens[index + 1] == "VALUE":
			continue

		return True
	return False


def looks_like_legacy_turn_quantity(name: str) -> bool:
	# These older names encode turn counts without a TURN(S) unit token. Keep them
	# covered so a future rename cannot silently drop the game-speed classification.
	return ("_MAX_COUNTDOWN_" in name or name.endswith("_MIN_AGE") or name.endswith("_MAX_AGE") or name.endswith("_LOG_INTERVAL"))


def check_turn_define_gamespeed(repo_root: Path) -> tuple[list[str], int, int]:
	failures: list[str] = []
	normal_count = 0
	unscaled_count = 0

	for name in sorted(read_global_define_ints(repo_root)):
		if not (has_temporal_turn_token(name) or looks_like_legacy_turn_quantity(name)):
			continue

		has_normal = NORMAL_PATTERN.search(name) is not None
		has_unscaled = UNSCALED_PATTERN.search(name) is not None
		if has_normal and has_unscaled:
			failures.append(f"{name}: contains both NORMAL_GAMESPEED and UNSCALED_GAMESPEED turn classifications")
		elif has_normal:
			normal_count += 1
		elif has_unscaled:
			unscaled_count += 1
		else:
			failures.append(f"{name}: turn-valued define must contain TURN(S)_NORMAL_GAMESPEED or TURN(S)_UNSCALED_GAMESPEED")

	return failures, normal_count, unscaled_count


def main() -> int:
	parser = argparse.ArgumentParser(description="Check explicit game-speed semantics in turn-valued AdvCiv-SAS integer define names.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	failures, normal_count, unscaled_count = check_turn_define_gamespeed(args.repo_root)
	if failures:
		print("FAIL SAS turn-define game-speed naming")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print(f"PASS SAS turn-define game-speed naming (normal={normal_count}, unscaled={unscaled_count})")
	return 0


if __name__ == "__main__":
	sys.exit(main())
