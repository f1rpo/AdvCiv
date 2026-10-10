#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Audit XML declarations against actual define lookups rather than all SAS-prefixed tokens: enums, widget names, diagnostics and comments are not configuration consumers.
# Follow forwarding helpers and local string aliases; expand bounded integer format lookups from their source guards.
# This checks static references, not runtime reachability. (GPT-6.1-Sol) -->

import argparse
from bisect import bisect_right
import ast
import io
from pathlib import Path
import re
import sys
import tokenize
import xml.etree.ElementTree as ET

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))
from xml_defines import get_default_repo_root, local_name, child_text_by_local_name

DEFINES_PATH = "Assets/XML/GlobalDefines_advciv_sas.xml"
SOURCE_GLOBS = ("Assets/Python/**/*.py", "PrivateMaps/**/*.py", "CvGameCoreDLL/**/*.cpp", "CvGameCoreDLL/**/*.h")
GETTER_RE = re.compile(r"getDefine(?:INT(?:External)?|BOOL|FLOAT|STRING)$")
NAME_RE = re.compile(r"SAS_[A-Z0-9_]+$")
CPP_LEXER = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*|\d+|[^\s]', re.S)
# <!-- custom: This existing define is coverage metadata, deliberately consumed by mapscripts.py rather than the game.
# Keep its exception explicit, verify that checker still reads it, and report it separately from runtime uses. (GPT-6.1-Sol) -->
METADATA = {"SAS_MAP_SCRIPT_NAMES_HEAVINESS_UNSPECIFIED": ".github/workflows/build/mapscripts.py"}
# <!-- custom: BBAI log-level Defines are consumed through an iterable C++ descriptor table instead of one literal getter call per category.
# Recognize only that exact field-pair shape when the same source also contains a dynamic helper lookup of descriptor names; arbitrary strings still do not satisfy runtime usage. (ChatGPT-5.6-Sol) -->
BBAI_REGISTRY_PATH = "CvGameCoreDLL/BBAILog.cpp"
BBAI_REGISTRY_CONSUMER_RE = re.compile(r"getClampedSASBBAILogLevel\s*\(\s*[A-Za-z_]\w*\s*\.\s*szDefineName\s*\)")



def lex(text, python):
    if python:
        return [(t.string, t.start[0], t.start[1], t.type == tokenize.STRING) for t in tokenize.generate_tokens(io.StringIO(text).readline) if t.type not in (tokenize.COMMENT, tokenize.NL, tokenize.NEWLINE, tokenize.INDENT, tokenize.DEDENT, tokenize.ENDMARKER, tokenize.ENCODING)]
    result = []
    newlines = [-1] + [m.start() for m in re.finditer("\n", text)]
    for match in CPP_LEXER.finditer(text):
        value = match.group()
        if value.startswith(("//", "/*")):
            continue
        line = bisect_right(newlines, match.start())
        column = match.start() - newlines[line - 1] - 1
        result.append((value, line, column, value.startswith(('"', "'"))))
    return result


def arguments(tokens, start):
    parts = [[]]
    depth = 0
    for index in range(start + 1, len(tokens)):
        value = tokens[index][0]
        if value == ")" and depth == 0:
            return parts, index
        if value == "," and depth == 0:
            parts.append([])
            continue
        if value in ("(", "[", "{"):
            depth += 1
        elif value in (")", "]", "}"):
            depth -= 1
        parts[-1].append(tokens[index])
    raise ValueError("unclosed argument list")


def calls(tokens):
    for index in range(len(tokens) - 1):
        if re.fullmatch(r"[A-Za-z_]\w*", tokens[index][0]) and tokens[index + 1][0] == "(":
            try:
                args, end = arguments(tokens, index + 1)
            except ValueError:
                # <!-- custom: Inherited C++ conditional-compilation branches can split a surrounding call/body; a partial enclosing construct is not a lookup.
                # Complete nested getter calls are still scanned independently. (GPT-6.1-Sol) -->
                continue
            yield index, tokens[index][0], args, end


def forwarding_helpers(sources):
    helpers = {}
    changed = True
    while changed:
        changed = False
        for _, _, tokens, python in sources:
            for index, name, params, end in calls(tokens):
                if python:
                    if index == 0 or tokens[index - 1][0] != "def":
                        continue
                    body_end = end + 2
                    while body_end < len(tokens) and (tokens[body_end][1] <= tokens[end][1] or tokens[body_end][2] > tokens[index - 1][2]):
                        body_end += 1
                    body = tokens[end + 2:body_end]
                else:
                    if end + 1 >= len(tokens) or tokens[end + 1][0] != "{":
                        continue
                    depth = 1
                    body_end = end + 2
                    while body_end < len(tokens) and depth:
                        depth += (tokens[body_end][0] == "{") - (tokens[body_end][0] == "}")
                        body_end += 1
                    body = tokens[end + 2:body_end - 1]
                names = [p[-1][0] if p else "" for p in params]
                for _, callee, args, _ in calls(body):
                    arg_index = 0 if GETTER_RE.fullmatch(callee) else helpers.get(callee)
                    if arg_index is None or arg_index >= len(args):
                        continue
                    arg = args[arg_index]
                    if len(arg) == 1 and not arg[0][3] and arg[0][0] in names:
                        position = names.index(arg[0][0])
                        if name not in helpers:
                            helpers[name] = position
                            changed = True
    return helpers


def string_value(token):
    value = token[0]
    # Python 2 permits ur literals; normalize the prefix for Python 3's literal reader.
    value = re.sub(r"^(?:ur|ru)", "r", value, flags=re.I)
    return ast.literal_eval(value)


def aliases(tokens):
    result = {}
    for index in range(len(tokens) - 2):
        if tokens[index + 1][0] != "=" or not re.fullmatch(r"[A-Za-z_]\w*", tokens[index][0]):
            continue
        value = tokens[index + 2]
        if not value[3] and not re.fullmatch(r"[A-Za-z_]\w*", value[0]):
            continue
        expression = [value]
        if index + 4 < len(tokens) and tokens[index + 3][0] == "%":
            expression += tokens[index + 3:index + 5]
        result.setdefault(tokens[index][0], []).append(expression)
    return result


def resolve(expression, bindings, text, seen=frozenset()):
    if len(expression) == 1 and not expression[0][3]:
        name = expression[0][0]
        if name in seen:
            return set()
        candidates = bindings.get(name, [])
        return set().union(*(resolve(candidate, bindings, text, seen | {name}) for candidate in candidates))
    if not expression or not expression[0][3]:
        return set()
    value = string_value(expression[0])
    if not isinstance(value, str) or not value.startswith("SAS_"):
        return set()
    if len(expression) == 1 and NAME_RE.fullmatch(value):
        return {value}
    if len(expression) == 3 and expression[1][0] == "%" and re.fullmatch(r"SAS_[A-Z0-9_]*%d[A-Z0-9_]*", value):
        variable = re.escape(expression[2][0])
        guard = re.search(r"if\s+" + variable + r"\s*<\s*(\d+)\s+or\s+" + variable + r"\s*>\s*(\d+)\s*:\s*return\b", text)
        if guard:
            minimum, maximum = int(guard[1]), int(guard[2])
            if 0 <= maximum - minimum <= 1000:
                return {value % n for n in range(minimum, maximum + 1)}
    raise ValueError(f"unresolved SAS define-name expression: {' '.join(t[0] for t in expression)}")


def bbai_registry_names(tokens):
    # <!-- custom: Searching raw source accepted a commented-out consumer as runtime usage.
    # Match comment-free tokens with string literals masked so neither comments nor diagnostic text can activate an inert registry. (GPT-6.1-Sol) -->
    code = " ".join(t[0] if not t[3] else '""' for t in tokens)
    if not BBAI_REGISTRY_CONSUMER_RE.search(code):
        return set()
    result = set()
    for index in range(len(tokens) - 8):
        values = [tokens[index + offset][0] for offset in range(9)]
        if (values[0] != "{" or not tokens[index + 1][3] or values[2] != "," or values[3] != "&" or
                values[4] != "SASBBAILogSettings" or values[5] != ":" or values[6] != ":" or
                re.fullmatch(r"[A-Za-z_]\w*", values[7]) is None or values[8] != "}"):
            continue
        try:
            name = string_value(tokens[index + 1])
        except (ValueError, SyntaxError):
            continue
        if isinstance(name, str) and NAME_RE.fullmatch(name):
            result.add(name)
    return result


def check(repo):
    root = ET.parse(repo / DEFINES_PATH).getroot()
    declared = set()
    errors = []
    for node in root.iter():
        if local_name(node.tag) != "Define":
            continue
        name = child_text_by_local_name(node, "DefineName") or ""
        if name.startswith("SAS_"):
            if name in declared:
                errors.append(f"{DEFINES_PATH}: duplicate {name}")
            declared.add(name)
    sources = []
    for pattern in SOURCE_GLOBS:
        for path in sorted(repo.glob(pattern)):
            if "temp_files" in path.parts or not path.is_file():
                continue
            text = path.read_text(encoding="utf-8-sig", errors="replace")
            try:
                sources.append((path, text, lex(text, path.suffix == ".py"), path.suffix == ".py"))
            except (tokenize.TokenError, IndentationError, ValueError) as exc:
                errors.append(f"{path.relative_to(repo)}: cannot tokenize: {exc}")
    helpers = forwarding_helpers(sources)
    referenced = set()
    for path, text, tokens, _ in sources:
        bindings = aliases(tokens)
        code = " ".join(t[0] if not t[3] else '""' for t in tokens)
        for index, name, args, _ in calls(tokens):
            position = 0 if GETTER_RE.fullmatch(name) else helpers.get(name)
            if position is None or position >= len(args):
                continue
            try:
                names = resolve(args[position], bindings, code)
            except (ValueError, SyntaxError) as exc:
                errors.append(f"{path.relative_to(repo)}:{tokens[index][1]}: {exc}")
                continue
            for define in sorted(names - declared):
                errors.append(f"{path.relative_to(repo)}:{tokens[index][1]}: {define} has no declaration in {DEFINES_PATH}")
            referenced.update(names)
        if path.relative_to(repo).as_posix() == BBAI_REGISTRY_PATH:
            names = bbai_registry_names(tokens)
            for define in sorted(names - declared):
                errors.append(f"{path.relative_to(repo)}: {define} has no declaration in {DEFINES_PATH}")
            referenced.update(names)
    for name, consumer in METADATA.items():
        if name not in declared:
            errors.append(f"{DEFINES_PATH}: missing coverage metadata {name}")
        elif not (repo / consumer).is_file() or name not in {string_value(t) for t in lex((repo / consumer).read_text(encoding="utf-8"), True) if t[3]}:
            errors.append(f"{name}: documented metadata consumer {consumer} is missing")
    for name in sorted(declared - referenced - METADATA.keys()):
        errors.append(f"{DEFINES_PATH}: {name} has no runtime define lookup")
    return errors, len(declared), len(referenced & declared)


def main():
    parser = argparse.ArgumentParser(description="Check SAS XML define declarations and runtime lookup references in both directions.")
    parser.add_argument("--repo-root", type=Path, default=get_default_repo_root())
    args = parser.parse_args()
    errors, declarations, references = check(args.repo_root)
    for error in errors:
        print(f"FAIL {error}")
    if errors:
        return 1
    print(f"PASS SAS define usage ({declarations} declarations, {references} runtime references, {len(METADATA)} coverage-only metadata define)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
