#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
#
# <!-- custom: Build check: BBAI and SASGameRecord share the DLL-owned live-readable prompt writer and keep literal text safe. (ChatGPT-5.6-Sol) -->

from pathlib import Path
import argparse
import re
import sys


UTILS_HEADER = Path("CvGameCoreDLL/CvGameCoreUtils.h")
UTILS_SOURCE = Path("CvGameCoreDLL/CvGameCoreUtils.cpp")
BBAI_SOURCE = Path("CvGameCoreDLL/BBAILog.cpp")
GAME_RECORD_SOURCE = Path("CvGameCoreDLL/SASGameRecordLog.cpp")
GAME_SOURCE = Path("CvGameCoreDLL/CvGame.cpp")
BUG_INIT = Path("Assets/Python/BUG/BugInit.py")
EVENT_INTERFACE = Path("Assets/Python/EntryPoints/CvEventInterface.py")


def get_default_repo_root() -> Path:
	return Path(__file__).resolve().parents[4]


def main() -> int:
	parser = argparse.ArgumentParser(description="Check live-readable BBAI/SASGameRecord emission and literal-text safety.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	failures = []
	texts = {}
	paths = (UTILS_HEADER, UTILS_SOURCE, BBAI_SOURCE, GAME_RECORD_SOURCE, GAME_SOURCE, BUG_INIT, EVENT_INTERFACE)
	for relative_path in paths:
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
	game_text = texts[GAME_SOURCE]
	bug_init_text = texts[BUG_INIT]
	event_text = texts[EVENT_INTERFACE]

	# <!-- custom: Keep this a small sink/sharing policy check, not a source-shape substitute for exercising I/O; the local C++ harness tests writes and failures with the actual writer code. (GPT-6.1-Sol) -->
	for symbol in ("logSASDiagnosticLiteralLine", "setSASDiagnosticLogRootDirectory", "flushSASDiagnosticLogs"):
		if not re.search(r"\b" + symbol + r"\s*\(", header_text):
			failures.append(f"{UTILS_HEADER}: missing shared writer interface {symbol}")
	if not re.search(r"CreateFileW\([^;]+FILE_SHARE_READ\s*\|\s*FILE_SHARE_WRITE\s*\|\s*FILE_SHARE_DELETE", utils_text):
		failures.append(f"{UTILS_SOURCE}: diagnostic output must use Unicode paths and permit live reading/copying")
	if "FlushFileBuffers(" in utils_text:
		failures.append(f"{UTILS_SOURCE}: live-readability flush must not request physical-disk durability through FlushFileBuffers")
	if "gDLL->logMsg(" in utils_text:
		failures.append(f"{UTILS_SOURCE}: shared SAS diagnostic writer must not fall back to the opaque EXE logger")
	if "szEscapedLine" in utils_text or "if (*p == '%')" in utils_text:
		failures.append(f"{UTILS_SOURCE}: direct writer should preserve already-formatted percent signs rather than EXE-boundary doubling")

	for relative_path, text, expected_call in (
		(BBAI_SOURCE, bbai_text, "logSASDiagnosticLiteralLine(szLogName.GetCString(), szLine.c_str());"),
		(GAME_RECORD_SOURCE, record_text, "logSASDiagnosticLiteralLine(szLogName.GetCString(), szLine.c_str());"),
	):
		if expected_call not in text:
			failures.append(f"{relative_path}: missing shared diagnostic sink call")
		if "gDLL->logMsg(" in text:
			failures.append(f"{relative_path}: direct gDLL->logMsg bypasses live-readable writing")

	turn_row = "if (isSASGameRecordLogEnabled()) logSASGameRecordTurnCompleted(getGameTurn());"
	turn_flush = "if (isSASBBAILogEnabled() || isSASGameRecordLogEnabled()) flushSASDiagnosticLogs();"
	if turn_row not in game_text or turn_flush not in game_text or game_text.find(turn_flush, game_text.find(turn_row)) < game_text.find(turn_row):
		failures.append(f"{GAME_SOURCE}: completed-turn boundary no longer retries pending diagnostic writes after the GameRecord completion marker")
	# <!-- custom: Shutdown must not defer early diagnostics for a BUG root handoff that will never happen; rollover retains that wait. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	destructor = re.search(r"CvGame::~CvGame\(\)\s*\{(.*?)\n\}", game_text, re.S)
	if destructor is None or "flushSASDiagnosticLogs(true, true);" not in destructor.group(1):
		failures.append(f"{GAME_SOURCE}: terminal destruction must allow fallback flushing of pre-BUG output")
	if not re.search(r"else if \(isSASBBAILogEnabled\(\)\) flushSASDiagnosticLogs\(true\);", game_text):
		failures.append(f"{GAME_SOURCE}: BBAI-only reset no longer publishes the old-session shared buffer")
	if "flushSASDiagnosticLogs(true);" not in record_text:
		failures.append(f"{GAME_RECORD_SOURCE}: session finalization no longer publishes the shared diagnostic buffers")
	if "CyGame().setSASDiagnosticLogRoot(szRoot)" not in bug_init_text:
		failures.append(f"{BUG_INIT}: BUG root resolution no longer supplies the shared diagnostic writer")
	if "CyGame().flushSASDiagnosticLogs()" not in event_text:
		failures.append(f"{EVENT_INTERFACE}: session event no longer publishes finalized GameStart/OnLoad diagnostic batches")

	if failures:
		print("FAIL diagnostic live-buffer/log safety")
		for failure in failures:
			print(f"  - {failure}")
		return 1
	print("PASS diagnostic live-buffer/log safety: BBAI and SASGameRecord use read-shared files with literal text preserved")
	return 0


if __name__ == "__main__":
	sys.exit(main())
