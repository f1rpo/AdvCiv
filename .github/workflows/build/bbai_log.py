#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: BBAI diagnostic logging must be disabled by default.

from pathlib import Path
import argparse
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))

from xml_defines import get_default_repo_root, read_global_define_ints, require_int_values


EXPECTED_NONZERO_LOGGING_DEFAULTS = {
	# These configure enabled logging but do not enable it themselves.
	"SAS_BBAI_SCORE_LOG_INTERVAL_TURNS_UNSCALED_GAMESPEED": 100,
	"SAS_BBAI_LOG_USE_TIMESTAMPED_FILENAME": 1,
}


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that BBAI diagnostic logging is disabled by default unless explicitly listed as non-enabling configuration.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	defines = read_global_define_ints(args.repo_root)
	# <!-- custom: The old manual list missed Citizen and Culture logging.
	# Require every integer SAS_BBAI_* define to be 0 automatically, with explicit exceptions only for settings that configure enabled logging without enabling it. (GPT-5.5 + GPT-5.5) -->
	expected = {name: 0 for name in defines if name.startswith("SAS_BBAI_")}
	expected.update(EXPECTED_NONZERO_LOGGING_DEFAULTS)
	expected["SAS_BBAI_LOG_ENABLE"] = 0
	failures = require_int_values(defines, expected)
	if failures:
		print("FAIL BBAI diagnostic logging disabled by default")
		for failure in failures:
			print(f"  - {failure}")
		return 1
	print(f"PASS BBAI diagnostic logging disabled by default: checked {len(expected)} define(s)")
	return 0


if __name__ == "__main__":
	sys.exit(main())
