#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Conservative Markdown prose reflow helper.
#
# This helper does not rewrite prose or wrap to an arbitrary width.
# It only inserts source line breaks at conservative sentence boundaries on already-long prose lines.
# Existing physical line breaks are preserved; lines are never joined.
#
# Intended result:
#
#   - **Feature:** First sentence. Second sentence. Third sentence.
#
# becomes:
#
#   - **Feature:** First sentence.
#     Second sentence.
#     Third sentence.
#
# Markdown normally renders these source newlines as spaces, so the visible paragraph/list item stays unchanged.
# Source diffs and LLM edits can still address one sentence at a time.
#
# Usage from mod root:
#   python LLM_Helpers\reflow_markdown_prose.py _1_AdvCiv-SAS\Docs\README_Known_Issues.md --diff
#   python LLM_Helpers\reflow_markdown_prose.py _1_AdvCiv-SAS\Docs\README_Known_Issues.md --in-place
#   python LLM_Helpers\reflow_markdown_prose.py _1_AdvCiv-SAS\Docs\README_Main_Changes_Guide.md --in-place
#
# Review the diff before committing.
# Semantic visible structure remains a human/LLM job: use a blank line for a real new paragraph, or a nested sub-bullet for a distinct child point.

from __future__ import print_function

import argparse
import difflib
import re
import sys
from pathlib import Path

DEFAULT_MIN_LINE_LEN = 320

# Conservative false-positive protection. Missing a possible split is preferable to splitting an abbreviation incorrectly.
ABBREVIATION_SUFFIXES = (
    "e.g.",
    "i.e.",
    "etc.",
    "vs.",
    "cf.",
    "approx.",
    "incl.",
    "excl.",
    "mr.",
    "mrs.",
    "ms.",
    "dr.",
    "prof.",
    "no.",
    "fig.",
    "jr.",
    "sr.",
)

LIST_PREFIX_RE = re.compile(r"^(\s*(?:[-*+]|\d+[.)])\s+)(.*)$")
PLAIN_PREFIX_RE = re.compile(r"^(\s{0,3})(.*)$")
LINK_DEST_RE = re.compile(r"\]\((?:[^()\\]|\\.|(?:\([^()]*\)))*\)")
URL_RE = re.compile(r"https?://\S+")


def decode_bytes(raw):
    if raw.startswith(b"\xef\xbb\xbf"):
        return "utf-8-sig", raw.decode("utf-8-sig")
    return "utf-8", raw.decode("utf-8")


def line_ending_from_bytes(raw):
    return "\r\n" if b"\r\n" in raw else "\n"


def protected_mask(text):
    mask = [False] * len(text)

    # Inline code spans. Handle one-or-more backtick delimiters conservatively.
    i = 0
    while i < len(text):
        if text[i] != "`":
            i += 1
            continue
        j = i
        while j < len(text) and text[j] == "`":
            j += 1
        delim = text[i:j]
        end = text.find(delim, j)
        if end < 0:
            i = j
            continue
        for pos in range(i, end + len(delim)):
            mask[pos] = True
        i = end + len(delim)

    # Link destinations and raw URLs can contain sentence-like punctuation.
    for match in LINK_DEST_RE.finditer(text):
        start, end = match.span()
        for pos in range(start + 1, end):
            mask[pos] = True
    for match in URL_RE.finditer(text):
        start, end = match.span()
        for pos in range(start, end):
            mask[pos] = True

    return mask


def sentence_break_starts(text):
    """Return starts of later sentences where a soft line break is safe enough."""
    mask = protected_mask(text)
    result = []
    n = len(text)

    for i, char in enumerate(text):
        if char not in ".?!" or mask[i]:
            continue

        if char == ".":
            # Decimal/version fragments such as 1.14 or 0.25.
            if i > 0 and i + 1 < n and text[i - 1].isdigit() and text[i + 1].isdigit():
                continue

            lower_prefix = text[:i + 1].lower()
            if any(lower_prefix.endswith(suffix) for suffix in ABBREVIATION_SUFFIXES):
                continue

            # Initials such as "A." are too ambiguous to split automatically.
            word_match = re.search(r"([A-Za-z]+)\.$", text[:i + 1])
            if word_match and len(word_match.group(1)) == 1:
                continue

        j = i + 1
        # Sentence punctuation is often followed by closing emphasis/quotes.
        while j < n and text[j] in ')"\'’]}*_':
            j += 1

        if j >= n or not text[j].isspace():
            continue

        k = j
        while k < n and text[k].isspace():
            k += 1
        if k >= n:
            continue

        # Requiring an obvious new-sentence starter is deliberately conservative.
        starter = text[k]
        if starter.isupper() or starter.isdigit() or starter in '*_`[("“\'‘':
            result.append(k)

    return result


def looks_risky(line, in_fence):
    if in_fence:
        return True

    stripped = line.lstrip()
    if not stripped:
        return True
    if stripped.startswith(("#", ">", "|", "<", "<!--")):
        return True
    if line.endswith("  ") or line.endswith("\\"):
        return True

    # Avoid indented code blocks. Nested list items are handled separately below.
    if re.match(r"^\s{4,}\S", line) and not re.match(r"^\s{2,}(?:[-*+]|\d+[.)])\s+", line):
        return True

    # Common table-ish/log/data lines where punctuation does not mean prose.
    if line.count("|") >= 2:
        return True
    if stripped.startswith(("GAME_RECORD_", "BBAI_", "{", "}", "[", "]")):
        return True

    return False


def split_prose_line(line, min_line_len):
    if len(line) < min_line_len:
        return [line]

    list_match = LIST_PREFIX_RE.match(line)
    if list_match:
        prefix = list_match.group(1)
        body = list_match.group(2)
        continuation = " " * len(prefix.expandtabs(4))
    else:
        plain_match = PLAIN_PREFIX_RE.match(line)
        if not plain_match:
            return [line]
        prefix = plain_match.group(1)
        body = plain_match.group(2)
        continuation = prefix

    breaks = sentence_break_starts(body)
    if not breaks:
        return [line]

    parts = []
    last = 0
    for start in breaks:
        parts.append(body[last:start].rstrip())
        last = start
    parts.append(body[last:].rstrip())

    if len(parts) < 2 or any(not part for part in parts):
        return [line]

    # Safety invariant: only spaces at chosen sentence boundaries are replaced.
    if " ".join(parts) != body:
        return [line]

    return [prefix + parts[0]] + [continuation + part for part in parts[1:]]


def transform_text(text, newline, min_line_len):
    source_lines = text.splitlines()
    output_lines = []
    changed_lines = 0
    inserted_breaks = 0
    in_fence = False
    fence_char = None
    fence_len = 0

    for line in source_lines:
        stripped = line.lstrip()

        fence_match = re.match(r"^(`{3,}|~{3,})", stripped)
        if fence_match:
            marker = fence_match.group(1)
            if not in_fence:
                in_fence = True
                fence_char = marker[0]
                fence_len = len(marker)
            elif marker[0] == fence_char and len(marker) >= fence_len:
                in_fence = False
                fence_char = None
                fence_len = 0
            output_lines.append(line)
            continue

        if looks_risky(line, in_fence):
            output_lines.append(line)
            continue

        split_lines = split_prose_line(line, min_line_len)
        output_lines.extend(split_lines)
        if len(split_lines) > 1:
            changed_lines += 1
            inserted_breaks += len(split_lines) - 1

    # splitlines() drops the final line ending; preserve whether one existed.
    result = newline.join(output_lines)
    if text.endswith(("\r\n", "\n")):
        result += newline

    return result, changed_lines, inserted_breaks


def expand_paths(path_args):
    result = []
    for arg in path_args:
        path = Path(arg)
        if path.is_dir():
            result.extend(sorted(path.rglob("*.md")))
        else:
            result.append(path)
    return result


def main(argv):
    parser = argparse.ArgumentParser(
        description="Insert soft Markdown line breaks at conservative sentence boundaries on long prose lines."
    )
    parser.add_argument("paths", nargs="+", help="Markdown file(s) or directories to process")
    parser.add_argument("--in-place", action="store_true", help="rewrite changed input files")
    parser.add_argument("--diff", action="store_true", help="print unified diff")
    parser.add_argument(
        "--check",
        action="store_true",
        help="return nonzero if the helper would change any file (manual/reporting use; not intended as a strict CI style gate)",
    )
    parser.add_argument("--min-line-len", type=int, default=DEFAULT_MIN_LINE_LEN)
    args = parser.parse_args(argv)

    if args.min_line_len < 80:
        print("ERROR: --min-line-len must be at least 80.", file=sys.stderr)
        return 2

    paths = expand_paths(args.paths)
    total_changed_files = 0
    total_changed_lines = 0
    total_inserted_breaks = 0

    for path in paths:
        if path.suffix.lower() != ".md":
            continue
        raw = path.read_bytes()
        encoding, text = decode_bytes(raw)
        newline = line_ending_from_bytes(raw)

        new_text, changed_lines, inserted_breaks = transform_text(
            text, newline, args.min_line_len
        )
        if new_text == text:
            continue

        total_changed_files += 1
        total_changed_lines += changed_lines
        total_inserted_breaks += inserted_breaks

        if args.diff:
            diff = difflib.unified_diff(
                text.splitlines(True),
                new_text.splitlines(True),
                fromfile=str(path),
                tofile=str(path) + ".reflowed",
            )
            sys.stdout.write("".join(diff))

        if args.in_place:
            write_encoding = "utf-8" if encoding == "utf-8-sig" else encoding
            path.write_bytes(new_text.encode(write_encoding))

        print(
            "%s: changed_long_lines=%d inserted_soft_breaks=%d"
            % (path, changed_lines, inserted_breaks)
        )

    print("changed_files=%d" % total_changed_files)
    print("changed_long_lines=%d" % total_changed_lines)
    print("inserted_soft_breaks=%d" % total_inserted_breaks)
    print("min_line_len=%d" % args.min_line_len)

    if not args.in_place:
        print("dry_run_only=true")
    if args.check and total_changed_files:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
