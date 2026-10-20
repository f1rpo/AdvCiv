#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
"""Preview redundant source separators; use --apply to trim them with backups."""

# <!-- custom: argparse selects preview/apply and backup paths; pathlib handles repository-relative files, subprocess enumerates tracked paths without touching the index, sys loads the shared checker, and tempfile keeps default backups outside the checkout. (GPT-6.1-Sol) -->
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
# <!-- custom: Reuse CI's source scope and literal/text protections so the cleanup and prevention rule agree; do not maintain a second parser in this manual helper. (GPT-6.1-Sol) -->
sys.path.insert(0, str(ROOT / ".github/workflows/build/source"))
import source_change_provenance as checker


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="*", help="Optional repository-relative tracked files; otherwise inspect all maintained source")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--apply", action="store_true", help="Rewrite eligible separators; default is preview only")
    parser.add_argument("--backup-dir", type=Path, help="Backup root; default is a new system temporary directory when applying changes")
    args = parser.parse_args()
    repo = args.repo_root.resolve()
    paths = subprocess.check_output(["git", "ls-files", "-z"], cwd=repo).decode("utf-8").split("\0")
    selected = {Path(path).as_posix() for path in args.paths}
    unknown = selected - set(paths)
    if unknown:
        parser.error("Not tracked repository-relative files: " + ", ".join(sorted(unknown)))
    backup = args.backup_dir.resolve() if args.backup_dir else None
    files = runs_total = removed_total = 0
    for relative in paths:
        if selected and relative not in selected:
            continue
        # <!-- custom: SAS define groups deliberately retain their wider separators; historical docs and context corpora are already outside the shared source scope. (GPT-6.1-Sol) -->
        if not checker.maintained_source_path(relative) or relative == "Assets/XML/GlobalDefines_advciv_sas.xml":
            continue
        path = repo / relative
        if path.is_symlink():
            continue
        text = checker.current_text(repo, relative)
        if text is None:
            continue
        runs = checker.long_blank_runs(text, path.suffix.lower())
        if not runs:
            continue
        raw = path.read_bytes()
        rows = raw.splitlines(keepends=True)
        # <!-- custom: Delete only whole blank rows from original bytes, preserving encoding/BOM, line endings and all other spacing; refuse unusual line mappings rather than risk rewriting payload text. (GPT-6.1-Sol) -->
        if len(rows) != len(text.splitlines()):
            raise ValueError("Ambiguous line boundaries: " + relative)
        removed = {i for start, end in runs for i in range(start + 2, end)}
        if any(rows[i].strip() for i in removed):
            raise ValueError("Nonblank bytes in selected rows: " + relative)
        updated = b"".join(row for i, row in enumerate(rows) if i not in removed)
        print("%s: %d run(s), %d blank line(s)" % (relative, len(runs), len(removed)))
        if args.apply:
            if backup is None:
                backup = Path(tempfile.mkdtemp(prefix="sas-blank-lines-"))
            saved = backup / relative
            saved.parent.mkdir(parents=True, exist_ok=True)
            with saved.open("xb") as stream:
                stream.write(raw)
            path.write_bytes(updated)
        files += 1
        runs_total += len(runs)
        removed_total += len(removed)
    print("%s: %d files, %d runs, %d blank lines" % ("APPLIED" if args.apply else "PREVIEW", files, runs_total, removed_total))
    if args.apply and files:
        print("Backups: " + str(backup))
    return 0


if __name__ == "__main__":
    sys.exit(main())
