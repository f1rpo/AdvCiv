#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: A post-fix replay paragraph was inserted before KI#979's existing archaeology and credits, breaking the discovery thread.
# Check clear paragraph insertions within existing KI entries; append new developments after prior reasoning rather than rewriting their apparent chronology.
# This structural guard permits corrections/reflow and cannot establish whether replacement prose changes historical meaning. (GPT-6.1-Sol) -->

# <!-- custom: argparse exposes the range-aware CLI, difflib aligns existing paragraphs, pathlib locates the repository, re identifies entry boundaries, and sys loads the shared Git context and reports failures. (GPT-6.1-Sol) -->
import argparse
import difflib
from pathlib import Path
import re
import sys

# <!-- custom: Reuse lib/git_context for the same push/PR base resolution and historical-text decoding as the other documentation checks. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))
import git_context

ROOT = Path(__file__).resolve().parents[4]
DOCUMENT = "_1_AdvCiv-SAS/Docs/README_Known_Issues.md"
ENTRY = re.compile(r"^## KI#(\d+(?:\.\d+)*)\b[^\n]*\n", re.MULTILINE)
ANCHOR = re.compile(r'^\s*<a\s+id="ki-[^"]+"\s*></a>\s*$', re.MULTILINE)


# <!-- custom: Index bodies by stable KI number so menu edits, title/status changes, new entries and entry ordering do not masquerade as inserted history.
# Strip the next entry's anchor and normalize paragraph whitespace so source wrapping does not create false additions. (GPT-6.1-Sol) -->
def entries(text):
    headings = list(ENTRY.finditer(text))
    result = {}
    for index, heading in enumerate(headings):
        end = headings[index + 1].start() if index + 1 < len(headings) else len(text)
        body = ANCHOR.sub("", text[heading.end():end])
        paragraphs = []
        line = text.count("\n", 0, heading.end()) + 1
        cursor = 0
        for match in re.finditer(r"\S[^\n]*(?:\n(?!\s*\n)[^\n]*)*", body):
            paragraph = " ".join(match.group().split())
            line += body.count("\n", cursor, match.start())
            cursor = match.start()
            paragraphs.append((paragraph, line))
        result[heading.group(1)] = paragraphs
    return result


# <!-- custom: Reject pure insertions only when prior body paragraphs still follow them.
# Replacements/deletions may be factual corrections, and an entirely new KI has no existing chronology to preserve; review still decides whether such edits are appropriate. (GPT-6.1-Sol) -->
def chronology_errors(old, new):
    errors = []
    previous = entries(old)
    for number, paragraphs in entries(new).items():
        before = previous.get(number)
        if not before:
            continue
        old_values = [value for value, _line in before]
        new_values = [value for value, _line in paragraphs]
        matcher = difflib.SequenceMatcher(None, old_values, new_values, autojunk=False)
        for kind, start, _end, new_start, _new_end in matcher.get_opcodes():
            if kind == "insert" and start < len(before):
                errors.append(f"{DOCUMENT}:{paragraphs[new_start][1]}: KI#{number}: new paragraph inserted before existing history; append the follow-up at the entry's end")
    return errors


def main():
    parser = argparse.ArgumentParser(description="Keep new KI history paragraphs at their entry's end.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--base-ref")
    parser.add_argument("--event-name", choices=("local", "push", "pull_request", "workflow_dispatch"), default="local")
    args = parser.parse_args()
    try:
        base = git_context.ensure_base(args.repo_root, args.base_ref, args.event_name)
        old = git_context.git_text(args.repo_root, base, DOCUMENT) if base else None
        path = args.repo_root / DOCUMENT
        new = path.read_text(encoding="utf-8-sig") if path.exists() else None
        errors = chronology_errors(old, new) if old is not None and new is not None and old != new else []
    except (OSError, RuntimeError) as exc:
        print(f"FAIL Known Issues chronology: {exc}", file=sys.stderr)
        return 1
    if errors:
        print("FAIL Known Issues chronology\n  - " + "\n  - ".join(errors))
        return 1
    print("PASS Known Issues chronology")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
