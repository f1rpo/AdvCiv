#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: argparse/sys provide the local/CI entry point, pathlib handles repository-relative examples, and subprocess asks Git for tracked paths and effective export-ignore attributes.
# Use Git's own attribute resolution rather than duplicating its path-pattern rules. (GPT-6.1-Sol) -->
import argparse
from pathlib import Path, PurePosixPath
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
EXAMPLES = PurePosixPath("_1_AdvCiv-SAS/Docs/examples")
# <!-- custom: Use decimal MB explicitly and check uncompressed bytes; exactly 1 MB is allowed.
# This is a review threshold for release examples, not an automatic exclusion or a compressed-ZIP size estimate. (GPT-6.1-Sol) -->
MAX_BYTES = 1_000_000


def check(repo: Path, max_bytes: int = MAX_BYTES) -> list[str]:
    tracked = subprocess.check_output(["git", "ls-files", "-z", "--", str(EXAMPLES)], cwd=repo)
    paths = [PurePosixPath(name) for name in tracked.decode("utf-8").split("\0") if name]
    if not paths:
        return []
    # <!-- custom: export-ignore on a directory prunes its whole subtree in git archive, even if a child has no attribute itself.
    # Query all ancestors too, including directories above examples, so both file patterns and whole-folder exclusions count. (GPT-6.1-Sol) -->
    queried = {str(path) for path in paths}
    for path in paths:
        queried.update(str(parent) for parent in path.parents if parent != PurePosixPath("."))
    result = subprocess.run(
        ["git", "check-attr", "-z", "--stdin", "export-ignore"], cwd=repo,
        input="".join(name + "\0" for name in sorted(queried)).encode("utf-8"),
        stdout=subprocess.PIPE, check=True,
    )
    fields = result.stdout.decode("utf-8").split("\0")
    excluded = {fields[index] for index in range(0, len(fields) - 1, 3) if fields[index + 2] == "set"}
    errors = []
    folders = {}
    for path in paths:
        if str(path) in excluded or any(str(parent) in excluded for parent in path.parents):
            continue
        local = repo / str(path)
        try:
            size = local.lstat().st_size
        except OSError as exc:
            errors.append(f"{path}: cannot measure tracked example: {exc}")
            continue
        if size > max_bytes:
            errors.append(f"{path}: file includes {size:,} bytes, exceeding {max_bytes:,}; manually review and exclude it with export-ignore or reduce the included content")
        # <!-- custom: Aggregate only archive-included bytes for every example folder, including the examples root.
        # Otherwise one excluded large capture would incorrectly force us to exclude its useful compact siblings too. (GPT-6.1-Sol) -->
        for parent in path.parents:
            if parent == EXAMPLES or EXAMPLES in parent.parents:
                folders[parent] = folders.get(parent, 0) + size
    for folder, size in sorted(folders.items()):
        if size > max_bytes:
            errors.append(f"{folder}/: folder includes {size:,} bytes, exceeding {max_bytes:,}; manually review export-ignore exclusions or reduce the included content")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description="Require manual review of archive-included example files/folders over 1 MB (1,000,000 bytes).")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    args = parser.parse_args()
    try:
        errors = check(args.repo_root)
    except (OSError, subprocess.CalledProcessError) as exc:
        print(f"FAIL example archive size setup: {exc}")
        return 1
    print("FAIL example archive size" if errors else f"PASS example archive size (maximum {MAX_BYTES:,} included bytes per file/folder)")
    for error in errors:
        print("  - " + error)
    return int(bool(errors))


if __name__ == "__main__":
    sys.exit(main())
