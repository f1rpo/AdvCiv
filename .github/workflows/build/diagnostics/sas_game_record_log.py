#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: SASGameRecord must be disabled by default and its public revision marker must match the latest maintained history entry.
# Runtime/schema correctness belongs to compilation, gameplay tests, log inspection and code review rather than source-shape regex contracts.

from pathlib import Path
import argparse
import re
import sys

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root, read_global_define_ints, require_int_values


REVISION_HEADER = Path("CvGameCoreDLL/SASGameRecordLog.h")
REVISION_SOURCE = Path("CvGameCoreDLL/SASGameRecordLog.cpp")
REVISION_HISTORY = Path("_1_AdvCiv-SAS/Docs/README_SASGameRecord_Revisions.md")
EXPECTED_GAME_RECORD_DEFAULTS = {
    "SAS_GAME_RECORD_LOG_LEVEL": 0,
}


def source_revision(header_text: str) -> tuple[int | None, list[str]]:
    values = re.findall(r"\bSAS_GAME_RECORD_REVISION\s*=\s*(\d+)\b", header_text)
    if len(values) != 1:
        return None, [f"{REVISION_HEADER}: expected exactly one SAS_GAME_RECORD_REVISION assignment, found {len(values)}"]
    return int(values[0]), []


def latest_history_revision(history_text: str) -> tuple[int | None, list[str]]:
    for line in history_text.splitlines():
        if not line.startswith("### Revision "):
            continue
        match = re.match(r"### Revision (\d+) - SAS practical (\d+)\b", line)
        if match is None:
            return None, [f"{REVISION_HISTORY}: malformed newest revision heading: {line}"]
        return int(match.group(1)), []
    return None, [f"{REVISION_HISTORY}: no revision heading found"]


def check_revision(repo_root: Path) -> list[str]:
    failures: list[str] = []
    paths = (REVISION_HEADER, REVISION_SOURCE, REVISION_HISTORY)
    for relative in paths:
        if not (repo_root / relative).is_file():
            failures.append(f"missing SASGameRecord revision file: {relative}")
    if failures:
        return failures

    header = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
    history = (repo_root / REVISION_HISTORY).read_text(encoding="utf-8", errors="replace")
    source = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
    revision, errors = source_revision(header)
    failures.extend(errors)
    history_revision, errors = latest_history_revision(history)
    failures.extend(errors)
    if revision is not None and history_revision is not None and revision != history_revision:
        failures.append(f"revision mismatch: source={revision}, newest history={history_revision}")

    # <!-- custom: Keep one small wiring check because the revision is only useful to downstream readers if the source-context row still emits it.
    # Avoid pinning any other recorder implementation shape here. (ChatGPT-5.6-Sol) -->
    if "GAME_RECORD_SOURCE_CONTEXT recordRevision=%d %s" not in source or "SAS_GAME_RECORD_REVISION" not in source:
        failures.append(f"{REVISION_SOURCE}: source-context row no longer emits SAS_GAME_RECORD_REVISION")
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description="Check SASGameRecord disabled default and public revision synchronization.")
    parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
    args = parser.parse_args()

    defines = read_global_define_ints(args.repo_root)
    failures = require_int_values(defines, EXPECTED_GAME_RECORD_DEFAULTS)
    failures.extend(check_revision(args.repo_root))
    if failures:
        print("FAIL SASGameRecord minimal safety checks")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print("PASS SASGameRecord minimal safety checks: logging disabled by default; revision source/history/source-context synchronized")
    return 0


if __name__ == "__main__":
    sys.exit(main())
