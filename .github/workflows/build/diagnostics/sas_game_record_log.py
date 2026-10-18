#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: SASGameRecord must be disabled by default and its single public revision label must match the maintained history.
# Runtime/schema correctness belongs to compilation, gameplay tests, log inspection and code review rather than source-shape regex contracts.

from pathlib import Path
import argparse
import re
import sys

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root, read_global_define_ints, require_int_values


REVISION_DEFINES = Path("Assets/XML/GlobalDefines_advciv_sas.xml")
REVISION_SOURCE = Path("CvGameCoreDLL/SASGameRecordLog.cpp")
REVISION_HISTORY = Path("_1_AdvCiv-SAS/Docs/revisions/SASGameRecord.md")
EXPECTED_GAME_RECORD_DEFAULTS = {
    "SAS_GAME_RECORD_LOG_LEVEL": 0,
    # <!-- custom: Ship action tracing ready for deliberately enabled records while the recorder master stays off; catch accidental staging of an actions-off test override. (GPT-6.1-Sol) -->
    "SAS_GAME_RECORD_ACTION_TRACE_ENABLE": 1,
    # <!-- custom: Keep richer support context opt-in; catch hardware/path/process and private/network INI test settings accidentally staged for shipping. (GPT-6.1-Sol + ChatGPT-5.6-Sol) -->
    "SAS_GAME_RECORD_SYSTEM_CONTEXT_LEVEL": 2,
    "SAS_GAME_RECORD_HARDWARE_IDENTITY_ENABLE": 0,
    "SAS_GAME_RECORD_EXACT_PATHS_ENABLE": 0,
    "SAS_GAME_RECORD_PROCESS_CONTEXT_ENABLE": 0,
    "SAS_GAME_RECORD_BUG_STRING_VALUES_ENABLE": 0,
    "SAS_GAME_RECORD_INI_PRIVATE_VALUES_ENABLE": 0,
    "SAS_GAME_RECORD_INI_NETWORK_VALUES_ENABLE": 0,
}


def latest_history_revision(history_text: str) -> tuple[int | None, list[str]]:
    for line in history_text.splitlines():
        if not line.startswith("### Revision "):
            continue
        match = re.match(r"### Revision (\d+) - SAS practical (\d+)\b", line)
        if match is None:
            return None, [f"{REVISION_HISTORY}: malformed newest revision heading: {line}"]
        return int(match.group(1)), []
    return None, [f"{REVISION_HISTORY}: no revision heading found"]


# <!-- custom: Accept the already-parsed Define map from main so this check shares the same shipped-default read; focused tests/direct callers may omit it and let the helper load XML itself. (ChatGPT-5.6-Sol) -->
def check_revision(repo_root: Path, defines: dict[str, int] | None = None) -> list[str]:
    failures: list[str] = []
    for relative in (REVISION_DEFINES, REVISION_SOURCE, REVISION_HISTORY):
        if not (repo_root / relative).is_file():
            failures.append(f"missing SASGameRecord revision file: {relative}")
    if failures:
        return failures

    if defines is None:
        defines = read_global_define_ints(repo_root)
    history = (repo_root / REVISION_HISTORY).read_text(encoding="utf-8", errors="replace")
    source = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
    revision = defines.get("SAS_GAME_RECORD_REVISION")
    if revision is None:
        failures.append("SAS_GAME_RECORD_REVISION: missing define")
    elif revision <= 0:
        failures.append(f"SAS_GAME_RECORD_REVISION: expected positive revision, found {revision}")

    history_revision, errors = latest_history_revision(history)
    failures.extend(errors)
    if revision is not None and history_revision is not None and revision != history_revision:
        failures.append(f"revision mismatch: SAS_GAME_RECORD_REVISION={revision}, newest history={history_revision}")

    # <!-- custom: The revision is a label, not compatibility/runtime branching.
    # Keep only the minimal wiring contract that SOURCE_CONTEXT reads that one Define directly. (ChatGPT-5.6-Sol) -->
    if 'GAME_RECORD_SOURCE_CONTEXT recordRevision=%d %s' not in source or 'GC.getDefineINT("SAS_GAME_RECORD_REVISION")' not in source:
        failures.append(f"{REVISION_SOURCE}: source-context row no longer emits the shared SAS_GAME_RECORD_REVISION label")
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description="Check SASGameRecord disabled default and public revision synchronization.")
    parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
    args = parser.parse_args()

    defines = read_global_define_ints(args.repo_root)
    failures = require_int_values(defines, EXPECTED_GAME_RECORD_DEFAULTS)
    failures.extend(check_revision(args.repo_root, defines))
    if failures:
        print("FAIL SASGameRecord minimal safety checks")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print("PASS SASGameRecord minimal safety checks: logging disabled by default; revision Define/history/source-context synchronized")
    return 0


if __name__ == "__main__":
    sys.exit(main())
