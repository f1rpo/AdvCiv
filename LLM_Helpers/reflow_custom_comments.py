#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Reflow only AdvCiv-SAS custom source comments by logical sentence.
# This is deliberately not a width formatter: join physical wraps inside one prose
# sentence, put distinct prose sentences on distinct physical comment lines, preserve
# semicolon-joined short clauses, and skip/report structured or code-like comments.

from __future__ import annotations

import argparse
import difflib
import io
from pathlib import Path
import re
import sys
import tokenize

DEFAULT_EXTENSIONS = {".cpp", ".h", ".inl", ".py", ".xml"}
SKIP_DIR_NAMES = {".git", "__pycache__", "_0_Common_Docs", "_SNAPSHOT_CONTEXT", "outputs"}
SKIP_PATH_PARTS = {
    ("LLM_Helpers", "context"),
    ("LLM_Helpers", "examples"),
}

CODE_OPEN_RE = re.compile(r"^(?P<indent>[ \t]*)(?P<prefix>//|#)[ \t]*<!-- custom:[ \t]*(?P<body>.*)$")
XML_OPEN_RE = re.compile(r"^(?P<indent>[ \t]*)<!-- custom:[ \t]*(?P<body>.*)$")
CREDIT_NAME_RE = re.compile(r"(?:ChatGPT|GPT|Codex|Claude|Gemini|DeepSeek|Grok|LLM)", re.IGNORECASE)
KI_ONLY_RE = re.compile(r"^(?:See(?: also)?|Cf\.)\s+KI#[0-9]", re.IGNORECASE)
WAS_TAIL_RE = re.compile(r"^Was\s+[^.!?]+\.$", re.IGNORECASE)
CREDIT_ONLY_RE = re.compile(r"^(?:Credit|Credits):\s+.+$", re.IGNORECASE)
LISTISH_RE = re.compile(r"^(?:[-*+]\s+|\d+[.)]\s+|Row\s+\d+\b|[XY]:\s+|Top[- ]|Bottom[- ]|Left[- ]|Right[- ])", re.IGNORECASE)
CODEISH_START_RE = re.compile(
    r"^(?:if|else|for|while|switch|case|return|continue|break|try|except|def|class|import|from|typedef|struct|enum|const|static)\b"
)

CONTINUATION_WORDS = {
    "a", "an", "and", "as", "at", "because", "but", "by", "for", "from", "if", "in", "into", "nor",
    "of", "on", "or", "so", "than", "that", "the", "then", "to", "when", "where", "which", "while", "who",
    "whose", "with", "without", "yet",
}

ABBREVIATIONS = (
    "e.g.", "i.e.", "etc.", "vs.", "cf.", "approx.", "no.", "fig.", "mr.", "mrs.", "ms.", "dr.", "st.",
)


def newline_style(raw: bytes) -> str:
    if b"\r\n" in raw:
        return "\r\n"
    if b"\r" in raw:
        return "\r"
    return "\n"


def decode_source(raw: bytes):
    if raw.startswith(b"\xef\xbb\xbf"):
        return raw.decode("utf-8-sig"), "utf-8-sig"
    if raw.startswith((b"\xff\xfe", b"\xfe\xff")):
        return raw.decode("utf-16"), "utf-16"
    head = raw[:512].decode("ascii", errors="ignore")
    match = re.search(r"encoding=[\"']([^\"']+)[\"']", head, re.IGNORECASE)
    candidates = []
    if match:
        candidates.append(match.group(1))
    candidates.extend(["utf-8", "cp1252"])
    for encoding in candidates:
        try:
            return raw.decode(encoding), encoding
        except (UnicodeDecodeError, LookupError):
            pass
    return None, None


def path_is_skipped(path: Path, root: Path) -> bool:
    try:
        rel = path.relative_to(root)
    except ValueError:
        rel = path
    parts = rel.parts
    if any(part in SKIP_DIR_NAMES for part in parts[:-1]):
        return True
    for pair in SKIP_PATH_PARTS:
        for i in range(len(parts) - len(pair) + 1):
            if tuple(parts[i:i + len(pair)]) == pair:
                return True
    return False




def display_path(path: Path, root: Path) -> str:
    try:
        return str(path.relative_to(root))
    except ValueError:
        return str(path)


def significant_python_tokens(text: str):
    result = []
    try:
        stream = tokenize.generate_tokens(io.StringIO(text).readline)
        for tok in stream:
            if tok.type in (
                tokenize.COMMENT,
                tokenize.NL,
                tokenize.NEWLINE,
                tokenize.ENCODING if hasattr(tokenize, "ENCODING") else -1,
                tokenize.ENDMARKER,
            ):
                continue
            result.append((tok.type, tok.string))
    except (tokenize.TokenError, IndentationError):
        return None
    return result


def strip_credit(text: str):
    stripped = text.rstrip()
    if not stripped.endswith(")"):
        return stripped, ""
    depth = 0
    start = None
    for i in range(len(stripped) - 1, -1, -1):
        ch = stripped[i]
        if ch == ")":
            depth += 1
        elif ch == "(":
            depth -= 1
            if depth == 0:
                start = i
                break
    if start is None:
        return stripped, ""
    candidate = stripped[start:]
    if not CREDIT_NAME_RE.search(candidate):
        return stripped, ""
    return stripped[:start].rstrip(), candidate


def sentence_boundaries(text: str):
    """Yield (start, end) spans for prose sentences without changing punctuation."""
    spans = []
    start = 0
    i = 0
    lower = text.lower()
    n = len(text)
    while i < n:
        if text[i] not in ".!?":
            i += 1
            continue

        # Consume ellipses / repeated punctuation and closing quotes/parens.
        end_punct = i + 1
        while end_punct < n and text[end_punct] in ".!?":
            end_punct += 1
        while end_punct < n and text[end_punct] in "\"')]}":
            end_punct += 1

        if text[i] == ".":
            prefix = lower[:i + 1]
            if any(re.search(r"(?<![A-Za-z])" + re.escape(abbr) + r"$", prefix) for abbr in ABBREVIATIONS):
                i += 1
                continue
            # Single-letter initials / dotted acronyms, e.g. "U.S.".
            token_match = re.search(r"(?:\b[A-Za-z]\.){1,}[A-Za-z]?$", text[:i + 1])
            if token_match:
                i += 1
                continue

        j = end_punct
        while j < n and text[j].isspace():
            j += 1
        if j >= n:
            spans.append((start, n))
            start = n
            break
        if j == end_punct:  # punctuation embedded in a token, not a sentence boundary
            i += 1
            continue
        if text[i:end_punct].startswith("...") and text[j:j + 1].islower():
            i = end_punct
            continue

        piece = text[start:end_punct].strip()
        if piece:
            spans.append((start, end_punct))
        start = j
        i = j

    if start < n:
        tail = text[start:].strip()
        if tail:
            left = text.find(tail, start)
            spans.append((left, left + len(tail)))
    if not spans and text.strip():
        stripped = text.strip()
        left = text.find(stripped)
        spans.append((left, left + len(stripped)))
    return spans


def metadata_piece(piece: str) -> bool:
    stripped = piece.strip()
    if KI_ONLY_RE.match(stripped) or CREDIT_ONLY_RE.match(stripped):
        return True
    if stripped.startswith("(") and CREDIT_NAME_RE.search(stripped):
        return True
    if re.match(r"^Long_Comments_[A-Za-z0-9_.-]+\s+#?\d+", stripped, re.IGNORECASE):
        return True
    return False


def split_sentences(text: str):
    pieces = [text[a:b].strip() for a, b in sentence_boundaries(text) if text[a:b].strip()]
    merged = []
    for piece in pieces:
        if merged and metadata_piece(piece):
            merged[-1] += " " + piece
            continue
        if merged and WAS_TAIL_RE.match(piece) and merged[-1].endswith("."):
            merged[-1] = merged[-1][:-1] + "; " + piece[:1].lower() + piece[1:]
            continue
        merged.append(piece)
    return merged


def prose_block_is_safe(parts):
    if not parts:
        return False, "empty"
    if any(not part.strip() for part in parts):
        return False, "blank/structured"
    for part in parts:
        s = part.strip()
        if "<!-- custom:" in s or "-->" in s:
            return False, "nested/malformed"
        if LISTISH_RE.match(s):
            return False, "list/layout"
        if CODEISH_START_RE.match(s):
            return False, "code-like"
        if s.startswith(("<", "</", "{", "}", "[", "]")):
            return False, "code/xml-like"
        if re.search(r"(?:^|\s)[A-Za-z_][A-Za-z0-9_]*\s*=\s*[^=]", s):
            return False, "assignment-like"
        if s.endswith(("{", "}", ";", "\\")) and not re.search(r"[.!?]\s*$", s):
            return False, "code-like"
    return True, ""


def parse_block(lines, start, suffix):
    line = lines[start]
    match = CODE_OPEN_RE.match(line) if suffix != ".xml" else XML_OPEN_RE.match(line)
    if not match:
        return None
    indent = match.group("indent")
    prefix = match.groupdict().get("prefix") or ""
    body0 = match.group("body")
    if "-->" in body0:
        before, _sep, after = body0.partition("-->")
        if after.strip():
            return {"end": start, "skip": "trailing text after -->"}
        return {
            "end": start,
            "indent": indent,
            "prefix": prefix,
            "parts": [before.rstrip()],
            "raw": lines[start:start + 1],
        }

    parts = [body0.rstrip()]
    j = start + 1
    while j < len(lines) and j - start <= 100:
        current = lines[j]
        if suffix == ".xml":
            if not current.startswith(indent):
                return {"end": j - 1, "skip": "continuation indentation changed"}
            content = current[len(indent):]
        else:
            cont_re = re.compile(r"^" + re.escape(indent) + re.escape(prefix) + r"(?:[ \t]?)(?P<body>.*)$")
            cont_match = cont_re.match(current)
            if not cont_match:
                return {"end": j - 1, "skip": "non-comment continuation/malformed block"}
            content = cont_match.group("body")
        if "-->" in content:
            before, _sep, after = content.partition("-->")
            if after.strip():
                return {"end": j, "skip": "trailing text after -->"}
            parts.append(before.rstrip())
            return {
                "end": j,
                "indent": indent,
                "prefix": prefix,
                "parts": parts,
                "raw": lines[start:j + 1],
            }
        parts.append(content.rstrip())
        j += 1
    return {"end": min(j, len(lines) - 1), "skip": "unterminated custom comment"}


def join_physical_parts(parts):
    cleaned = [re.sub(r"[ \t]+", " ", part.strip()) for part in parts]
    if not cleaned:
        return None, "empty"
    out = cleaned[0]
    previous = cleaned[0]
    for current in cleaned[1:]:
        prev = previous.rstrip()
        cur = current.lstrip()
        if not prev or not cur:
            return None, "blank/structured"
        last_word_match = re.search(r"([A-Za-z]+)[^A-Za-z]*$", prev)
        last_word = last_word_match.group(1).lower() if last_word_match else ""
        obvious_continuation = (
            prev.endswith((",", ";", ":", "(", "[", "{", "/", "-", "="))
            or last_word in CONTINUATION_WORDS
            or cur[:1].islower()
            or cur.startswith((")", "]", "}", ",", ";", ":", "("))
        )
        strong_boundary = bool(re.search(r"[.!?][\"')\]}]*$", prev))
        if not strong_boundary and not obvious_continuation:
            return None, "ambiguous physical boundary"
        out += " " + cur
        previous = cur
    return out.strip(), ""


def render_block(block):
    parts = block["parts"]
    safe, reason = prose_block_is_safe(parts)
    if not safe:
        return None, reason

    joined, reason = join_physical_parts(parts)
    if joined is None:
        return None, reason
    body, credit = strip_credit(joined)
    sentences = split_sentences(body)
    if not sentences:
        return None, "empty after normalization"
    if credit:
        sentences[-1] = (sentences[-1] + " " + credit).strip()

    indent = block["indent"]
    prefix = block["prefix"]
    leader = indent + (prefix + " " if prefix else "")
    out = []
    for i, sentence in enumerate(sentences):
        if i == 0:
            content = "<!-- custom: " + sentence
        else:
            content = sentence
        if i == len(sentences) - 1:
            content += " -->"
        out.append(leader + content)
    return out, ""


def transform_text(text: str, suffix: str):
    lines = text.split("\n")
    had_final_newline = bool(lines and lines[-1] == "")
    if had_final_newline:
        lines = lines[:-1]

    out = []
    stats = {"seen": 0, "changed": 0, "unchanged": 0, "skipped": 0}
    skip_reasons = {}
    i = 0
    while i < len(lines):
        block = parse_block(lines, i, suffix)
        if block is None:
            out.append(lines[i])
            i += 1
            continue
        stats["seen"] += 1
        end = block["end"]
        if "skip" in block:
            raw = lines[i:end + 1]
            out.extend(raw)
            stats["skipped"] += 1
            skip_reasons[block["skip"]] = skip_reasons.get(block["skip"], 0) + 1
            i = end + 1
            continue
        rendered, reason = render_block(block)
        if rendered is None:
            out.extend(block["raw"])
            stats["skipped"] += 1
            skip_reasons[reason] = skip_reasons.get(reason, 0) + 1
        elif rendered != block["raw"]:
            out.extend(rendered)
            stats["changed"] += 1
        else:
            out.extend(block["raw"])
            stats["unchanged"] += 1
        i = end + 1

    result = "\n".join(out)
    if had_final_newline:
        result += "\n"
    return result, stats, skip_reasons


def iter_files(paths, root: Path, extensions):
    seen = set()
    for input_path in paths:
        path = (root / input_path).resolve() if not Path(input_path).is_absolute() else Path(input_path).resolve()
        if path.is_file():
            candidates = [path]
        elif path.is_dir():
            candidates = path.rglob("*")
        else:
            print("warning: path not found: %s" % input_path, file=sys.stderr)
            continue
        for candidate in candidates:
            if not candidate.is_file() or candidate.suffix.lower() not in extensions:
                continue
            if path_is_skipped(candidate, root):
                continue
            key = str(candidate)
            if key in seen:
                continue
            seen.add(key)
            yield candidate


def main():
    script = Path(__file__).resolve()
    root = script.parent.parent
    parser = argparse.ArgumentParser(description="Reflow marked AdvCiv-SAS custom prose comments by logical sentence, never by width.")
    parser.add_argument("paths", nargs="*", default=["."], help="files/directories relative to repo root (default: repo root)")
    parser.add_argument("--apply", action="store_true", help="rewrite files in place")
    parser.add_argument("--check", action="store_true", help="exit 1 if any safe reflow would change a file")
    parser.add_argument("--diff", action="store_true", help="print unified diff for safe proposed changes")
    parser.add_argument("--extensions", default="cpp,h,inl,py,xml", help="comma-separated extensions (default: cpp,h,inl,py,xml)")
    args = parser.parse_args()

    extensions = {"." + x.strip().lstrip(".").lower() for x in args.extensions.split(",") if x.strip()}
    unknown = extensions - DEFAULT_EXTENSIONS
    if unknown:
        parser.error("unsupported extensions: %s" % ", ".join(sorted(unknown)))

    totals = {"files": 0, "files_changed": 0, "seen": 0, "changed": 0, "unchanged": 0, "skipped": 0}
    reasons = {}

    for path in iter_files(args.paths, root, extensions):
        totals["files"] += 1
        raw = path.read_bytes()
        nl = newline_style(raw)
        text, encoding = decode_source(raw)
        if text is None:
            print("skip undecodable: %s" % display_path(path, root))
            continue
        normalized = text.replace("\r\n", "\n").replace("\r", "\n")
        new, stats, skip_reasons = transform_text(normalized, path.suffix.lower())
        for key in ("seen", "changed", "unchanged", "skipped"):
            totals[key] += stats[key]
        for reason, count in skip_reasons.items():
            reasons[reason] = reasons.get(reason, 0) + count
        if new == normalized:
            continue

        if path.suffix.lower() == ".py":
            before_tokens = significant_python_tokens(normalized)
            after_tokens = significant_python_tokens(new)
            if before_tokens is None or after_tokens is None or before_tokens != after_tokens:
                raise SystemExit("ERROR: Python token safety check failed for %s" % display_path(path, root))

        totals["files_changed"] += 1
        rel = display_path(path, root)
        if args.diff:
            sys.stdout.write("".join(difflib.unified_diff(
                normalized.splitlines(True), new.splitlines(True),
                fromfile=rel, tofile=rel,
            )))
        if args.apply:
            out = new if nl == "\n" else new.replace("\n", nl)
            path.write_bytes(out.encode(encoding))

    mode = "applied" if args.apply else "would_change"
    print("custom-comment logical reflow: files_scanned=%d files_%s=%d blocks_seen=%d blocks_changed=%d blocks_unchanged=%d blocks_skipped=%d" % (
        totals["files"], mode, totals["files_changed"], totals["seen"], totals["changed"], totals["unchanged"], totals["skipped"]
    ))
    if reasons:
        print("skips:")
        for reason, count in sorted(reasons.items(), key=lambda item: (-item[1], item[0])):
            print("  %5d  %s" % (count, reason))

    if args.check and totals["files_changed"]:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
