#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Keep the default-off external-agent status contract small: current-state JSON with a timed transition history, transition/turn hooks present, actual process-start identity, and no physical-disk flush requirement.
# Runtime semantics still belong to compilation/gameplay testing. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->

# <!-- custom: Path locates repository files, argparse handles the CLI, and sys exposes the shared workflow-library directory. (GPT-6.1-Sol) -->
from pathlib import Path
import argparse
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))
# <!-- custom: Reuse repository-root discovery and integer-Define parsing/validation instead of duplicating XML handling in this status guard. (GPT-6.1-Sol) -->
from xml_defines import get_default_repo_root, read_global_define_ints, require_int_values


UTILS_HEADER = Path("CvGameCoreDLL/CvGameCoreUtils.h")
UTILS_SOURCE = Path("CvGameCoreDLL/CvGameCoreUtils.cpp")
STATUS_SOURCE = Path("CvGameCoreDLL/SASRuntimeStatus.cpp")
REASON_CATALOG = Path("CvGameCoreDLL/SASRuntimeStatusReasons.h")
GAME_SOURCE = Path("CvGameCoreDLL/CvGame.cpp")
EVENT_SOURCE = Path("CvGameCoreDLL/CvEventReporter.cpp")


def main() -> int:
    parser = argparse.ArgumentParser(description="Check optional SAS runtime-status defaults and lifecycle wiring.")
    parser.add_argument("--repo-root", type=Path, default=get_default_repo_root())
    args = parser.parse_args()
    failures = require_int_values(read_global_define_ints(args.repo_root), {"SAS_RUNTIME_STATUS_ENABLE": 0})
    texts = {}
    for relative in (UTILS_HEADER, UTILS_SOURCE, STATUS_SOURCE, REASON_CATALOG, GAME_SOURCE, EVENT_SOURCE):
        path = args.repo_root / relative
        if not path.is_file():
            failures.append(f"missing runtime-status source: {relative}")
        else:
            texts[relative] = path.read_text(encoding="utf-8", errors="replace")
    if len(texts) == 6:
        header = texts[UTILS_HEADER]
        utils = texts[UTILS_SOURCE] + texts[STATUS_SOURCE] + texts[REASON_CATALOG]
        game = texts[GAME_SOURCE]
        event = texts[EVENT_SOURCE]
        for symbol in (
            "updateSASRuntimeStatusNoGame",
            "updateSASRuntimeStatusSessionStarted",
            "updateSASRuntimeStatusSessionReady",
            "updateSASRuntimeStatusTurnCompleted",
            "updateSASRuntimeStatusAutoPlay",
            "updateSASRuntimeStatusGameEnded",
            "updateSASRuntimeStatusCleanExit",
        ):
            if symbol not in header:
                failures.append(f"{UTILS_HEADER}: missing runtime-status interface {symbol}")
        for token in (
            "SASRuntimeStatus.json",
            r'L"\\Logs\\SASRuntimeStatus.json"',
            "GetProcessTimes(",
            "CREATE_ALWAYS",
            'GC.getDefineINT("SAS_RUNTIME_STATUS_ENABLE")',
            r'\"updateReason\"',
            r'\"sessionStartedUtc\"',
            r'\"nativeReadyUtc\"',
            'DO(SESSION_STARTING)',
            'SAS_RUNTIME_STATUS_REASON(LOAD_GAME_STARTED)',
            'SAS_RUNTIME_STATUS_REASON(NEW_GAME_STARTED)',
        ):
            if token not in utils:
                failures.append(f"{UTILS_SOURCE}: missing runtime-status contract token {token}")
        if "FlushFileBuffers(" in utils:
            failures.append(f"{UTILS_SOURCE}: runtime status must not force physical-disk durability")
        for token in ("updateSASRuntimeStatusNoGame(", "updateSASRuntimeStatusTurnCompleted(", "updateSASRuntimeStatusAutoPlay(", "updateSASRuntimeStatusSessionStarted(true)", "updateSASRuntimeStatusSessionStarted(false)", "updateSASRuntimeStatusSessionReady()", "updateSASRuntimeStatusCleanExit()"):
            if token not in game:
                failures.append(f"{GAME_SOURCE}: missing runtime-status lifecycle hook {token}")
        for token in ("updateSASRuntimeStatusSessionReady()", "updateSASRuntimeStatusGameEnded()"):
            if token not in event:
                failures.append(f"{EVENT_SOURCE}: missing runtime-status lifecycle hook {token}")
    if failures:
        print("FAIL SAS runtime status")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print("PASS SAS runtime status: disabled by default; current-state JSON lifecycle hooks present")
    return 0


if __name__ == "__main__":
    sys.exit(main())
