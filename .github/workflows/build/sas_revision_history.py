#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: A Git object can survive an amend/rebase while no longer belonging to the checked branch. Require ancestry, not just object existence, and verify explicit revision markers against the referenced source. The latest pending entry avoids an impossible self-referential commit hash. (GPT-6.1-Sol) -->
import argparse
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
HISTORY = Path("_1_AdvCiv-SAS/Docs/README_SASGameRecord_Revisions.md")
HEADER = "CvGameCoreDLL/SASGameRecordLog.h"


def check(repo):
    if subprocess.check_output(["git", "rev-parse", "--is-shallow-repository"], cwd=repo).strip() != b"false":
        return ["revision provenance requires full Git history (checkout fetch-depth: 0)"]
    ancestors = set(subprocess.check_output(["git", "rev-list", "HEAD"], cwd=repo).decode().splitlines())
    text = (repo / HISTORY).read_text("utf-8")
    entries = list(re.finditer(r"^### Revision (\d+)\b(.*?)(?=^### Revision |\Z)", text, re.MULTILINE | re.DOTALL))
    if not entries:
        return ["no revision history entries"]
    errors = []
    for index, entry in enumerate(entries):
        revision = int(entry.group(1))
        value = re.search(r"^- \*\*Git commit:\*\* (.+)$", entry.group(2), re.MULTILINE)
        if value is None:
            errors.append(f"revision {revision}: missing Git commit field")
            continue
        commit = value.group(1).strip().strip("`")
        if commit == "pending":
            if index:
                errors.append(f"revision {revision}: only the latest entry may remain pending")
            continue
        if not re.fullmatch(r"[0-9a-f]{40}", commit):
            errors.append(f"revision {revision}: expected a full commit hash or latest-entry pending")
            continue
        if commit not in ancestors:
            errors.append(f"revision {revision}: {commit} is absent from HEAD ancestry; reconcile after amend/rebase")
            continue
        if revision >= 69:
            source = subprocess.run(["git", "show", f"{commit}:{HEADER}"], cwd=repo, capture_output=True)
            marker = re.search(rb"SAS_GAME_RECORD_REVISION\s*=\s*(\d+)", source.stdout)
            if source.returncode or marker is None or int(marker.group(1)) != revision:
                errors.append(f"revision {revision}: {commit} does not contain its matching explicit source marker")
    return errors


def main():
    parser = argparse.ArgumentParser(description="Reject stale SASGameRecord revision commit references.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    errors = check(parser.parse_args().repo_root)
    print("FAIL SASGameRecord revision provenance" if errors else "PASS SASGameRecord revision provenance")
    for error in errors:
        print("  - " + error)
    return int(bool(errors))

if __name__ == "__main__":
    sys.exit(main())
