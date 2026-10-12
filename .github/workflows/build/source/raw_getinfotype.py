#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: literal/static XML tag lookups in runtime Python should fail loudly through
# getInfoTypeOrFail(...) or findInfoTypeNumOrFail(...), not silently return -1.

from pathlib import Path
import argparse
import re
import sys
import tokenize

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root


RAW_GETINFOTYPE_NAME = "getInfoTypeForString"
RAW_FINDINFOTYPE_NAME = "findInfoTypeNum"
RAW_LOOKUP_NAMES = {RAW_GETINFOTYPE_NAME, RAW_FINDINFOTYPE_NAME}

SCAN_RELATIVE_DIRS = (
	Path("Assets/Python"),
	Path("PrivateMaps"),
)

# These are repository/dev helper files, not Civ4 runtime gameplay files. They are not under
# SCAN_RELATIVE_DIRS today, but keeping this guard makes the intent explicit if the scan grows.
IGNORED_RELATIVE_DIRS = {
	Path("LLM_Helpers"),
}

# Narrow runtime file exceptions where raw lookup is dynamic/legacy by design.
# Do not add broad gameplay files here. Prefer converting literal/static callsites to the strict helpers.
ALLOWED_RELATIVE_PATH_REASONS = {
	Path("Assets/Python/BUG/AttitudeUtil.py"): {
		RAW_GETINFOTYPE_NAME: "BUG config reads dynamic color tags and raises ConfigError on missing tags",
	},
	Path("Assets/Python/BUG/TechPrefs.py"): {
		RAW_GETINFOTYPE_NAME: "BUG config reads dynamic tech preference tags",
	},
	Path("Assets/Python/pyWB/CvWBDesc.py"): {
		RAW_GETINFOTYPE_NAME: "WorldBuilder save/load reads dynamic scenario strings",
		RAW_FINDINFOTYPE_NAME: "WorldBuilder save/load reads dynamic scenario strings",
	},
}

# Function-scope exceptions. These helpers are allowed to call the raw API/wrapper internally;
# the rest of the file is still checked normally.
ALLOWED_FUNCTION_REASONS = {
	(Path("Assets/Python/SASUtils.py"), "getInfoTypeOrFail", RAW_GETINFOTYPE_NAME): "strict helper implementation",
	(Path("Assets/Python/CvUtil.py"), "findInfoTypeNum", RAW_GETINFOTYPE_NAME): "legacy wrapper implementation",
	(Path("Assets/Python/SASUtils.py"), "findInfoTypeNumOrFail", RAW_FINDINFOTYPE_NAME): "strict helper implementation",
}

# Contrib/savemap.py writes Python code text into generated map-script files. Token scanning ignores
# Python STRING tokens, so those stringized getInfoTypeForString snippets are intentionally not flagged.

TOP_LEVEL_DEF_RE = re.compile(r"^def\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(")
TOP_LEVEL_BOUNDARY_RE = re.compile(r"^(def|class)\s+")


class RawLookupHit:
	def __init__(self, lookup_name: str, relative_path: Path, line_number: int, line_text: str):
		self.lookup_name = lookup_name
		self.relative_path = relative_path
		self.line_number = line_number
		self.line_text = line_text.strip()

	def format(self) -> str:
		return f"{self.relative_path}:{self.line_number}: {self.line_text}"


def is_under(relative_path: Path, relative_dir: Path) -> bool:
	return relative_path == relative_dir or relative_dir in relative_path.parents


def iter_python_files(repo_root: Path) -> list[Path]:
	paths: list[Path] = []

	for relative_dir in SCAN_RELATIVE_DIRS:
		root = repo_root / relative_dir
		if not root.exists():
			continue

		for path in sorted(root.rglob("*.py")):
			relative_path = path.relative_to(repo_root)
			if any(is_under(relative_path, ignored_dir) for ignored_dir in IGNORED_RELATIVE_DIRS):
				continue
			paths.append(path)

	return paths


def get_top_level_function_ranges(path: Path) -> dict[str, tuple[int, int]]:
	lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
	starts: list[tuple[str, int]] = []

	for index, line in enumerate(lines, start=1):
		match = TOP_LEVEL_DEF_RE.match(line)
		if match:
			starts.append((match.group(1), index))

	ranges: dict[str, tuple[int, int]] = {}
	for function_index, (function_name, start_line) in enumerate(starts):
		end_line = len(lines) + 1
		for index in range(start_line + 1, len(lines) + 1):
			line = lines[index - 1]
			if TOP_LEVEL_BOUNDARY_RE.match(line):
				end_line = index
				break
		ranges[function_name] = (start_line, end_line)

	return ranges


def is_in_allowed_function(relative_path: Path, function_ranges: dict[str, tuple[int, int]], hit: RawLookupHit) -> str | None:
	for (allowed_path, function_name, lookup_name), reason in ALLOWED_FUNCTION_REASONS.items():
		if relative_path != allowed_path or hit.lookup_name != lookup_name:
			continue
		range_tuple = function_ranges.get(function_name)
		if not range_tuple:
			continue
		start_line, end_line = range_tuple
		if start_line <= hit.line_number < end_line:
			return reason
	return None


def find_raw_lookup_hits(repo_root: Path, path: Path) -> list[RawLookupHit]:
	hits: list[RawLookupHit] = []
	relative_path = path.relative_to(repo_root)
	previous_significant_token = None

	try:
		with path.open("rb") as handle:
			for token in tokenize.tokenize(handle.readline):
				if token.type in {tokenize.ENCODING, tokenize.NL, tokenize.NEWLINE, tokenize.INDENT, tokenize.DEDENT}:
					continue
				if token.type == tokenize.NAME and token.string in RAW_LOOKUP_NAMES:
					# Do not count the helper/wrapper definition itself, only raw lookup uses.
					if not (previous_significant_token and previous_significant_token.type == tokenize.NAME and previous_significant_token.string == "def"):
						hits.append(RawLookupHit(token.string, relative_path, token.start[0], token.line))
				previous_significant_token = token
	except tokenize.TokenError as exc:
		raise RuntimeError(f"{relative_path}: Python tokenization failed: {exc}") from exc

	return hits


def get_path_exception_reason(relative_path: Path, lookup_name: str) -> str | None:
	lookup_reasons = ALLOWED_RELATIVE_PATH_REASONS.get(relative_path)
	if not lookup_reasons:
		return None
	return lookup_reasons.get(lookup_name)


def failure_hint(lookup_name: str) -> str:
	if lookup_name == RAW_GETINFOTYPE_NAME:
		return "use getInfoTypeOrFail(...) for literal XML tag lookups, or add a narrow exception for dynamic/guarded legacy code"
	if lookup_name == RAW_FINDINFOTYPE_NAME:
		return "use findInfoTypeNumOrFail(...) for literal/static XML tag lookups, or add a narrow exception for dynamic/guarded legacy code"
	return "use a strict fail-fast helper, or add a narrow exception for dynamic/guarded legacy code"


def check_raw_infotype_lookups(repo_root: Path, show_ignored: bool) -> tuple[list[str], int, int]:
	failures: list[str] = []
	ignored_hits = 0
	paths = iter_python_files(repo_root)

	for path in paths:
		relative_path = path.relative_to(repo_root)
		hits = find_raw_lookup_hits(repo_root, path)
		if not hits:
			continue

		function_ranges = get_top_level_function_ranges(path)
		for hit in hits:
			allowed_reason = is_in_allowed_function(relative_path, function_ranges, hit)
			if allowed_reason is None:
				allowed_reason = get_path_exception_reason(relative_path, hit.lookup_name)

			if allowed_reason is not None:
				ignored_hits += 1
				if show_ignored:
					print(f"IGNORED {hit.format()} ({allowed_reason})")
				continue

			failures.append(f"{hit.format()} -- {failure_hint(hit.lookup_name)}")

	return failures, len(paths), ignored_hits


def main() -> int:
	parser = argparse.ArgumentParser(description="Check for raw info-type lookups in runtime Python files.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	parser.add_argument("--show-ignored", action="store_true", help="print allowed legacy/dynamic raw lookup hits")
	args = parser.parse_args()

	failures, checked_files, ignored_hits = check_raw_infotype_lookups(args.repo_root, args.show_ignored)

	if failures:
		print("FAIL raw info-type lookup usage")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print(f"PASS raw info-type lookup usage: checked {checked_files} Python files, ignored {ignored_hits} helper/dynamic hits")
	return 0


if __name__ == "__main__":
	sys.exit(main())
