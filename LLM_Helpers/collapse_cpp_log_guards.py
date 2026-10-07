#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Conservative C++ logging-guard cleanup helper.
#
# Collapses only simple `if` / `else if` guards whose condition contains a recognizable logging pre-gate and whose sole body statement is a logging call.
# Semantic-only conditions such as `isNormalizing()`, `bCoastal`, or `isDebug()` are intentionally skipped even when their body logs; an enclosing block may already perform the real logging pre-gate, and compacting those semantic branches would create unrelated cosmetic churn.
# Eligible guards include local `bLog...` flags, BBAI/SAS log-level expressions such as `gGameRecordLogLevel >= 3`, and explicit logging predicates such as `GC.isLogging()`. For example:
#   if (gGameRecordLogLevel >= 3)
#       logThing(...);
#   if (bLogSomething)
#   {
#       logThing(...);
#   }
# becomes:
#   if (gGameRecordLogLevel >= 3) logThing(...);
#   if (bLogSomething) logThing(...);
#
# Multiline logging calls stay multiline; this helper only joins the guard to the first call line, removes a now-unnecessary one-statement brace pair, and shifts continuation indentation left by the removed body indent.
# Safe trailing `//` comments are preserved; block comments/macros/uncertain calls are skipped.
# Review the diff before committing.

from __future__ import annotations

import argparse
from contextlib import nullcontext
from datetime import datetime, timezone
import difflib
from pathlib import Path
import re
import sys

from collapse_cpp_signatures import (
    CPP_SUFFIXES,
    decode_bytes,
    display_path,
    get_default_repo_root,
    iter_candidate_files,
    leading_ws,
    split_line_ending,
)

LOG_GATE_RE = re.compile(
    r"(?:\bg[A-Za-z0-9_]*LogLevel\b|\bi[A-Za-z0-9_]*LogLevel\b|\bbLog[A-Za-z0-9_]*\b|"
    r"\bgLogBBAI\b|\bg_bSASGameRecord[A-Za-z0-9_]*\b|\bgetSASGameRecordLogLevel\s*\(|"
    r"\bGC\.isLogging\s*\(|\bisSAS(?:BBAI|GameRecord)[A-Za-z0-9_]*Log[A-Za-z0-9_]*Enabled\s*\(|"
    r"\bisFound(?:Value)?Log(?:ging)?Enabled\s*\()"
)

class Rewrite:
    def __init__(self, start: int, end: int, lines: list[str], removed_braces: bool):
        self.start = start
        self.end = end
        self.lines = lines
        self.removed_braces = removed_braces


class Stats:
    def __init__(self):
        self.files_scanned = 0
        self.files_changed = 0
        self.guards = 0
        self.braced_guards = 0
        self.unbraced_guards = 0


def default_diff_file_path(repo_root: Path, timestamp: str) -> Path:
    return repo_root / "LLM_Helpers" / "outputs" / f"collapse_cpp_log_guards_{timestamp}.diff.txt"


def resolve_output_file_path(repo_root: Path, value: str | None, default_path: Path) -> Path | None:
    if value is None:
        return None
    if value == "auto":
        return default_path
    path = Path(value)
    return path if path.is_absolute() else repo_root / path


def split_cpp_line_comment(text: str) -> tuple[str, str | None]:
    """Split a trailing // comment outside string/char literals."""
    quote = ""
    i = 0
    while i + 1 < len(text):
        ch = text[i]
        if quote:
            if ch == "\\":
                i += 2
                continue
            if ch == quote:
                quote = ""
        else:
            if ch in ('"', "'"):
                quote = ch
            elif ch == "/" and text[i + 1] == "/":
                return text[:i], text[i:]
        i += 1
    return text, None


def has_block_comment_or_directive(text: str) -> bool:
    stripped = text.lstrip()
    return stripped.startswith("#") or "/*" in text or "*/" in text


def scan_matching_paren(text: str, open_pos: int) -> int | None:
    depth = 0
    quote = ""
    i = open_pos
    while i < len(text):
        ch = text[i]
        if quote:
            if ch == "\\":
                i += 2
                continue
            if ch == quote:
                quote = ""
        else:
            if ch in ('"', "'"):
                quote = ch
            elif ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
                if depth == 0:
                    return i
        i += 1
    return None


def simple_control_header(text: str) -> tuple[str, str, str] | None:
    """Return (indent, exact header text, condition) for a plain `if (...)` or `else if (...)`."""
    if has_block_comment_or_directive(text):
        return None
    code, comment = split_cpp_line_comment(text)
    if comment is not None:
        return None
    indent = leading_ws(code)
    stripped = code[len(indent):].rstrip()
    match = re.match(r"(?:else\s+)?if\b", stripped)
    if match is None:
        return None
    open_pos = stripped.find("(", match.end())
    if open_pos < 0:
        return None
    close_pos = scan_matching_paren(stripped, open_pos)
    if close_pos is None or stripped[close_pos + 1 :].strip():
        return None
    condition = stripped[open_pos + 1 : close_pos]
    return indent, stripped, condition


def log_like_callee(statement_start: str) -> bool:
    """Accept ordinary call expressions whose final callable identifier is log-like."""
    if has_block_comment_or_directive(statement_start):
        return False
    code, _comment = split_cpp_line_comment(statement_start)
    stripped = code.strip()
    open_pos = stripped.find("(")
    if open_pos <= 0:
        return False
    callee = stripped[:open_pos].strip()
    # Keep this helper away from assignments, returns and arbitrary expressions.
    if any(token in callee for token in ("=", "+", "-", "*", "/", "?", "!", "<", ">", "&", "|", ",", ";", "[", "]", "{")):
        # `->` is the one '-'/'>' combination allowed for a member call.
        cleaned = callee.replace("->", "").replace("::", "").replace(".", "")
        if re.search(r"[^A-Za-z0-9_]", cleaned):
            return False
    final_match = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*$", callee)
    if final_match is None:
        return False
    name = final_match.group(1)
    # Avoid macro-like ALL_CAPS logging wrappers; brace removal around an unsafe
    # multi-statement macro could change semantics. Ordinary log helpers are fine.
    if name.upper() == name and any(ch.isalpha() for ch in name):
        return False
    return (
        name == "log"
        or (name.startswith("log") and (len(name) == 3 or name[3].isupper() or name[3] == "_"))
        or name.startswith("myLog")
        or re.search(r"(?:^|_)log(?:_|[A-Z0-9])", name) is not None
        or name.endswith("Log")
    )


def statement_end(lines: list[str], start: int) -> int | None:
    """Return final line of one plain logging call statement, otherwise None."""
    if start >= len(lines):
        return None
    first = lines[start]
    if not log_like_callee(first):
        return None
    text_parts: list[str] = []
    for index in range(start, min(len(lines), start + 80)):
        body, _eol = split_line_ending(lines[index])
        if has_block_comment_or_directive(body) or "{" in body or "}" in body:
            return None
        code, comment = split_cpp_line_comment(body)
        text_parts.append(code)
        combined = "\n".join(text_parts)
        stripped = combined.strip()
        open_pos = stripped.find("(")
        if open_pos < 0:
            return None
        close_pos = scan_matching_paren(stripped, open_pos)
        if close_pos is None:
            if comment is not None:
                return None
            continue
        tail = stripped[close_pos + 1 :].strip()
        if tail != ";":
            return None
        return index
    return None


def deindent_statement_lines(lines: list[str], start: int, end: int, header_indent: str) -> list[str] | None:
    first_body, first_eol = split_line_ending(lines[start])
    body_indent = leading_ws(first_body)
    if not body_indent.startswith(header_indent) or len(body_indent) <= len(header_indent):
        return None
    indent_delta = body_indent[len(header_indent):]
    if not indent_delta:
        return None
    result: list[str] = []
    for index in range(start, end + 1):
        body, eol = split_line_ending(lines[index])
        if index == start:
            result.append(body[len(body_indent):] + first_eol)
            continue
        prefix = header_indent + indent_delta
        if body.strip() and not body.startswith(prefix):
            return None
        if body.startswith(prefix):
            body = header_indent + body[len(prefix):]
        result.append(body + eol)

    # A braced/originally nested call can already have a continuation tail that
    # sits two or more levels below the log-call head. Once the guard and call
    # head share a line, keep the shallowest continuation exactly one body
    # indent deeper than the guard and preserve any extra relative nesting.
    # This avoids leaving visually over-indented tails after brace/body removal.
    continuation_indices = [i for i in range(1, len(result)) if split_line_ending(result[i])[0].strip()]
    if continuation_indices:
        desired_prefix = header_indent + indent_delta
        continuation_ws = [leading_ws(split_line_ending(result[i])[0]) for i in continuation_indices]
        if all(ws.startswith(desired_prefix) for ws in continuation_ws):
            shortest = min(continuation_ws, key=len)
            common_extra = shortest[len(desired_prefix):]
            if common_extra and all(ws.startswith(desired_prefix + common_extra) for ws in continuation_ws):
                for i in continuation_indices:
                    body, eol = split_line_ending(result[i])
                    result[i] = desired_prefix + body[len(desired_prefix + common_extra):] + eol
    return result


def try_rewrite(lines: list[str], index: int) -> Rewrite | None:
    header_body, header_eol = split_line_ending(lines[index])
    header = simple_control_header(header_body)
    if header is None or index + 1 >= len(lines):
        return None
    header_indent, header_text, condition = header
    # Formatting this helper is intentionally narrower than "any conditional whose body logs":
    # only an explicit logging pre-gate qualifies. Semantic-only branches may sit inside an
    # already-gated logging block and are left author-maintained.
    if LOG_GATE_RE.search(condition) is None:
        return None

    next_body, _next_eol = split_line_ending(lines[index + 1])
    braced = next_body.strip() == "{" and leading_ws(next_body) == header_indent
    statement_start = index + 2 if braced else index + 1
    end = statement_end(lines, statement_start)
    if end is None:
        return None

    if braced:
        close_index = end + 1
        if close_index >= len(lines):
            return None
        close_body, _close_eol = split_line_ending(lines[close_index])
        if close_body.strip() != "}" or leading_ws(close_body) != header_indent:
            return None
        # Keep if/else layout author-maintained rather than changing brace style in
        # a compound conditional chain.
        if close_index + 1 < len(lines):
            following, _following_eol = split_line_ending(lines[close_index + 1])
            if following.lstrip().startswith("else"):
                return None
        rewrite_end = close_index
    else:
        rewrite_end = end

    shifted = deindent_statement_lines(lines, statement_start, end, header_indent)
    if shifted is None:
        return None
    first_shifted_body, _first_shifted_eol = split_line_ending(shifted[0])
    replacement = [header_indent + header_text + " " + first_shifted_body.lstrip(" \t") + header_eol]
    replacement.extend(shifted[1:])
    return Rewrite(index, rewrite_end, replacement, braced)


def rewrite_lines(lines: list[str]) -> tuple[list[str], Stats]:
    stats = Stats()
    new_lines: list[str] = []
    index = 0
    while index < len(lines):
        rewrite = try_rewrite(lines, index)
        if rewrite is None:
            new_lines.append(lines[index])
            index += 1
            continue
        new_lines.extend(rewrite.lines)
        stats.guards += 1
        if rewrite.removed_braces:
            stats.braced_guards += 1
        else:
            stats.unbraced_guards += 1
        index = rewrite.end + 1
    return new_lines, stats


def process_file(repo_root: Path, path: Path, args: argparse.Namespace, stats: Stats, diff_file) -> bool:
    if path.suffix.lower() not in CPP_SUFFIXES:
        return False
    raw = path.read_bytes()
    encoding, text = decode_bytes(raw)
    lines = text.splitlines(keepends=True)
    new_lines, file_stats = rewrite_lines(lines)
    stats.files_scanned += 1
    if file_stats.guards == 0:
        return False
    old_text = "".join(lines)
    new_text = "".join(new_lines)
    if old_text == new_text:
        return False
    stats.files_changed += 1
    stats.guards += file_stats.guards
    stats.braced_guards += file_stats.braced_guards
    stats.unbraced_guards += file_stats.unbraced_guards
    relative_path = path.relative_to(repo_root)
    if diff_file is not None:
        diff_file.writelines(
            difflib.unified_diff(
                lines,
                new_lines,
                fromfile=f"{relative_path.as_posix()} (before)",
                tofile=f"{relative_path.as_posix()} (after)",
                lineterm="\n",
            )
        )
    if args.in_place:
        path.write_text(new_text, encoding=encoding, newline="")
    return True


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Collapse simple C++ if / else-if guards around logging calls.")
    parser.add_argument("paths", nargs="*", type=Path, help="files or directories to scan; defaults to C/C++ under CvGameCoreDLL")
    parser.add_argument("--repo-root", type=Path, default=get_default_repo_root())
    parser.add_argument("--in-place", action="store_true", help="write changes instead of only reporting")
    parser.add_argument("--diff-file", nargs="?", const="auto", default=None, help="write review diff; without a path, writes a timestamped file under LLM_Helpers/outputs/")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    repo_root = args.repo_root.resolve()
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    diff_file_path = resolve_output_file_path(repo_root, args.diff_file, default_diff_file_path(repo_root, timestamp))
    if diff_file_path is not None:
        diff_file_path.parent.mkdir(parents=True, exist_ok=True)
    stats = Stats()
    changed: list[Path] = []
    diff_context = diff_file_path.open("w", encoding="utf-8", newline="") if diff_file_path is not None else nullcontext(None)
    with diff_context as diff_file:
        for path in iter_candidate_files(repo_root, args.paths):
            if process_file(repo_root, path, args, stats, diff_file):
                changed.append(path.relative_to(repo_root))
    print(f"Scanned {stats.files_scanned} C/C++ file(s).")
    if changed:
        verb = "Updated" if args.in_place else "Would update"
        print(
            f"{verb} {len(changed)} file(s): {stats.guards} log guard collapse(s) "
            f"({stats.unbraced_guards} unbraced joins, {stats.braced_guards} brace removals)."
        )
        for path in changed:
            print(f"  - {display_path(repo_root, repo_root / path)}")
        if diff_file_path is not None:
            print(f"Diff file written: {display_path(repo_root, diff_file_path)}")
        if not args.in_place:
            print("Run again with --in-place after reviewing the target list/diff.")
            return 1
    else:
        print("No simple logging guards to collapse.")
        if diff_file_path is not None and diff_file_path.exists() and diff_file_path.stat().st_size == 0:
            diff_file_path.unlink()
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
