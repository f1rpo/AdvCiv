#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
from __future__ import annotations

import argparse
import filecmp
import shutil
from pathlib import Path


GENERATED_NAMES = {"INDEX.txt", "PATH_HISTORY_INDEX.txt"}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Refresh the canonical local generated commit-diff context from current Git ancestry.")
    parser.add_argument("--repo-root", default=None, help="AdvCiv-SAS repository root. Defaults to the same auto-detection as make_light_source_zip.py.")
    parser.add_argument("--dry-run", action="store_true", help="Report additions/updates/removals without changing local generated context files or the Git-history cache.")
    return parser.parse_args()


def generated_value_matches_file(value: bytes | Path, path: Path) -> bool:
    if not path.is_file():
        return False
    if isinstance(value, Path):
        try:
            return path.stat().st_size == value.stat().st_size and filecmp.cmp(path, value, shallow=False)
        except OSError:
            return False
    try:
        return path.read_bytes() == value
    except OSError:
        return False


def write_generated_value(value: bytes | Path, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if isinstance(value, Path):
        shutil.copyfile(value, path)
    else:
        path.write_bytes(value)


def refresh_commit_diff_context(repo_root: Path, generated: dict[str, bytes | Path], context_dir: str, dry_run: bool = False) -> tuple[int, int, int, int]:
    """Synchronize a full generated history result into its canonical local generated context directory."""
    local_dir = repo_root / context_dir
    prefix = context_dir.rstrip("/") + "/"
    wanted: dict[Path, bytes | Path] = {}
    for rel, value in generated.items():
        if rel.startswith(prefix):
            wanted[local_dir / rel[len(prefix):]] = value

    existing_generated: set[Path] = set()
    if local_dir.is_dir():
        existing_generated.update(path for path in local_dir.glob("*.diff") if path.is_file())
        existing_generated.update(local_dir / name for name in GENERATED_NAMES if (local_dir / name).is_file())
    stale = sorted(existing_generated - set(wanted))

    added = updated = unchanged = 0
    for path, value in sorted(wanted.items(), key=lambda item: item[0].name.lower()):
        if not path.exists():
            added += 1
        elif generated_value_matches_file(value, path):
            unchanged += 1
            continue
        else:
            updated += 1
        if not dry_run:
            write_generated_value(value, path)

    if not dry_run:
        for path in stale:
            path.unlink()
        local_dir.mkdir(parents=True, exist_ok=True)
    return added, updated, len(stale), unchanged


def format_refresh_summary(result: tuple[int, int, int, int]) -> str:
    added, updated, removed, unchanged = result
    return f"added={added}, updated={updated}, removed={removed}, unchanged={unchanged}"


def main() -> int:
    # <!-- custom: Keep this command independently usable by people, Codex and other agents while sharing the exact renderer/cache/filtering policy with the light-source exporter.
    # Import lazily so make_light_source_zip.py can reuse the synchronization function without a circular import. (GPT-5.6-Sol) -->
    import make_light_source_zip as light_source

    args = parse_args()
    repo_root = light_source.find_repo_root(args.repo_root)
    generated, history_summary = light_source.build_commit_diff_history_context(repo_root, -1, write_cache=not args.dry_run)
    result = refresh_commit_diff_context(repo_root, generated, light_source.COMMIT_DIFF_CONTEXT_DIR, dry_run=args.dry_run)
    print(f"Repo root: {repo_root}")
    print(f"Commit-diff context: {light_source.COMMIT_DIFF_CONTEXT_DIR}")
    print(history_summary)
    print(f"Refresh: {format_refresh_summary(result)}")
    if args.dry_run:
        print("Dry run: no local generated context files or history cache were changed.")
    else:
        print("Reminder: the context reflects committed HEAD; the later commit recording this refresh cannot contain its own future diff.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
