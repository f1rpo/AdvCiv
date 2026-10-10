#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Check logical Markdown prose blocks only on added/edited lines so new bulky prose does not accumulate without forcing unrelated historical document rewrites.
# This is source layout, not an automatic decision about rendered paragraphs or nested bullets. (GPT-6.1-Sol) -->
import argparse
import difflib
from pathlib import Path
import re
import sys

# <!-- custom: Reuse the provenance checker's Git/base resolution and the existing prose transformer instead of duplicating push-history handling or sentence-boundary heuristics. (GPT-6.1-Sol) -->
import source_change_provenance as git_context

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "LLM_Helpers"))
import reflow_markdown_prose as prose


# <!-- custom: Follow the maintained Markdown scope used by markdownlint, excluding copied/generated context and published numbered changelogs while retaining their maintained README indexes. (GPT-6.1-Sol) -->
def maintained(path):
    path = path.replace("\\", "/")
    if not path.lower().endswith(".md"):
        return False
    excluded = ("_0_Common_Docs/", "_SNAPSHOT_CONTEXT/", "LLM_Helpers/outputs/", "LLM_Helpers/examples/", "LLM_Helpers/context/")
    if path.startswith(excluded) or "node_modules" in Path(path).parts:
        return False
    return not ("changelogs_web" in Path(path).parts and re.match(r"_?\d", Path(path).name))


# <!-- custom: Check rendered prose blocks, not isolated physical lines: soft newlines left the earlier cleanup visually unchanged and must not bypass this rule.
# Blank lines and list items separate thoughts; code, quotes, tables and HTML comments remain outside the prose check. (GPT-6.1-Sol) -->
def prose_blocks(text):
    lines = []
    parts = []
    fence = None
    comment = False
    list_indent = None
    for index, line in enumerate(text.splitlines()):
        stripped = line.lstrip()
        marker = re.match(r"^(`{3,}|~{3,})", stripped)
        excluded = not stripped or fence is not None or marker is not None
        if marker:
            token = marker.group(1)
            if fence is None:
                fence = token
            elif token[0] == fence[0] and len(token) >= len(fence):
                fence = None
        if not marker and fence is None and (comment or "<!--" in line):
            excluded = True
            if "<!--" in line:
                comment = "-->" not in line[line.rfind("<!--") + 4:]
            elif "-->" in line:
                comment = False
        item = prose.LIST_PREFIX_RE.match(line) if not excluded else None
        if not excluded:
            # <!-- custom: A list's wrapped prose can be indented like code; accept it only inside that active item, while preserving standalone indented-code exemptions. (GPT-6.1-Sol) -->
            continuation = list_indent is not None and len(line) - len(stripped) >= list_indent
            excluded = prose.looks_risky(stripped if continuation else line, False)
        if excluded or item:
            if lines:
                yield lines, " ".join(parts)
            lines, parts, list_indent = [], [], None
        if excluded:
            continue
        if item:
            list_indent = len(item.group(1).expandtabs(4))
            body = item.group(2)
        else:
            body = stripped
        lines.append(index)
        parts.append(body)
    if lines:
        yield lines, " ".join(parts)


# <!-- custom: The 400-character cleanup still exposed blocks with only a short heading or trailing follow-up sentence; forcing those apart repeated the earlier over-splitting mistake.
# Require at least 100 characters on each side of a sentence boundary before demanding visible separation; these are candidate filters, not fixed-width wraps. (GPT-6.1-Sol) -->
def needs_split(body):
    pieces = prose.split_prose_line(body, prose.DEFAULT_MIN_LINE_LEN)
    left = 0
    for piece in pieces[:-1]:
        left += len(piece) + (1 if left else 0)
        if left >= 100 and len(body) - left - 1 >= 100:
            return True
    return False


# <!-- custom: Enforce the 400-character candidate threshold only when a block has a safe sentence boundary and intersects added/edited Git lines.
# Reviewers choose a few coherent paragraphs or child bullets; the check never requires one paragraph per sentence or a fixed-width wrap. (GPT-6.1-Sol) -->
def findings(path, old, new, edited_lines=None):
    if not maintained(path):
        return []
    candidates = [(lines, body) for lines, body in prose_blocks(new)
                  if needs_split(body)]
    # <!-- custom: The full-document cleanup exposed expensive repeated-line matching in large issue histories; avoid Git-line matching when there is no reportable block. (GPT-6.1-Sol) -->
    if not candidates:
        return []
    edited = edited_lines
    if edited is None:
        edited = set()
        for tag, _a, _b, start, end in difflib.SequenceMatcher(None, old.splitlines(), new.splitlines(), autojunk=False).get_opcodes():
            if tag in ("insert", "replace"):
                edited.update(range(start, end))
    errors = []
    for lines, _body in candidates:
        affected = edited.intersection(lines)
        if affected:
            errors.append(f"{path}:{min(affected) + 1}: edited prose block reaches {prose.DEFAULT_MIN_LINE_LEN} characters and has separate sentences; group distinct thoughts into blank-line paragraphs or child bullets (soft newlines do not split a rendered paragraph)")
    return errors


# <!-- custom: Support local working-tree review and push/PR bases through the same Git selection as the provenance checker; deleted files cannot introduce bulky prose. (GPT-6.1-Sol) -->
def check(repo, base_ref, event_name):
    base = git_context.ensure_base(repo, base_ref, event_name)
    if base is None:
        return [], 0
    raw = git_context.run_git(repo, "diff", "--name-only", "-z", "--diff-filter=ACMRT", base, "--").stdout
    errors = []
    count = 0
    for path in raw.decode("utf-8").split("\0"):
        if not maintained(path):
            continue
        current = repo / path
        if not current.is_file():
            continue
        old = git_context.git_text(repo, base, path) or ""
        _encoding, new = prose.decode_bytes(current.read_bytes())
        # <!-- custom: Use Git's actual changed-line hunks for repository checks; matching entire repeated issue histories in Python made the initial full-document run unnecessarily slow. (GPT-6.1-Sol) -->
        patch = git_context.run_git(repo, "diff", "--no-ext-diff", "--no-textconv", "--unified=0", base, "--", path).stdout.decode("utf-8")
        edited = set()
        for start, count_text in re.findall(r"^@@ -[^ ]+ \+(\d+)(?:,(\d+))? @@", patch, re.MULTILINE):
            first = int(start) - 1
            edited.update(range(first, first + (int(count_text) if count_text else 1)))
        errors.extend(findings(path, old, new, edited))
        count += 1
    return errors, count


# <!-- custom: CI reports all affected edited lines and never writes Markdown; sentence splitting does not replace human review of visible paragraph/list structure. (GPT-6.1-Sol) -->
def main():
    parser = argparse.ArgumentParser(description="Check long Markdown prose blocks only on added/edited lines.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--base-ref")
    parser.add_argument("--event-name", default="local", choices=("local", "push", "pull_request", "workflow_dispatch"))
    args = parser.parse_args()
    try:
        errors, count = check(args.repo_root, args.base_ref, args.event_name)
    except (RuntimeError, OSError, ValueError) as error:
        print(f"FAIL Markdown prose setup: {error}")
        return 1
    print("FAIL edited Markdown prose" if errors else f"PASS edited Markdown prose ({count} changed documents checked)")
    for error in errors:
        print("  - " + error)
    return int(bool(errors))


if __name__ == "__main__":
    raise SystemExit(main())
