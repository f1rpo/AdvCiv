#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Heuristic audit for C++ BBAI/SASGameRecord caller-side pre-gating.
#
# This intentionally reports review candidates rather than rewriting control flow: reordering `&&` operands can change side effects or prerequisite ordering, and a logging helper may be safely gated by an outer caller that a local text pass cannot prove.
# The audit does understand ordinary enclosing `if (bLog...) { ... }` blocks, so logging-only loops/setup behind a single outer gate are not treated as missing a per-call gate.
#
# Typical use:
#   python LLM_Helpers/audit_cpp_logging_pregates.py CvGameCoreDLL
#   python LLM_Helpers/audit_cpp_logging_pregates.py CvGameCoreDLL/CvCity.cpp --late-gates
#
# Review the output; do not turn this heuristic into blocking CI.

from __future__ import annotations

import argparse
from pathlib import Path
import re
from typing import Iterable

from reflow_cpp_logging_calls import _iter_active_calls

CPP_SUFFIXES = {".cpp"}
SKIP_IMPLEMENTATION_FILES = {"BBAILog.cpp", "SASGameRecordLog.cpp"}

LOG_CALL_RE = re.compile(
    r"\b(?:logBBAI|logSASGameRecord[A-Za-z0-9_]*|recordSASGameRecord[A-Za-z0-9_]*|noteSASGameRecord[A-Za-z0-9_]*|SAS_log[A-Za-z0-9_]*)\s*\("
)

LOG_HELPER_DEF_RE = re.compile(
    r"^\s*[A-Za-z_][A-Za-z0-9_:<>,*&\s]*\s+(?:(?:[A-Za-z_][A-Za-z0-9_]*::)+)?(?P<name>(?:(?:SAS_)?log|record|note)[A-Za-z0-9_]*)\s*\("
)

GATE_RE = re.compile(
    r"(?:\bg[A-Za-z0-9_]*LogLevel\b|\bgLogBBAI\b|\bg_bSASGameRecord[A-Za-z0-9_]*\b|\bi[A-Za-z0-9_]*LogLevel\b|"
    r"\bbLog[A-Za-z0-9_]*\b|\bgetSASGameRecordLogLevel\s*\(|"
    r"\bisSAS(?:BBAI|GameRecord)[A-Za-z0-9_]*Log[A-Za-z0-9_]*Enabled\s*\(|"
    r"\bisFound(?:Value)?Log(?:ging)?Enabled\s*\()"
)


def strip_cpp_comments_and_strings(line: str, in_block_comment: bool = False) -> tuple[str, bool]:
    """Blank strings/comments while preserving enough punctuation for the audit."""
    out = []
    i = 0
    quote = ""
    while i < len(line):
        ch = line[i]
        nxt = line[i + 1] if i + 1 < len(line) else ""
        if in_block_comment:
            end = line.find("*/", i)
            if end < 0:
                out.extend(" " * (len(line) - i))
                break
            out.extend(" " * (end + 2 - i))
            i = end + 2
            in_block_comment = False
            continue
        if quote:
            if ch == "\\":
                out.extend("  ")
                i += 2
                continue
            if ch == quote:
                quote = ""
            out.append(" ")
        else:
            if ch in ('"', "'"):
                quote = ch
                out.append(" ")
            elif ch == "/" and nxt == "/":
                out.extend(" " * (len(line) - i))
                break
            elif ch == "/" and nxt == "*":
                end = line.find("*/", i + 2)
                if end < 0:
                    out.extend(" " * (len(line) - i))
                    in_block_comment = True
                    break
                out.extend(" " * (end + 2 - i))
                i = end + 2
                continue
            else:
                out.append(ch)
        i += 1
    return "".join(out), in_block_comment


def extract_if_condition(code: str) -> str | None:
    match = re.search(r"\bif\s*\(", code)
    if match is None:
        return None
    open_pos = code.find("(", match.start())
    depth = 0
    for i in range(open_pos, len(code)):
        ch = code[i]
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                return code[open_pos + 1 : i]
    return None


def has_log_gate(condition: str | None) -> bool:
    return condition is not None and GATE_RE.search(condition) is not None


def log_gate_is_late(condition: str | None) -> bool:
    if condition is None:
        return False
    gate = GATE_RE.search(condition)
    if gate is None:
        return False
    return "&&" in condition[: gate.start()]


def iter_files(paths: list[str]) -> Iterable[Path]:
    seen: set[Path] = set()
    for raw in paths:
        path = Path(raw)
        if path.is_file():
            candidates = [path]
        elif path.is_dir():
            candidates = sorted(p for p in path.rglob("*") if p.is_file() and p.suffix.lower() in CPP_SUFFIXES)
        else:
            continue
        for candidate in candidates:
            resolved = candidate.resolve()
            if resolved not in seen:
                seen.add(resolved)
                yield candidate


def audit_file(path: Path, include_implementations: bool, report_late: bool) -> list[tuple[int, str, str]]:
    if not include_implementations and path.name in SKIP_IMPLEMENTATION_FILES:
        return []
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return []

    # Each brace scope stores whether an enclosing logging gate is already active.
    gated_stack: list[bool] = [False]
    helper_stack: list[bool] = [False]
    pending_if_gate: bool | None = None
    pending_log_helper = False
    findings: list[tuple[int, str, str]] = []
    in_block_comment = False

    for line_no, raw in enumerate(lines, 1):
        code, in_block_comment = strip_cpp_comments_and_strings(raw, in_block_comment)
        stripped = code.strip()

        # Pop leading closing braces before classifying the current statement.
        leading_closes = len(stripped) - len(stripped.lstrip("}"))
        for _ in range(leading_closes):
            if len(gated_stack) > 1:
                gated_stack.pop()
            if len(helper_stack) > 1:
                helper_stack.pop()
        if leading_closes:
            stripped = stripped[leading_closes:].lstrip()
            code = stripped

        condition = extract_if_condition(code)
        same_line_gate = has_log_gate(condition)
        enclosing_gate = gated_stack[-1]
        in_log_helper = helper_stack[-1]
        pending_statement_gate = (pending_if_gate is True and stripped != "{")
        helper_match = LOG_HELPER_DEF_RE.match(code)
        current_log_helper_definition = (helper_match is not None and not stripped.endswith(";"))

        if LOG_CALL_RE.search(code) and not in_log_helper and not current_log_helper_definition:
            # A long logging call often starts on the guarded line and ends several
            # physical lines later, so do not require the semicolon to be present on
            # this head line. Definition lines are filtered separately above.
            if not enclosing_gate and not same_line_gate and not pending_statement_gate:
                findings.append((line_no, "NO_OBVIOUS_PREGATE", raw.strip()))
            elif report_late and same_line_gate and log_gate_is_late(condition):
                findings.append((line_no, "LOG_GATE_NOT_FIRST", raw.strip()))

        if current_log_helper_definition and "{" not in code:
            pending_log_helper = True

        # Remember a standalone if condition for a following opening brace.
        if condition is not None and "{" not in code:
            pending_if_gate = enclosing_gate or same_line_gate
        elif stripped and stripped != "{" and not stripped.startswith("else"):
            # An unrelated statement breaks the pending one-line if association.
            # A standalone opening brace is the expected continuation of `if (...)`.
            pending_if_gate = None

        # Push braces after processing the statement. For an `if (...) {` block,
        # inherit the recognized logging gate; ordinary blocks inherit only the
        # already-active outer state. Braces inside aggregate initializers can make
        # this heuristic imperfect, which is why output is advisory only.
        opens = code.count("{")
        closes = max(0, code.count("}") - leading_closes)
        for open_index in range(opens):
            if open_index == 0 and condition is not None:
                gated_stack.append(enclosing_gate or same_line_gate)
            elif open_index == 0 and pending_if_gate is not None:
                gated_stack.append(pending_if_gate)
                pending_if_gate = None
            else:
                gated_stack.append(gated_stack[-1])

            if open_index == 0 and (current_log_helper_definition or pending_log_helper):
                helper_stack.append(True)
                pending_log_helper = False
            else:
                helper_stack.append(helper_stack[-1])
        for _ in range(closes):
            if len(gated_stack) > 1:
                gated_stack.pop()
            if len(helper_stack) > 1:
                helper_stack.pop()

    return findings


# <!-- custom: Guard/call formatters intentionally preserve message text and skip multi-call blocks.
# Audit active literal BBAI prefixes separately so prose leftovers are visible even when their gates are correct; report only, since selecting names or removing duplicates needs semantic review. (GPT-6.1-Sol) -->
def audit_message_style(text: str) -> list[tuple[int, str, str]]:
    findings = []
    for start, name, opening, _closing in _iter_active_calls(text):
        if name != "logBBAI":
            continue
        literal = re.match(r'\s*"((?:[^"\\]|\\.)*)"', text[opening + 1:])
        if literal is None:
            continue
        message = literal.group(1)
        event = message.lstrip()
        if not re.match(r"[A-Z][A-Z0-9]*(?:_[A-Z0-9]+)+(?=\s|$)", event):
            kind = "BBAI_UNSTRUCTURED_PREFIX"
        elif message != event:
            kind = "BBAI_INDENTED_PREFIX"
        else:
            continue
        findings.append((text.count("\n", 0, start) + 1, kind, message))
    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit obvious C++ BBAI/SASGameRecord caller-side pre-gates.")
    parser.add_argument("paths", nargs="+", help="C++ files or directories to scan")
    parser.add_argument("--late-gates", action="store_true", help="also report same-line && conditions where the logging gate is not first")
    parser.add_argument("--include-log-implementations", action="store_true", help="also scan BBAILog.cpp and SASGameRecordLog.cpp")
    parser.add_argument("--message-style", action="store_true", help="also report literal BBAI messages without a stable uppercase event prefix, or with leading indentation")
    args = parser.parse_args()

    total = 0
    for path in iter_files(args.paths):
        findings = audit_file(path, args.include_log_implementations, args.late_gates)
        if args.message_style:
            findings.extend(audit_message_style(path.read_text(encoding="utf-8", errors="replace")))
        for line_no, kind, text in findings:
            print(f"{path}:{line_no}: {kind}: {text}")
            total += 1
    print(f"review candidates: {total}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
