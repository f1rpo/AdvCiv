#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Keep a small source-policy guard for the shared BBAI/SASGameRecord sink and runtime publisher.
# Exact argument lists, local variable names, checkpoint statements and JSON fields are implementation details; the native harness and game tests exercise their behavior.
# These source checks are conservative review sentinels, not proof of live readability or correct lifecycle handling. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->

# <!-- custom: Path locates maintained publishers, argparse handles the CLI, re masks C++ text and scopes the legacy bridge, and sys returns its status. (GPT-6.1-Sol) -->
from pathlib import Path
import argparse
import re
import sys


UTILS_SOURCE = Path("CvGameCoreDLL/CvGameCoreUtils.cpp")
STATUS_SOURCE = Path("CvGameCoreDLL/SASRuntimeStatus.cpp")
BBAI_SOURCE = Path("CvGameCoreDLL/BBAILog.cpp")
GAME_RECORD_SOURCE = Path("CvGameCoreDLL/SASGameRecordLog.cpp")


# <!-- custom: The legacy xml.log/PythonDbg formatter legitimately calls the EXE logger in the same utility file as the owned writer; a whole-file ban falsely rejected that unrelated bridge.
# Mask comments/literals before balancing its body, and exempt only that function from the EXE-call ban.
# The owned writer and all other utility code remain checked; forced physical-disk flushes stay forbidden throughout. (GPT-6.1-Sol) -->
def executable_cpp(text: str) -> str:
    pattern = r'//[^\n]*|/\*.*?\*/|R"(?P<delimiter>[^\s()\\]{0,16})\(.*?\)(?P=delimiter)"|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    return re.sub(pattern, lambda match: "".join("\n" if char == "\n" else " " for char in match.group()), text, flags=re.S)


def without_legacy_bridge(code: str) -> str:
    pattern = r"\bvoid\s+logSASCiv4LegacyLogRow\s*\([^;{}]*\)\s*\{"
    for match in reversed(list(re.finditer(pattern, code))):
        depth = 1
        end = match.end()
        while end < len(code) and depth:
            depth += (code[end] == "{") - (code[end] == "}")
            end += 1
        if depth == 0:
            code = code[:match.start()] + " " * (end - match.start()) + code[end:]
    return code


def check_source(relative: Path, text: str) -> list[str]:
    failures = []
    code = executable_cpp(text)
    compact = "".join(code.split())
    if "FlushFileBuffers(" in compact:
        failures.append(f"{relative}: diagnostic visibility must not force physical-disk durability")
    owned_code = without_legacy_bridge(code) if relative == UTILS_SOURCE else code
    if "gDLL->logMsg(" in "".join(owned_code.split()):
        failures.append(f"{relative}: SAS diagnostic output must not bypass the owned writer through the EXE logger")
    if relative in (UTILS_SOURCE, STATUS_SOURCE):
        for flag in ("FILE_SHARE_READ", "FILE_SHARE_WRITE", "FILE_SHARE_DELETE"):
            if flag not in code:
                failures.append(f"{relative}: missing live-reader sharing policy flag {flag}")
    elif "logSASDiagnosticLiteralLine" not in code:
        failures.append(f"{relative}: missing shared literal-text diagnostic sink")
    return failures


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

    # <!-- custom: Check actual publisher code; only the unrelated legacy bridge in the shared utility file may retain its EXE call. (GPT-6.1-Sol) -->
    for relative, text in texts.items():
        failures.extend(check_source(relative, text))

    if failures:
        print("FAIL diagnostic sink policy")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print("PASS diagnostic sink policy: shared literal sink, reader-sharing flags and no forced disk flush or EXE fallback")
    return 0


if __name__ == "__main__":
    sys.exit(main())
