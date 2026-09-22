#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: GlobalDefines integer values should stay within a sane range.

from pathlib import Path
import argparse
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))

from xml_defines import get_default_repo_root, read_global_define_ints


MIN_ALLOWED_VALUE = -100000
MAX_ALLOWED_VALUE = 100000

# The launch-guard sentinel is intentionally a distinctive out-of-band value so missing/wrong
# GlobalDefines are obvious; it is checked separately by launch_guard.py.
ALLOWED_OUT_OF_RANGE_DEFINES = {
	"SAS_LAUNCH_GUARD_TEST_INT",
}


def check_define_int_bounds(repo_root: Path) -> list[str]:
	failures: list[str] = []
	int_defines = read_global_define_ints(repo_root)

	for name, value in sorted(int_defines.items()):
		if name in ALLOWED_OUT_OF_RANGE_DEFINES:
			continue

		if value < MIN_ALLOWED_VALUE or value > MAX_ALLOWED_VALUE:
			failures.append(f"GlobalDefines_advciv_sas.xml: {name} = {value} is outside [{MIN_ALLOWED_VALUE}, {MAX_ALLOWED_VALUE}]")

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that AdvCiv-SAS GlobalDefines integer values stay within a sane range.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	failures = check_define_int_bounds(args.repo_root)

	if failures:
		print("FAIL GlobalDefines integer bounds")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print(f"PASS GlobalDefines integer bounds [{MIN_ALLOWED_VALUE}, {MAX_ALLOWED_VALUE}]")
	return 0


if __name__ == "__main__":
	sys.exit(main())
