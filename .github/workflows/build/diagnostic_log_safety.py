#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: BBAI and SASGameRecord must cross Civ4's printf-style EXE log boundary through the shared literal-safe helper.

from pathlib import Path
import argparse
import re
import sys


UTILS_HEADER = Path("CvGameCoreDLL/CvGameCoreUtils.h")
UTILS_SOURCE = Path("CvGameCoreDLL/CvGameCoreUtils.cpp")
BBAI_SOURCE = Path("CvGameCoreDLL/BBAILog.cpp")
GAME_RECORD_SOURCE = Path("CvGameCoreDLL/SASGameRecordLog.cpp")


def get_default_repo_root() -> Path:
	return Path(__file__).resolve().parents[3]


def main() -> int:
	parser = argparse.ArgumentParser(description="Check literal-safe BBAI/SASGameRecord emission through Civ4 gDLL->logMsg.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	failures = []
	texts = {}
	for relative_path in (UTILS_HEADER, UTILS_SOURCE, BBAI_SOURCE, GAME_RECORD_SOURCE):
		path = args.repo_root / relative_path
		if not path.is_file():
			failures.append(f"missing diagnostic log-safety file: {relative_path}")
			continue
		texts[relative_path] = path.read_text(encoding="utf-8", errors="replace")
	if failures:
		for failure in failures:
			print(f"FAIL diagnostic log safety: {failure}")
		return 1

	header_text = texts[UTILS_HEADER]
	utils_text = texts[UTILS_SOURCE]
	bbai_text = texts[BBAI_SOURCE]
	record_text = texts[GAME_RECORD_SOURCE]

	declaration = "void logSASDiagnosticLiteralLine(char const* szLogName, char const* szLine);"
	if declaration not in header_text:
		failures.append(f"{UTILS_HEADER}: missing shared logSASDiagnosticLiteralLine declaration")

	helper_match = re.search(r"void\s+logSASDiagnosticLiteralLine\s*\([^)]*\)\s*\{(?P<body>.*?)^\}", utils_text, flags=re.DOTALL | re.MULTILINE)
	if helper_match is None:
		failures.append(f"{UTILS_SOURCE}: missing logSASDiagnosticLiteralLine implementation")
	else:
		body = helper_match.group("body")
		for required in ("if (*p == '%')", "szEscapedLine += '%';", "szEscapedLine += *p;", "gDLL->logMsg(szLogName, szEscapedLine.c_str(), false, false);"):
			if required not in body:
				failures.append(f"{UTILS_SOURCE}: literal-safe helper missing contract token {required!r}")
		if body.find("szEscapedLine += '%';") > body.find("szEscapedLine += *p;"):
			failures.append(f"{UTILS_SOURCE}: percent duplication must occur before appending the original percent character")

	# The dangerous pattern is an already-formatted dynamic line passed directly to the EXE logger.
	# Keep the two SAS diagnostic sinks centralized through the helper so new producer rows inherit the fix automatically.
	for relative_path, text, expected_call in (
		(BBAI_SOURCE, bbai_text, "logSASDiagnosticLiteralLine(szLogName.GetCString(), szLine.c_str());"),
		(GAME_RECORD_SOURCE, record_text, "logSASDiagnosticLiteralLine(szLogName.GetCString(), szLine.c_str());"),
	):
		if expected_call not in text:
			failures.append(f"{relative_path}: missing shared literal-safe diagnostic sink call")
		if "gDLL->logMsg(" in text:
			failures.append(f"{relative_path}: direct gDLL->logMsg bypasses literal-percent protection")

	# Lightweight executable specification of the boundary transformation documented by the C++ helper.
	# This does not replace runtime Civ4 testing; it makes the intended one-pass contract explicit in CI.
	def escape_for_exe_logger(line: str) -> str:
		return line.replace("%", "%%")

	specimens = {
		"timing=75%": "timing=75%%",
		"best=Farm/140/75% second=Cottage/130/50%": "best=Farm/140/75%% second=Cottage/130/50%%",
		"literal=100%%": "literal=100%%%%",
	}
	for raw, expected in specimens.items():
		actual = escape_for_exe_logger(raw)
		if actual != expected:
			failures.append(f"boundary escape self-test failed: {raw!r} -> {actual!r}, expected {expected!r}")

	if failures:
		print("FAIL diagnostic log literal-percent safety")
		for failure in failures:
			print(f"  - {failure}")
		return 1
	print("PASS diagnostic log literal-percent safety: BBAI and SASGameRecord use the shared final-boundary escape")
	return 0


if __name__ == "__main__":
	sys.exit(main())
