#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: every Long_Comments entry must still have a live repository marker, and every live marker must resolve to an archived entry.

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
ARCHIVE_DIR = Path("_1_AdvCiv-SAS/Docs/Long_Comments")
ARCHIVES = (
    "Long_Comments_XML.txt",
    "Long_Comments_py.txt",
)
ENTRY_RE = re.compile(r"^----- Comment #(\d+) -----\s*$", re.MULTILINE)
SECTION_RE = re.compile(r"^===== (.+?) =====\s*$")
MARKER_RE = re.compile(r"#\s*(\d+)")
TEXT_SUFFIXES = {
    ".cfg", ".cpp", ".h", ".hpp", ".ini", ".inl", ".md", ".py", ".txt", ".xml",
}
# Historical/generated material can legitimately mention comments that no longer belong to current source.
IGNORE_PREFIXES = (
    ".github/",
    "_1_AdvCiv-SAS/Docs/changelogs_web/",
    "_1_AdvCiv-SAS/Docs/git_logs/",
    "LLM_Helpers/context/",
    "_SNAPSHOT_CONTEXT/",
)
IGNORE_FILES = {
    "_LLM_REPO_FILE_MANIFEST.txt",
}


def archive_entries(path: Path) -> tuple[dict[int, str], list[str]]:
    errors: list[str] = []
    entries: dict[int, str] = {}
    section = "(unknown section)"
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError as exc:
        return {}, [f"{path.name}: archive is not valid UTF-8 ({exc})"]
    if "\ufffd" in text or re.search(r"[\x80-\x9f]", text):
        errors.append(f"{path.name}: archive contains Unicode replacement/C1 control characters; repair the text encoding")
    for line_number, line in enumerate(text.splitlines(), 1):
        section_match = SECTION_RE.match(line)
        if section_match:
            section = section_match.group(1).replace("\\", "/").lstrip("/")
            continue
        entry_match = re.match(r"^----- Comment #(\d+) -----\s*$", line)
        if not entry_match:
            continue
        number = int(entry_match.group(1))
        if number in entries:
            errors.append(f"{path.name}: duplicate Comment #{number} at line {line_number}")
        else:
            entries[number] = section
    return entries, errors


def ignored(relative: Path, archive_relative: Path) -> bool:
    name = relative.as_posix()
    if name.startswith(ARCHIVE_DIR.as_posix() + "/") or name in IGNORE_FILES:
        return True
    return any(name.startswith(prefix) for prefix in IGNORE_PREFIXES)


def live_markers(repo_root: Path, archive_relative: Path) -> dict[int, list[str]]:
    basename = archive_relative.name
    result: dict[int, list[str]] = {}
    for path in repo_root.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in TEXT_SUFFIXES:
            continue
        relative = path.relative_to(repo_root)
        if ignored(relative, archive_relative):
            continue
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        for line_number, line in enumerate(lines, 1):
            index = line.find(basename)
            if index < 0:
                continue
            for match in MARKER_RE.finditer(line[index + len(basename):]):
                number = int(match.group(1))
                result.setdefault(number, []).append(f"{relative.as_posix()}:{line_number}")
    return result


def check(repo_root: Path) -> list[str]:
    errors: list[str] = []
    archive_root = repo_root / ARCHIVE_DIR
    if not archive_root.is_dir():
        return [f"{ARCHIVE_DIR.as_posix()}: missing Long_Comments documentation folder"]

    for basename in ARCHIVES:
        relative = ARCHIVE_DIR / basename
        path = repo_root / relative
        if not path.is_file():
            errors.append(f"{relative.as_posix()}: missing archive")
            continue

        entries, parse_errors = archive_entries(path)
        errors.extend(parse_errors)
        markers = live_markers(repo_root, relative)

        for number in sorted(markers.keys() - entries.keys()):
            errors.append(
                f"{basename} #{number}: live marker has no archived entry ({', '.join(markers[number])})"
            )
        for number in sorted(entries.keys() - markers.keys()):
            errors.append(
                f"{basename} #{number}: archived entry is orphaned; remove it or restore a live marker"
                f" (section: {entries[number]})"
            )

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description="Check Long_Comments markers and archived entries for bidirectional consistency.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    args = parser.parse_args()

    errors = check(args.repo_root)
    print("FAIL Long_Comments references" if errors else "PASS Long_Comments references")
    for error in errors:
        print("  - " + error)
    return int(bool(errors))


if __name__ == "__main__":
    sys.exit(main())
