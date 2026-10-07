#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Conservatively reflow C++ logging calls: wrap long BBAI/SASGameRecord calls and optionally collapse short ones.
# Keeps the log format string unchanged and wraps only at top-level commas between arguments.
# Commented-out calls, preprocessor directives/macros, and calls containing comments/multiline argument expressions are skipped.
# Existing prose-comment layout is outside this helper's scope and is never reflowed.
# Line endings are preserved. C++ token and preprocessor-block checks refuse writes if the rewrite changes source semantics.

from __future__ import annotations

import argparse
import difflib
import re
from pathlib import Path
from typing import Iterable, Iterator, List, Sequence, Tuple

LOG_FUNCTIONS = ("logBBAI", "logSASGameRecord")
SHORT_COLLAPSE_FUNCTIONS = LOG_FUNCTIONS + ("log",)
CPP_SUFFIXES = {".cpp", ".h"}


def _find_matching_paren(text: str, open_pos: int) -> int | None:
    depth = 0
    i = open_pos
    state = "code"
    quote = ""
    while i < len(text):
        c = text[i]
        n = text[i + 1] if i + 1 < len(text) else ""
        if state == "code":
            if c in ('"', "'"):
                state, quote = "string", c
            elif c == "/" and n == "/":
                state = "line_comment"
                i += 1
            elif c == "/" and n == "*":
                state = "block_comment"
                i += 1
            elif c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
                if depth == 0:
                    return i
        elif state == "string":
            if c == "\\":
                i += 1
            elif c == quote:
                state = "code"
        elif state == "line_comment":
            if c == "\n":
                state = "code"
        elif state == "block_comment":
            if c == "*" and n == "/":
                state = "code"
                i += 1
        i += 1
    return None


def _split_top_level_args(text: str) -> List[str]:
    args: List[str] = []
    start = 0
    paren = bracket = brace = 0
    i = 0
    state = "code"
    quote = ""
    while i < len(text):
        c = text[i]
        n = text[i + 1] if i + 1 < len(text) else ""
        if state == "code":
            if c in ('"', "'"):
                state, quote = "string", c
            elif c == "/" and n == "/":
                state = "line_comment"
                i += 1
            elif c == "/" and n == "*":
                state = "block_comment"
                i += 1
            elif c == "(":
                paren += 1
            elif c == ")":
                paren -= 1
            elif c == "[":
                bracket += 1
            elif c == "]":
                bracket -= 1
            elif c == "{":
                brace += 1
            elif c == "}":
                brace -= 1
            elif c == "," and paren == bracket == brace == 0:
                args.append(text[start:i])
                start = i + 1
        elif state == "string":
            if c == "\\":
                i += 1
            elif c == quote:
                state = "code"
        elif state == "line_comment":
            if c == "\n":
                state = "code"
        elif state == "block_comment":
            if c == "*" and n == "/":
                state = "code"
                i += 1
        i += 1
    args.append(text[start:])
    return args


def _visual_len(text: str) -> int:
    return len(text.expandtabs(4))


def _preprocessor_line_starts(text: str) -> set[int]:
    """Physical line starts belonging to #directives, including continued macro lines."""
    starts: set[int] = set()
    line_start = 0
    in_directive = False
    for line in text.splitlines(True):
        physical = line.rstrip("\r\n")
        if not in_directive and physical.lstrip().startswith("#"):
            in_directive = True
        if in_directive:
            starts.add(line_start)
        continued = in_directive and physical.rstrip(" \t").endswith("\\")
        if in_directive and not continued:
            in_directive = False
        line_start += len(line)
    return starts


def _preprocessor_blocks(text: str) -> Tuple[str, ...]:
    """Exact physical #directive blocks; line-splicing backslash placement is semantic."""
    blocks: List[str] = []
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i]
        if line.lstrip().startswith("#"):
            block = [line]
            while block[-1].rstrip(" \t").endswith("\\") and i + 1 < len(lines):
                i += 1
                block.append(lines[i])
            blocks.append("\n".join(block))
        i += 1
    return tuple(blocks)


def _iter_active_calls(text: str, function_names: Sequence[str] = LOG_FUNCTIONS) -> Iterator[Tuple[int, str, int, int]]:
    """Yield (name_start, function_name, open_paren, close_paren) in code only."""
    i = 0
    state = "code"
    quote = ""
    while i < len(text):
        c = text[i]
        n = text[i + 1] if i + 1 < len(text) else ""
        if state == "code":
            if c in ('"', "'"):
                state, quote = "string", c
            elif c == "/" and n == "/":
                state = "line_comment"
                i += 1
            elif c == "/" and n == "*":
                state = "block_comment"
                i += 1
            elif c == "_" or c.isalpha():
                j = i + 1
                while j < len(text) and (text[j] == "_" or text[j].isalnum()):
                    j += 1
                name = text[i:j]
                if name in function_names and j < len(text) and text[j] == "(":
                    close = _find_matching_paren(text, j)
                    if close is not None:
                        yield i, name, j, close
                        i = close
                else:
                    i = j - 1
        elif state == "string":
            if c == "\\":
                i += 1
            elif c == quote:
                state = "code"
        elif state == "line_comment":
            if c == "\n":
                state = "code"
        elif state == "block_comment":
            if c == "*" and n == "/":
                state = "code"
                i += 1
        i += 1


def _cpp_tokens_without_comments(text: str) -> Tuple[str, ...]:
    """Small lexical guard suitable for whitespace-only source rewrites."""
    tokens: List[str] = []
    i = 0
    while i < len(text):
        c = text[i]
        n = text[i + 1] if i + 1 < len(text) else ""
        if c.isspace():
            i += 1
            continue
        if c == "/" and n == "/":
            i = text.find("\n", i + 2)
            if i < 0:
                break
            continue
        if c == "/" and n == "*":
            end = text.find("*/", i + 2)
            i = len(text) if end < 0 else end + 2
            continue
        if c in ('"', "'"):
            quote = c
            j = i + 1
            while j < len(text):
                if text[j] == "\\":
                    j += 2
                    continue
                if text[j] == quote:
                    j += 1
                    break
                j += 1
            tokens.append(text[i:j])
            i = j
            continue
        if c == "_" or c.isalpha():
            j = i + 1
            while j < len(text) and (text[j] == "_" or text[j].isalnum()):
                j += 1
            tokens.append(text[i:j])
            i = j
            continue
        if c.isdigit():
            j = i + 1
            while j < len(text) and (text[j].isalnum() or text[j] in "._"):
                j += 1
            tokens.append(text[i:j])
            i = j
            continue
        matched = False
        for op in (">>=", "<<=", "->*", "...", "::", "->", "++", "--", "&&", "||", "==", "!=", "<=", ">=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<", ">>"):
            if text.startswith(op, i):
                tokens.append(op)
                i += len(op)
                matched = True
                break
        if not matched:
            tokens.append(c)
            i += 1
    return tuple(tokens)


def reflow_text(text: str, trigger_width: int = 180, wrap_width: int = 140, collapse_short: bool = False, collapse_width: int = 180) -> Tuple[str, int]:
    changes: List[Tuple[int, int, str]] = []
    preprocessor_lines = _preprocessor_line_starts(text)
    functions = SHORT_COLLAPSE_FUNCTIONS if collapse_short else LOG_FUNCTIONS
    for start, name, open_pos, close_pos in _iter_active_calls(text, functions):
        line_start = text.rfind("\n", 0, start) + 1
        # Preprocessor line-splicing backslashes are semantic before C++ tokenization.
        # Keep all #directive/macro bodies byte-for-byte outside this formatter's scope.
        if line_start in preprocessor_lines:
            continue
        args = _split_top_level_args(text[open_pos + 1:close_pos])
        if len(args) < 1:
            continue
        # BBAI/SASGameRecord calls conventionally start with a format string. For
        # generic member `.log(...)` calls, require that too before touching them.
        if not args[0].lstrip().startswith('"'):
            continue
        # Don't rewrite a call whose individual non-format argument is itself a
        # multiline/commented expression. Whitespace-only line breaks are fine.
        if any("\n" in arg.strip() or "//" in arg or "/*" in arg for arg in args[1:]):
            continue
        line_end = text.find("\n", close_pos)
        if line_end < 0:
            line_end = len(text)
        call_text = text[start:close_pos + 1]
        call_is_multiline = "\n" in call_text

        if collapse_short and call_is_multiline:
            single_call = name + "(" + ", ".join(arg.strip() for arg in args) + ")"
            candidate_line = text[line_start:start] + single_call + text[close_pos + 1:line_end]
            if "//" not in call_text and "/*" not in call_text and _visual_len(candidate_line) <= collapse_width:
                changes.append((start, close_pos + 1, single_call))
                continue

        # The wrapping mode remains intentionally limited to the original
        # BBAI/SASGameRecord functions; generic `.log` is opt-in collapse-only.
        # A one-argument format-only call has nothing to wrap.
        if name not in LOG_FUNCTIONS or len(args) < 2:
            continue
        lines = text[line_start:line_end].splitlines()
        if len(lines) == 1:
            too_long = _visual_len(lines[0]) > trigger_width
        else:
            # A very long format string is allowed. Trigger only on long
            # argument/continuation lines once the call is already multiline.
            too_long = any(_visual_len(line) > trigger_width for line in lines[1:])
        if not too_long:
            continue

        indent = re.match(r"[ \t]*", text[line_start:start]).group(0)  # type: ignore[union-attr]
        continuation = indent + "\t"
        remaining = [arg.strip() for arg in args[1:]]
        groups: List[str] = []
        current = ""
        for arg in remaining:
            candidate = arg if not current else current + ", " + arg
            if current and _visual_len(continuation + candidate) > wrap_width:
                groups.append(current)
                current = arg
            else:
                current = candidate
        if current:
            groups.append(current)

        replacement = name + "(" + args[0].strip() + ",\n"
        replacement += ",\n".join(continuation + group for group in groups)
        replacement += ")"
        if replacement != text[start:close_pos + 1]:
            changes.append((start, close_pos + 1, replacement))

    for start, end, replacement in reversed(changes):
        text = text[:start] + replacement + text[end:]
    return text, len(changes)


def _read_preserving_eol(path: Path) -> Tuple[str, str]:
    raw = path.read_bytes()
    crlf = raw.count(b"\r\n")
    lf = raw.count(b"\n")
    eol = "\r\n" if crlf and crlf == lf else "\n"
    return raw.decode("utf-8", errors="surrogateescape").replace("\r\n", "\n"), eol


def _iter_files(paths: Sequence[Path]) -> Iterable[Path]:
    seen = set()
    for path in paths:
        if path.is_file() and path.suffix.lower() in CPP_SUFFIXES:
            candidates = (path,)
        elif path.is_dir():
            candidates = tuple(p for p in path.rglob("*") if p.is_file() and p.suffix.lower() in CPP_SUFFIXES)
        else:
            candidates = ()
        for candidate in candidates:
            resolved = candidate.resolve()
            if resolved not in seen:
                seen.add(resolved)
                yield candidate


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="+", type=Path, help="C++ file(s) or directories to scan recursively")
    parser.add_argument("--in-place", action="store_true", help="write verified changes")
    parser.add_argument("--diff", action="store_true", help="print unified diff")
    parser.add_argument("--trigger-width", type=int, default=180, help="line width that triggers wrapping (default: 180)")
    parser.add_argument("--wrap-width", type=int, default=140, help="target continuation width (default: 140)")
    parser.add_argument("--collapse-short", action="store_true", help="also collapse simple multiline log calls that fit on one line")
    parser.add_argument("--collapse-width", type=int, default=180, help="maximum full physical line width for --collapse-short (default: 180)")
    args = parser.parse_args()

    total = changed_files = 0
    for path in _iter_files(args.paths):
        old, eol = _read_preserving_eol(path)
        new, count = reflow_text(old, args.trigger_width, args.wrap_width, args.collapse_short, args.collapse_width)
        if not count:
            continue
        if _preprocessor_blocks(old) != _preprocessor_blocks(new):
            raise SystemExit("refusing to write: preprocessor directive/macro block changed in %s" % path)
        if _cpp_tokens_without_comments(old) != _cpp_tokens_without_comments(new):
            raise SystemExit("refusing to write: significant C++ token sequence changed in %s" % path)
        changed_files += 1
        total += count
        print("%s: %d logging call(s)" % (path, count))
        if args.diff:
            print("".join(difflib.unified_diff(old.splitlines(True), new.splitlines(True), fromfile=str(path), tofile=str(path))))
        if args.in_place:
            path.write_bytes(new.replace("\n", eol).encode("utf-8", errors="surrogateescape"))

    print("%d logging call(s) across %d file(s)" % (total, changed_files))
    if not args.in_place and changed_files:
        print("dry run only; pass --in-place to write changes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
