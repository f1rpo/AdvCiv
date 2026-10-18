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

# <!-- custom: Import shared Git/base resolution from lib, keeping Markdown checks independent of the source-provenance scanner; reuse the existing prose transformer for sentence-boundary decisions. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))
import git_context

# <!-- custom: Add the repository-local helper folder before importing the prose transformer. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[4] / "LLM_Helpers"))
import reflow_markdown_prose as prose

ROOT = Path(__file__).resolve().parents[4]


# <!-- custom: Follow the maintained Markdown scope used by markdownlint, excluding copied/generated context and published numbered changelogs while retaining their maintained README indexes. (GPT-6.1-Sol) -->
def maintained(path):
    path = path.replace("\\", "/")
    if not path.lower().endswith(".md"):
        return False
    excluded = ("_0_Common_Docs/", "_SNAPSHOT_CONTEXT/", "LLM_Helpers/outputs/", "_1_AdvCiv-SAS/Docs/examples/", "LLM_Helpers/context/")
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


# <!-- custom: Detect apparent indented lists that Markdown merges into the preceding paragraph, independently of edit history.
# The README teaser reproduced that rendering failure: tabs before its bullets plus no blank line produced one paragraph and zero list items.
# Ordinary soft wrapping, valid nested lists and code examples remain allowed. (GPT-6.1-Sol) -->
def list_findings(path, text):
    physical = text.splitlines()
    errors = []
    for lines, _body in prose_blocks(text):
        first, last = lines[0], lines[-1]
        if prose.LIST_PREFIX_RE.match(physical[first]) or physical[first][:1].isspace():
            continue
        following = last + 1
        if following < len(physical) and re.match(r"^(?:\t| {4})[ \t]*(?:[-+*]|\d+[.)])\s+", physical[following]):
            errors.append(f"{path}:{following + 1}: indented list directly after prose renders as paragraph text; insert a blank line and unindent top-level bullets")
    return errors


# <!-- custom: Paragraph grouping remains an incremental editorial rule; untouched historical prose does not need reworking just because a different section changed. (GPT-6.1-Sol) -->
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


# <!-- custom: Always scan tracked Markdown for the concrete list-rendering defect; the earlier changed-files-only scope missed four older instances.
# Prose-length review still uses the push/PR base and changed-line hunks, avoiding a broad historical rewrite. (GPT-6.1-Sol) -->
def check(repo, base_ref, event_name):
    errors = []
    tracked = git_context.run_git(repo, "ls-files", "-z").stdout.decode("utf-8").split("\0")
    for path in tracked:
        current = repo / path
        if path.lower().endswith(".md") and current.is_file():
            _encoding, text = prose.decode_bytes(current.read_bytes())
            errors.extend(list_findings(path, text))
    base = git_context.ensure_base(repo, base_ref, event_name)
    if base is None:
        return errors, 0
    raw = git_context.run_git(repo, "diff", "--name-only", "-z", "--diff-filter=ACMRT", base, "--").stdout
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


# <!-- custom: Report global tracked-list failures and incremental prose findings without rewriting Markdown; grouping distinct thoughts still needs human review. (GPT-6.1-Sol) -->
def main():
    parser = argparse.ArgumentParser(description="Check all tracked Markdown list layouts and long prose blocks on added/edited lines.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--base-ref")
    parser.add_argument("--event-name", default="local", choices=("local", "push", "pull_request", "workflow_dispatch"))
    args = parser.parse_args()
    try:
        errors, count = check(args.repo_root, args.base_ref, args.event_name)
    except (RuntimeError, OSError, ValueError) as error:
        print(f"FAIL Markdown prose setup: {error}")
        return 1
    print("FAIL Markdown list layout/edited prose" if errors else f"PASS Markdown list layout/edited prose (all tracked Markdown lists; {count} changed documents checked for prose)")
    for error in errors:
        print("  - " + error)
    return int(bool(errors))


if __name__ == "__main__":
    raise SystemExit(main())
