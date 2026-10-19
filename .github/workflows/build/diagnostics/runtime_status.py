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
INFO_SOURCE = Path("CvGameCoreDLL/CvInfo_Misc.cpp")
GAME_INTERFACE_SOURCE = Path("CvGameCoreDLL/CvGameInterface.cpp")
POPUP_SOURCE = Path("CvGameCoreDLL/CvDLLButtonPopup.cpp")
CYGAME_HEADER = Path("CvGameCoreDLL/CyGame.h")
CYGAME_SOURCE = Path("CvGameCoreDLL/CyGame.cpp")
CYGAME_INTERFACE_SOURCE = Path("CvGameCoreDLL/CyGameInterface.cpp")
SCREENS_SOURCE = Path("Assets/Python/EntryPoints/CvScreensInterface.py")


def main() -> int:
    parser = argparse.ArgumentParser(description="Check optional SAS runtime-status defaults and lifecycle wiring.")
    parser.add_argument("--repo-root", type=Path, default=get_default_repo_root())
    args = parser.parse_args()
    failures = require_int_values(read_global_define_ints(args.repo_root), {"SAS_RUNTIME_STATUS_ENABLE": 0})
    texts = {}
    sources = (UTILS_HEADER, UTILS_SOURCE, STATUS_SOURCE, REASON_CATALOG, GAME_SOURCE, EVENT_SOURCE, INFO_SOURCE, GAME_INTERFACE_SOURCE, POPUP_SOURCE, CYGAME_HEADER, CYGAME_SOURCE, CYGAME_INTERFACE_SOURCE, SCREENS_SOURCE)
    for relative in sources:
        path = args.repo_root / relative
        if not path.is_file():
            failures.append(f"missing runtime-status source: {relative}")
        else:
            texts[relative] = path.read_text(encoding="utf-8", errors="replace")
    if len(texts) == len(sources):
        header = texts[UTILS_HEADER]
        utils = texts[UTILS_SOURCE] + texts[STATUS_SOURCE] + texts[REASON_CATALOG]
        game = texts[GAME_SOURCE]
        event = texts[EVENT_SOURCE]
        info = texts[INFO_SOURCE]
        game_interface = texts[GAME_INTERFACE_SOURCE]
        popup = texts[POPUP_SOURCE]
        cygame = texts[CYGAME_HEADER] + texts[CYGAME_SOURCE] + texts[CYGAME_INTERFACE_SOURCE]
        screens = texts[SCREENS_SOURCE]
        for symbol in (
            "updateSASRuntimeStatusNoGame",
            "updateSASRuntimeStatusSessionStarted",
            "updateSASRuntimeStatusSessionReady",
            "updateSASRuntimeStatusTurnCompleted",
            "updateSASRuntimeStatusAutoPlay",
            "updateSASRuntimeStatusGameEnded",
            "updateSASRuntimeStatusMainMenuSceneRequested",
            "updateSASRuntimeStatusLoadingScreenAssetRequested",
            "updateSASRuntimeStatusWorldBuilderMode",
            "updateSASRuntimeStatusWorldBuilderScreen",
            "updateSASRuntimeStatusExtendedGameStarted",
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
            'SAS_RUNTIME_STATUS_REASON(MAIN_MENU_SCENE_REQUESTED)',
            'SAS_RUNTIME_STATUS_REASON(LOADING_SCREEN_ASSET_REQUESTED)',
            'SAS_RUNTIME_STATUS_REASON(WORLD_BUILDER_MODE_ENTERED)',
            'SAS_RUNTIME_STATUS_REASON(WORLD_BUILDER_SCREEN_SHOWN)',
            'SAS_RUNTIME_STATUS_REASON(EXTENDED_GAME_STARTED)',
            r'\"mainMenuSceneRequestedUtc\"',
            r'\"loadingScreenAssetRequestedUtc\"',
            r'\"worldBuilderMode\"',
            r'\"worldBuilderScreenVisible\"',
            r'\"extendedGame\"',
            r'\"record\":\"run\"',
            r'\"record\":\"session\"',
            r'\"record\":\"transition\"',
            r'\"priorStatusWriteFailures\"',
            r'\"priorHistoryWriteFailures\"',
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
        for token in ("updateSASRuntimeStatusMainMenuSceneRequested()", "updateSASRuntimeStatusLoadingScreenAssetRequested()"):
            if token not in info:
                failures.append(f"{INFO_SOURCE}: missing passive presentation hook {token}")
        soundtrack_start = info.find("std::string CvMainMenuInfo::getSoundtrack() const")
        loading_start = info.find("std::string CvMainMenuInfo::getLoading() const", soundtrack_start)
        if soundtrack_start < 0 or loading_start < 0 or "updateSASRuntimeStatus" in info[soundtrack_start:loading_start]:
            failures.append(f"{INFO_SOURCE}: soundtrack getter must remain observationally untouched; querying it changes shuffle/ASyncRand state")
        if "updateSASRuntimeStatusExtendedGameStarted()" not in game or "GAMESTATE_OVER" not in game or "GAMESTATE_EXTENDED" not in game:
            failures.append(f"{GAME_SOURCE}: missing authoritative extended-game transition hook")
        for source_path, source_text in ((GAME_INTERFACE_SOURCE, game_interface), (POPUP_SOURCE, popup)):
            if "setWorldBuilder" not in source_text or "updateSASRuntimeStatusWorldBuilderMode" not in source_text:
                failures.append(f"{source_path}: missing native WorldBuilder mode observation")
        for token in ("updateSASRuntimeStatusWorldBuilderScreen", 'def("updateSASRuntimeStatusWorldBuilderScreen"', "isInAdvancedStart()", "GetWorldBuilderMode()"):
            if token not in cygame:
                failures.append(f"CyGame bridge: missing WorldBuilder runtime interface {token}")
        for token in ("def showWorldBuilderScreen", "def hideWorldBuilderScreen", "updateSASRuntimeStatusWorldBuilderScreen(True)", "updateSASRuntimeStatusWorldBuilderScreen(False)"):
            if token not in screens:
                failures.append(f"{SCREENS_SOURCE}: missing filtered WorldBuilder presentation hook {token}")
        if "getSoundtrack" in texts[STATUS_SOURCE] or "isInMainMenu" in texts[STATUS_SOURCE]:
            failures.append(f"{STATUS_SOURCE}: runtime status must not query side-effectful soundtrack or poll main-menu state")
    if failures:
        print("FAIL SAS runtime status")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print("PASS SAS runtime status: disabled by default; lifecycle/presentation hooks are event-driven and avoid side-effectful polling")
    return 0


if __name__ == "__main__":
    sys.exit(main())
