#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Keep a small source-policy guard for the shared BBAI/SASGameRecord sink and runtime publisher.
# Exact argument lists, local variable names, checkpoint statements and JSON fields are implementation details; the native harness and game tests exercise their behavior.
# These source checks are conservative review sentinels, not proof of live readability or correct lifecycle handling. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->

# <!-- custom: Path locates maintained publishers, argparse handles the CLI, and sys returns its status. (GPT-6.1-Sol) -->
from pathlib import Path
import argparse
import sys


UTILS_SOURCE = Path("CvGameCoreDLL/CvGameCoreUtils.cpp")
STATUS_SOURCE = Path("CvGameCoreDLL/SASRuntimeStatus.cpp")
BBAI_SOURCE = Path("CvGameCoreDLL/BBAILog.cpp")
GAME_RECORD_SOURCE = Path("CvGameCoreDLL/SASGameRecordLog.cpp")


def main() -> int:
    parser = argparse.ArgumentParser(description="Check diagnostic sink and live-reading source policies.")
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[4])
    args = parser.parse_args()
    failures = []
    texts = {}
    for relative in (UTILS_SOURCE, STATUS_SOURCE, BBAI_SOURCE, GAME_RECORD_SOURCE):
        path = args.repo_root / relative
        if not path.is_file():
            failures.append(f"missing diagnostic publisher source: {relative}")
        else:
            texts[relative] = path.read_text(encoding="utf-8", errors="replace")

    # <!-- custom: Win32 sharing flags express the reader-access policy; their order and surrounding call layout are irrelevant here.
    # Ignore whitespace for prohibited direct calls, without prescribing writer arguments or the implementation of lifecycle checkpoints. (GPT-6.1-Sol) -->
    for relative, text in texts.items():
        compact = "".join(text.split())
        if "FlushFileBuffers(" in compact:
            failures.append(f"{relative}: diagnostic visibility must not force physical-disk durability")
        if "gDLL->logMsg(" in compact:
            failures.append(f"{relative}: SAS diagnostic output must not bypass the owned writer through the EXE logger")
        if relative in (UTILS_SOURCE, STATUS_SOURCE):
            for flag in ("FILE_SHARE_READ", "FILE_SHARE_WRITE", "FILE_SHARE_DELETE"):
                if flag not in text:
                    failures.append(f"{relative}: missing live-reader sharing policy flag {flag}")
        elif "logSASDiagnosticLiteralLine" not in text:
            failures.append(f"{relative}: missing shared literal-text diagnostic sink")

    if failures:
        print("FAIL diagnostic sink policy")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print("PASS diagnostic sink policy: shared literal sink, reader-sharing flags and no forced disk flush or EXE fallback")
    return 0


if __name__ == "__main__":
    sys.exit(main())
