#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Keep runtime recording disabled for distributed gameplay; local developer overrides remain separate.
# Source-token inventories cannot establish callback ordering or valid JSON, and made ordinary refactoring require parallel checker edits.
# The native harness exercises output/timing/failure behavior; actual EXE lifecycle observations require game testing. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->

# <!-- custom: Path/argparse provide the repository CLI; sys exposes the shared XML helpers. (GPT-6.1-Sol) -->
from pathlib import Path
import argparse
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))
# <!-- custom: Reuse integer-Define parsing and shipped-value validation rather than inspect source spellings. (GPT-6.1-Sol) -->
from xml_defines import get_default_repo_root, read_global_define_ints, require_int_values


def main() -> int:
    parser = argparse.ArgumentParser(description="Check the shipped runtime-recording default.")
    parser.add_argument("--repo-root", type=Path, default=get_default_repo_root())
    args = parser.parse_args()
    failures = require_int_values(read_global_define_ints(args.repo_root), {"SAS_RUNTIME_STATUS_ENABLE": 0})
    if failures:
        print("FAIL SAS runtime-status default")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print("PASS SAS runtime status: recording disabled by default")
    return 0


if __name__ == "__main__":
    sys.exit(main())
