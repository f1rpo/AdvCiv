#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Review repeatedly caught source-history mistakes that ordinary syntax checks cannot see: newly authored prose could look inherited, legacy comments could be silently rewritten, dependency changes could lose their rationale, and changed interfaces could omit why their parameters moved.
# Compare the selected Git change range with its base so these provenance rules are checked from the diff itself rather than by linting the already-mixed current tree. (ChatGPT-6-Sol) -->

from __future__ import annotations

import argparse
from collections import defaultdict
from dataclasses import dataclass
import difflib
from pathlib import Path
import re
import subprocess
import sys
import tokenize
from io import StringIO

ROOT = Path(__file__).resolve().parents[3]

CPP_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
PY_SUFFIXES = {".py"}
XML_SUFFIXES = {".xml"}
SOURCE_SUFFIXES = CPP_SUFFIXES | PY_SUFFIXES | XML_SUFFIXES

CUSTOM_OPEN = "<!-- custom:"
CUSTOM_CLOSE = "-->"
MODEL_CREDIT_RE = re.compile(
    r"(?:ChatGPT|GPT(?:-\d)?|Codex|Claude|Gemini|DeepSeek|Grok|wonderingabout|AI/LLM)",
    re.IGNORECASE,
)
REFERENCE_RE = re.compile(
    r"\b(?:advc\.[A-Za-z0-9]+|KI#\d+(?:\.\d+)?|Long_Comments_[A-Za-z0-9_.-]+\s+#?\d+)\b",
    re.IGNORECASE,
)
CPP_INCLUDE_RE = re.compile(r"^[ \t]*#[ \t]*include[ \t]+[<\"]([^>\"]+)[>\"]")
PY_IMPORT_RE = re.compile(
    r"^[ \t]*(?:from[ \t]+([A-Za-z_][\w.]*)[ \t]+import\b|import[ \t]+([A-Za-z_][\w.]*))"
)
SAS_FUNCTION_RE = re.compile(r"SAS")
CONTROL_NAMES = {
    "if", "for", "while", "switch", "catch", "sizeof", "alignof", "decltype",
    "return", "new", "delete", "FAssert", "FAssertMsg",
}

# <!-- custom: Treat repository credit headers and machine directives as metadata rather than newly authored explanatory comments, so new files can keep required boilerplate without weakening provenance checks for ordinary prose. (ChatGPT-6-Sol) -->
BOILERPLATE_COMMENT_RE = re.compile(
    r"^(?:"
    r"AI, UI, logging, or other modifications first developed in AdvCiv-SAS"
    r"|\(c\)\s+2026\s+wonderingabout\b"
    r"|[-*]\s*-\*-\s*coding:"
    r"|coding[:=]"
    r"|noqa\b"
    r"|type:\s*ignore\b"
    r"|pylint:"
    r"|fmt:"
    r"|clang-format\b"
    r"|NOLINT\b"
    r")",
    re.IGNORECASE,
)


@dataclass(frozen=True)
class CommentBlock:
    start: int
    end: int
    raw: str
    text: str
    normalized: str
    custom: bool
    credited: bool
    references: tuple[str, ...]
    standalone: bool


@dataclass(frozen=True)
class Signature:
    name: str
    params: str
    start: int
    end: int


def run_git(repo: Path, *args: str, check: bool = True) -> subprocess.CompletedProcess:
    result = subprocess.run(
        ["git", *args],
        cwd=repo,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=False,
    )
    if check and result.returncode:
        raise RuntimeError(result.stderr.decode("utf-8", errors="replace").strip())
    return result


def has_commit(repo: Path, ref: str) -> bool:
    return run_git(repo, "cat-file", "-e", ref + "^{commit}", check=False).returncode == 0


def ensure_base(repo: Path, base_ref: str | None, event_name: str | None) -> str | None:
    if base_ref and set(base_ref) == {"0"}:
        return None
    if not base_ref:
        candidate = "HEAD^"
        return candidate if has_commit(repo, candidate) else None
    if has_commit(repo, base_ref):
        return base_ref
    if event_name == "push":
        fetched = run_git(repo, "fetch", "--no-tags", "origin", base_ref, check=False)
        if fetched.returncode == 0 and has_commit(repo, base_ref):
            return base_ref
        print(
            "NOTICE: push-before commit %s is unavailable; source-change provenance cannot "
            "check this rewritten push range." % base_ref,
            file=sys.stderr,
        )
        return None
    raise RuntimeError("change-range base %s is unavailable; fetch the base commit first" % base_ref)


def changed_paths(repo: Path, base_ref: str) -> list[str]:
    # <!-- custom: Compare the base with the current working tree, not only HEAD, so local pre-commit runs inspect staged/unstaged edits too; CI checkouts are clean, making this equivalent to base..HEAD there. (ChatGPT-6-Sol) -->
    output = run_git(
        repo,
        "diff",
        "--name-only",
        "--diff-filter=ACDMRTUXB",
        base_ref,
        "--",
    ).stdout.decode("utf-8", errors="replace")
    return [line for line in output.splitlines() if maintained_source_path(line)]


def maintained_source_path(path: str) -> bool:
    posix = path.replace("\\", "/")
    suffix = Path(posix).suffix.lower()
    if suffix not in SOURCE_SUFFIXES:
        return False
    lower = posix.lower()
    if "/context/" in lower or lower.startswith("_snapshot_context/"):
        return False
    if suffix in CPP_SUFFIXES:
        return lower.startswith("cvgamecoredll/")
    if suffix in PY_SUFFIXES:
        return (
            lower.startswith("assets/python/")
            or lower.startswith("privatemaps/")
            or lower.startswith("llm_helpers/")
            or lower.startswith(".github/workflows/")
        )
    return lower.startswith("assets/xml/")


def git_text(repo: Path, ref: str, path: str) -> str | None:
    result = run_git(repo, "show", "%s:%s" % (ref, path), check=False)
    if result.returncode:
        return None
    raw = result.stdout
    for encoding in ("utf-8-sig", "utf-8", "cp1252"):
        try:
            return raw.decode(encoding).replace("\r\n", "\n").replace("\r", "\n")
        except UnicodeDecodeError:
            pass
    return raw.decode("utf-8", errors="replace").replace("\r\n", "\n").replace("\r", "\n")


def current_text(repo: Path, path: str) -> str | None:
    target = repo / path
    if not target.is_file():
        return None
    raw = target.read_bytes()
    for encoding in ("utf-8-sig", "utf-8", "cp1252"):
        try:
            return raw.decode(encoding).replace("\r\n", "\n").replace("\r", "\n")
        except UnicodeDecodeError:
            pass
    return raw.decode("utf-8", errors="replace").replace("\r\n", "\n").replace("\r", "\n")


def normalize_comment_text(text: str) -> str:
    text = text.replace(CUSTOM_OPEN, "").replace(CUSTOM_CLOSE, "")
    text = re.sub(r"^[ \t]*(?://+|#+|/\*+|\*+)[ \t]?", "", text)
    text = re.sub(r"\*/[ \t]*$", "", text)
    return re.sub(r"\s+", " ", text).strip()


def comment_payload(raw: str, suffix: str) -> str:
    text = raw
    if suffix in CPP_SUFFIXES:
        if text.lstrip().startswith("//"):
            pieces = []
            for line in text.splitlines():
                pieces.append(re.sub(r"^[ \t]*//[ \t]?", "", line))
            return "\n".join(pieces)
        text = re.sub(r"^[ \t]*/\*+", "", text)
        text = re.sub(r"\*/[ \t]*$", "", text)
        return "\n".join(re.sub(r"^[ \t]*\*[ \t]?", "", line) for line in text.splitlines())
    if suffix in PY_SUFFIXES:
        return "\n".join(re.sub(r"^[ \t]*#[ \t]?", "", line) for line in text.splitlines())
    text = re.sub(r"^[ \t]*<!--", "", text)
    text = re.sub(r"-->[ \t]*$", "", text)
    return text


def make_comment_block(start: int, end: int, raw: str, suffix: str, standalone: bool) -> CommentBlock:
    payload = comment_payload(raw, suffix)
    normalized = re.sub(r"\s+", " ", payload).strip()
    custom = CUSTOM_OPEN in raw
    tail = raw[-320:]
    credited = bool(custom and CUSTOM_CLOSE in raw and MODEL_CREDIT_RE.search(tail))
    refs = tuple(sorted({match.group(0) for match in REFERENCE_RE.finditer(payload)}, key=str.lower))
    return CommentBlock(start, end, raw, payload.strip(), normalized, custom, credited, refs, standalone)


def cpp_comment_tokens(text: str) -> list[tuple[int, int, str, bool, str]]:
    # <!-- custom: Parse C/C++ comments outside strings/chars so comment provenance is based on actual source comments rather than `//` or `/*` text embedded in literals. (ChatGPT-6-Sol) -->
    tokens = []
    i = 0
    line = 1
    line_start = 0
    n = len(text)
    state = "code"
    quote = ""
    while i < n:
        ch = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if state == "code":
            if ch in ('"', "'"):
                state = "string"
                quote = ch
                i += 1
                continue
            if ch == "/" and nxt == "/":
                start_i = i
                start_line = line
                end_i = text.find("\n", i)
                if end_i < 0:
                    end_i = n
                raw = text[start_i:end_i]
                standalone = not text[line_start:start_i].strip()
                tokens.append((start_line, start_line, raw, standalone, "line"))
                i = end_i
                continue
            if ch == "/" and nxt == "*":
                start_i = i
                start_line = line
                standalone = not text[line_start:start_i].strip()
                end_i = text.find("*/", i + 2)
                if end_i < 0:
                    end_i = n - 2
                end_i += 2
                raw = text[start_i:end_i]
                end_line = start_line + raw.count("\n")
                tokens.append((start_line, end_line, raw, standalone, "block"))
                line += raw.count("\n")
                last_nl = raw.rfind("\n")
                if last_nl >= 0:
                    line_start = end_i - (len(raw) - last_nl - 1)
                i = end_i
                continue
        else:
            if ch == "\\":
                i += 2
                continue
            if ch == quote:
                state = "code"
        if ch == "\n":
            line += 1
            line_start = i + 1
        i += 1
    return tokens


def group_line_comments(tokens, suffix: str) -> list[CommentBlock]:
    result = []
    i = 0
    while i < len(tokens):
        start, end, raw, standalone, kind = tokens[i]
        if kind != "line" or not standalone:
            result.append(make_comment_block(start, end, raw, suffix, standalone))
            i += 1
            continue
        raws = [raw]
        final = end
        custom_mode = CUSTOM_OPEN in raw
        if custom_mode and CUSTOM_CLOSE in raw:
            result.append(make_comment_block(start, end, raw, suffix, True))
            i += 1
            continue
        j = i + 1
        while j < len(tokens):
            ns, ne, nraw, nstandalone, nkind = tokens[j]
            if nkind != "line" or not nstandalone or ns != final + 1:
                break
            if custom_mode:
                raws.append(nraw)
                final = ne
                j += 1
                if CUSTOM_CLOSE in nraw:
                    break
                continue
            if CUSTOM_OPEN in nraw:
                break
            raws.append(nraw)
            final = ne
            j += 1
        result.append(make_comment_block(start, final, "\n".join(raws), suffix, True))
        i = j
    return result


def python_comments(text: str) -> list[CommentBlock]:
    try:
        toks = list(tokenize.generate_tokens(StringIO(text).readline))
    except (tokenize.TokenError, IndentationError):
        return []
    raw_tokens = []
    lines = text.splitlines()
    for tok in toks:
        if tok.type != tokenize.COMMENT:
            continue
        start_line, start_col = tok.start
        line_text = lines[start_line - 1] if start_line - 1 < len(lines) else ""
        standalone = not line_text[:start_col].strip()
        raw_tokens.append((start_line, start_line, tok.string, standalone, "line"))
    return group_line_comments(raw_tokens, ".py")


def xml_comments(text: str) -> list[CommentBlock]:
    result = []
    for match in re.finditer(r"<!--.*?-->", text, flags=re.DOTALL):
        start = text.count("\n", 0, match.start()) + 1
        end = start + match.group(0).count("\n")
        line_start = text.rfind("\n", 0, match.start()) + 1
        standalone = not text[line_start:match.start()].strip()
        result.append(make_comment_block(start, end, match.group(0), ".xml", standalone))
    return result


def comments(text: str, suffix: str) -> list[CommentBlock]:
    if suffix in CPP_SUFFIXES:
        return group_line_comments(cpp_comment_tokens(text), suffix)
    if suffix in PY_SUFFIXES:
        return python_comments(text)
    if suffix in XML_SUFFIXES:
        return xml_comments(text)
    return []


def exempt_new_comment(block: CommentBlock) -> bool:
    payload = normalize_comment_text(block.text)
    if not payload:
        return True
    if block.raw.lstrip().startswith("#!"):
        return True
    return bool(BOILERPLATE_COMMENT_RE.match(payload))


def unmatched_blocks(old: list[CommentBlock], new: list[CommentBlock], custom: bool) -> tuple[list[CommentBlock], list[CommentBlock]]:
    old_by = defaultdict(list)
    new_by = defaultdict(list)
    for block in old:
        if block.custom == custom:
            old_by[block.normalized].append(block)
    for block in new:
        if block.custom == custom:
            new_by[block.normalized].append(block)
    removed = []
    added = []
    for key in set(old_by) | set(new_by):
        old_items = old_by.get(key, [])
        new_items = new_by.get(key, [])
        common = min(len(old_items), len(new_items))
        removed.extend(old_items[common:])
        added.extend(new_items[common:])
    return removed, added


def line_opcodes(old_text: str, new_text: str):
    old_lines = old_text.splitlines()
    new_lines = new_text.splitlines()
    return old_lines, new_lines, difflib.SequenceMatcher(a=old_lines, b=new_lines, autojunk=False).get_opcodes()


def map_old_line_to_new(line: int, opcodes) -> int:
    index = max(0, line - 1)
    for _tag, i1, i2, j1, j2 in opcodes:
        if i1 <= index < i2:
            if i2 == i1:
                return j1 + 1
            offset = min(index - i1, max(0, j2 - j1 - 1))
            return j1 + offset + 1
        if index < i1:
            return j1 + 1
    return (opcodes[-1][4] + 1) if opcodes else 1


def nearby_added_custom(added_custom: list[CommentBlock], start: int, end: int, radius: int = 5) -> list[CommentBlock]:
    low = max(1, start - radius)
    high = end + radius
    return [block for block in added_custom if block.end >= low and block.start <= high]


def directive_kind(line: str, suffix: str):
    if suffix in CPP_SUFFIXES:
        match = CPP_INCLUDE_RE.match(line)
        return ("include", match.group(1)) if match else None
    if suffix in PY_SUFFIXES:
        match = PY_IMPORT_RE.match(line)
        if match:
            return ("import", match.group(1) or match.group(2))
    return None


def import_include_changes(old_text: str, new_text: str, suffix: str):
    old_lines, new_lines, opcodes = line_opcodes(old_text, new_text)
    changes = []
    for tag, i1, i2, j1, j2 in opcodes:
        if tag == "equal":
            continue
        directives = []
        for index in range(i1, i2):
            found = directive_kind(old_lines[index], suffix)
            if found:
                directives.append(("removed", index + 1, found[0], found[1], old_lines[index].strip()))
        for index in range(j1, j2):
            found = directive_kind(new_lines[index], suffix)
            if found:
                directives.append(("added", index + 1, found[0], found[1], new_lines[index].strip()))
        if directives:
            changes.append((j1 + 1, max(j1 + 1, j2), directives))
    return changes


def mask_cpp(text: str) -> str:
    chars = list(text)
    i = 0
    n = len(chars)
    state = "code"
    quote = ""
    while i < n:
        ch = chars[i]
        nxt = chars[i + 1] if i + 1 < n else ""
        if state == "code":
            if ch in ('"', "'"):
                state = "string"
                quote = ch
                chars[i] = " "
                i += 1
                continue
            if ch == "/" and nxt == "/":
                chars[i] = chars[i + 1] = " "
                i += 2
                while i < n and chars[i] != "\n":
                    chars[i] = " "
                    i += 1
                continue
            if ch == "/" and nxt == "*":
                chars[i] = chars[i + 1] = " "
                i += 2
                while i < n - 1 and not (chars[i] == "*" and chars[i + 1] == "/"):
                    if chars[i] != "\n":
                        chars[i] = " "
                    i += 1
                if i < n - 1:
                    chars[i] = chars[i + 1] = " "
                    i += 2
                continue
        else:
            if ch == "\\":
                chars[i] = " "
                if i + 1 < n and chars[i + 1] != "\n":
                    chars[i + 1] = " "
                i += 2
                continue
            if ch == quote:
                state = "code"
            if ch != "\n":
                chars[i] = " "
            i += 1
            continue
        i += 1
    return "".join(chars)


def matching_paren(text: str, open_pos: int) -> int | None:
    depth = 0
    for index in range(open_pos, len(text)):
        if text[index] == "(":
            depth += 1
        elif text[index] == ")":
            depth -= 1
            if depth == 0:
                return index
    return None




def cpp_params_look_declarative(params: str) -> bool:
    if not params or params == "void":
        return True
    pieces = []
    depth = 0
    start = 0
    for index, ch in enumerate(params):
        if ch in "(<[{":
            depth += 1
        elif ch in ")>]}":
            depth = max(0, depth - 1)
        elif ch == "," and depth == 0:
            pieces.append(params[start:index].strip())
            start = index + 1
    pieces.append(params[start:].strip())
    primitive = {"bool", "char", "short", "int", "long", "float", "double", "size_t", "wchar_t"}
    for piece in pieces:
        base = piece.split("=", 1)[0].strip()
        if not base:
            return False
        if re.search(r"\s|[*&]|\.\.\.", base):
            continue
        bare = base.split("::")[-1]
        if bare in primitive or bare.endswith("Types"):
            continue
        if re.match(r"^[A-Z][A-Za-z0-9_<>]*$", bare) and not bare.isupper():
            continue
        return False
    return True


def cpp_signatures(text: str) -> list[Signature]:
    code = mask_cpp(text)
    result = []
    for match in re.finditer(r"\(", code):
        open_pos = match.start()
        close_pos = matching_paren(code, open_pos)
        if close_pos is None:
            continue
        before = code[max(0, open_pos - 240):open_pos]
        name_match = re.search(r"(operator\s*[^\s(]+|[A-Za-z_~][A-Za-z0-9_:~]*)\s*$", before)
        if not name_match:
            continue
        name = re.sub(r"\s+", "", name_match.group(1))
        short_name = name.split("::")[-1]
        if short_name in CONTROL_NAMES:
            continue
        name_start = max(0, open_pos - 240) + name_match.start(1)
        boundary = max(
            code.rfind(";", 0, name_start),
            code.rfind("{", 0, name_start),
            code.rfind("}", 0, name_start),
        )
        prefix = code[boundary + 1:name_start].strip()
        # <!-- custom: Require a declaration-like prefix because qualified calls such as std::min(...) otherwise look deceptively similar to out-of-class definitions. (ChatGPT-6-Sol) -->
        if re.search(r"\b(?:return|if|for|while|switch|case|else|new|delete)\b", prefix):
            continue
        if re.search(r"(?:->|\.|=|\+|!|\?|\||/|%)", prefix):
            continue
        if "(" in prefix or ")" in prefix:
            continue
        after = code[close_pos + 1:close_pos + 220]
        qualifier = after
        # <!-- custom: Strip only common declaration qualifiers; this checker needs a conservative signature heuristic, not a full C++ parser. (ChatGPT-6-Sol) -->
        qualifier = re.sub(r"^[ \t\r\n]+", "", qualifier)
        consumed = True
        while consumed:
            old = qualifier
            qualifier = re.sub(
                r"^(?:const|volatile|override|final|noexcept(?:\s*\([^)]*\))?|"
                r"throw\s*\([^)]*\)|__declspec\s*\([^)]*\)|=\s*(?:0|default|delete))\s*",
                "",
                qualifier,
            )
            consumed = qualifier != old
        if not qualifier:
            continue
        if qualifier[0] not in "{;:":
            continue
        if not prefix:
            # <!-- custom: Accept no-return-type qualified forms only for out-of-class constructors/destructors; ordinary qualified calls such as std::sort(...) stay outside the signature audit. (ChatGPT-6-Sol) -->
            parts = name.split("::")
            if len(parts) < 2 or parts[-1].lstrip("~") != parts[-2].lstrip("~") or qualifier[0] == ";":
                continue
        params = re.sub(r"\s+", " ", code[open_pos + 1:close_pos]).strip()
        if qualifier[0] == ";" and not cpp_params_look_declarative(params):
            continue
        start_line = code.count("\n", 0, name_start) + 1
        end_line = code.count("\n", 0, close_pos) + 1
        result.append(Signature(name, params, start_line, end_line))
    # <!-- custom: Nested parentheses can generate duplicate heuristic candidates; keep only unique signature spans before comparing parameter lists. (ChatGPT-6-Sol) -->
    unique = {}
    for sig in result:
        unique[(sig.name, sig.start, sig.end, sig.params)] = sig
    return sorted(unique.values(), key=lambda sig: (sig.start, sig.end, sig.name))


PY_DEF_RE = re.compile(r"(?m)^(?P<indent>[ \t]*)def[ \t]+(?P<name>[A-Za-z_]\w*)[ \t]*\(")


def python_signatures(text: str) -> list[Signature]:
    result = []
    for match in PY_DEF_RE.finditer(text):
        open_pos = text.find("(", match.start(), match.end() + 1)
        close_pos = matching_paren(text, open_pos)
        if close_pos is None:
            continue
        rest = text[close_pos + 1:close_pos + 50]
        if not re.match(r"[ \t]*(?:->[^\n:]+)?[ \t]*:", rest):
            continue
        params = re.sub(r"\s+", " ", text[open_pos + 1:close_pos]).strip()
        start = text.count("\n", 0, match.start()) + 1
        end = text.count("\n", 0, close_pos) + 1
        result.append(Signature(match.group("name"), params, start, end))
    return result


def signatures(text: str, suffix: str) -> list[Signature]:
    if suffix in CPP_SUFFIXES:
        return cpp_signatures(text)
    if suffix in PY_SUFFIXES:
        return python_signatures(text)
    return []


def changed_signatures(old_text: str, new_text: str, suffix: str):
    old_by = defaultdict(list)
    new_by = defaultdict(list)
    for sig in signatures(old_text, suffix):
        old_by[sig.name].append(sig)
    for sig in signatures(new_text, suffix):
        new_by[sig.name].append(sig)
    changes = []
    for name in sorted(set(old_by) & set(new_by)):
        if SAS_FUNCTION_RE.search(name):
            continue
        old_list = old_by[name][:]
        new_list = new_by[name][:]
        # <!-- custom: Remove exact overload matches first so only unmatched overloads are paired as possible parameter-contract changes. (ChatGPT-6-Sol) -->
        used_new = set()
        remaining_old = []
        for old_sig in old_list:
            found = None
            for index, new_sig in enumerate(new_list):
                if index in used_new:
                    continue
                if old_sig.params == new_sig.params:
                    found = index
                    break
            if found is None:
                remaining_old.append(old_sig)
            else:
                used_new.add(found)
        remaining_new = [sig for index, sig in enumerate(new_list) if index not in used_new]
        for old_sig, new_sig in zip(remaining_old, remaining_new):
            if old_sig.params != new_sig.params:
                changes.append((old_sig, new_sig))
    return changes


# <!-- custom: Add an optional warnings collector so missing model credits remain advisory: Git cannot distinguish an uncredited user comment from an LLM comment.
# Other provenance failures retain their blocking results. (GPT-6.1-Sol) -->
def check_change(path: str, old_text: str | None, new_text: str | None, warnings: list[str] | None = None) -> list[str]:
    suffix = Path(path).suffix.lower()
    if suffix not in SOURCE_SUFFIXES or new_text is None:
        return []
    errors = []
    new_file = old_text is None
    old_text = old_text or ""
    old_comments = comments(old_text, suffix)
    new_comments = comments(new_text, suffix)
    removed_legacy, added_legacy = unmatched_blocks(old_comments, new_comments, custom=False)
    _removed_custom, added_custom = unmatched_blocks(old_comments, new_comments, custom=True)

    for block in added_legacy:
        if exempt_new_comment(block):
            continue
        errors.append(
            "%s:%d: new/rewritten non-custom comment; preserve inherited wording (reflow-only is allowed) "
            "or use '<!-- custom: ... (model credit) -->'" % (path, block.start)
        )
    for block in added_custom:
        if not block.credited and warnings is not None:
            warnings.append(
                "%s:%d: new/rewritten custom comment has no recognized model credit near its closing '-->'"
                % (path, block.start)
            )

    _old_lines, _new_lines, opcodes = line_opcodes(old_text, new_text)
    for block in removed_legacy:
        if not block.references:
            continue
        mapped = map_old_line_to_new(block.start, opcodes)
        nearby = nearby_added_custom(added_custom, mapped, mapped, radius=6)
        if not nearby:
            continue
        combined = "\n".join(item.raw for item in nearby)
        missing = [ref for ref in block.references if ref.lower() not in combined.lower()]
        if missing:
            errors.append(
                "%s:%d: replacement custom comment near removed inherited comment dropped provenance reference(s): %s"
                % (path, nearby[0].start, ", ".join(missing))
            )

    if not new_file and suffix in CPP_SUFFIXES | PY_SUFFIXES:
        for start, end, directives in import_include_changes(old_text, new_text, suffix):
            nearby = nearby_added_custom(added_custom, start, end, radius=5)
            if not nearby:
                summary = ", ".join("%s %s" % (action, target) for action, _line, _kind, target, _raw in directives)
                errors.append(
                    "%s:%d: include/import change lacks a nearby new custom rationale comment (%s)"
                    % (path, start, summary)
                )

        for old_sig, new_sig in changed_signatures(old_text, new_text, suffix):
            nearby = nearby_added_custom(added_custom, new_sig.start, new_sig.end, radius=6)
            if not nearby:
                errors.append(
                    "%s:%d: parameters changed for %s without a nearby new custom comment explaining the interface change"
                    % (path, new_sig.start, new_sig.name)
                )

    return errors


# <!-- custom: Forward the optional warning collector across the Git range without treating user-optional model credits as CI failures. (GPT-6.1-Sol) -->
def check(repo: Path, base_ref: str | None, event_name: str | None = None, warnings: list[str] | None = None) -> tuple[list[str], int]:
    base = ensure_base(repo, base_ref, event_name)
    if base is None:
        return [], 0
    errors = []
    count = 0
    for path in changed_paths(repo, base):
        old = git_text(repo, base, path)
        new = current_text(repo, path)
        if new is None:
            # <!-- custom: Whole-file deletion cannot introduce a misleading new comment/dependency/interface, so only surviving current files need source-change provenance checks. (ChatGPT-6-Sol) -->
            continue
        count += 1
        errors.extend(check_change(path, old, new, warnings))
    return errors, count


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate diff-aware source comment, dependency and changed-parameter provenance."
    )
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--base-ref", help="PR base or push-before commit; defaults to HEAD^ for local/manual runs")
    parser.add_argument("--event-name", choices=("push", "pull_request", "workflow_dispatch", "local"), default="local")
    args = parser.parse_args()
    warnings = []
    try:
        errors, files_checked = check(args.repo_root, args.base_ref, args.event_name, warnings)
    except (RuntimeError, OSError, ValueError) as exc:
        print("FAIL source change provenance setup: %s" % exc)
        return 1
    print("FAIL source change provenance" if errors else "PASS source change provenance (%d changed source file(s) checked)" % files_checked)
    for error in errors:
        print("  - " + error)
    for warning in warnings:
        print("WARNING: " + warning)
    return int(bool(errors))


if __name__ == "__main__":
    sys.exit(main())
