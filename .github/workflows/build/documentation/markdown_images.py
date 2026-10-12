#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Verify that local images referenced by Markdown files exist in the repository.

from __future__ import annotations

import html
import re
import sys
from pathlib import Path
from urllib.parse import unquote, urlsplit


# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
ROOT = Path(__file__).resolve().parents[4]

HTML_IMG_RE = re.compile(
    r"<img\b[^>]*?\bsrc\s*=\s*(?:\"([^\"]*)\"|'([^']*)'|([^\s>]+))",
    re.IGNORECASE,
)
REFERENCE_DEF_RE = re.compile(r"^\s{0,3}\[([^\]]+)\]:\s*(.+?)\s*$", re.MULTILINE)
FENCE_RE = re.compile(r"^\s{0,3}(`{3,}|~{3,})")
SCHEME_RE = re.compile(r"^[A-Za-z][A-Za-z0-9+.-]*:")


def without_fenced_code(text: str) -> str:
    """Blank fenced-code contents while preserving line numbers."""
    result: list[str] = []
    fence_char = ""
    fence_len = 0

    for line in text.splitlines(keepends=True):
        match = FENCE_RE.match(line)
        if match:
            marker = match.group(1)
            if not fence_char:
                # A matching marker later on the same line is a multi-backtick/tilde
                # inline code span, not the start of a fenced code block.
                if line.find(marker, match.end()) >= 0:
                    result.append(line)
                    continue
                fence_char = marker[0]
                fence_len = len(marker)
                result.append("\n" if line.endswith("\n") else "")
                continue
            if marker[0] == fence_char and len(marker) >= fence_len:
                fence_char = ""
                fence_len = 0
                result.append("\n" if line.endswith("\n") else "")
                continue

        if fence_char:
            result.append("\n" if line.endswith("\n") else "")
        else:
            result.append(line)

    return "".join(result)


def strip_html_comments(text: str) -> str:
    """Blank HTML comments while preserving line numbers."""

    def repl(match: re.Match[str]) -> str:
        value = match.group(0)
        return "\n" * value.count("\n")

    return re.sub(r"<!--.*?-->", repl, text, flags=re.DOTALL)


def without_inline_code(text: str) -> str:
    """Blank Markdown backtick code spans while preserving line numbers."""
    result: list[str] = []
    index = 0

    while index < len(text):
        if text[index] != "`":
            result.append(text[index])
            index += 1
            continue

        run_end = index + 1
        while run_end < len(text) and text[run_end] == "`":
            run_end += 1
        marker = text[index:run_end]
        close = text.find(marker, run_end)
        if close < 0:
            result.append(text[index:run_end])
            index = run_end
            continue

        span_end = close + len(marker)
        value = text[index:span_end]
        result.append("\n" * value.count("\n"))
        index = span_end

    return "".join(result)


def split_destination(value: str) -> str:
    """Return a Markdown link destination without an optional title."""
    value = value.strip()
    if not value:
        return ""
    if value.startswith("<"):
        end = value.find(">")
        return value[1:end] if end >= 0 else value[1:]

    escaped = False
    depth = 0
    for index, char in enumerate(value):
        if escaped:
            escaped = False
            continue
        if char == "\\":
            escaped = True
            continue
        if char == "(":
            depth += 1
            continue
        if char == ")" and depth:
            depth -= 1
            continue
        if char.isspace() and depth == 0:
            return value[:index]
    return value


def markdown_reference_definitions(text: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for match in REFERENCE_DEF_RE.finditer(text):
        label = " ".join(match.group(1).strip().lower().split())
        destination = split_destination(match.group(2))
        if destination:
            result[label] = destination
    return result


def matching_bracket(text: str, start: int, opening: str, closing: str) -> int:
    depth = 1
    escaped = False
    index = start + 1
    while index < len(text):
        char = text[index]
        if escaped:
            escaped = False
        elif char == "\\":
            escaped = True
        elif char == opening:
            depth += 1
        elif char == closing:
            depth -= 1
            if depth == 0:
                return index
        index += 1
    return -1


def markdown_images(text: str):
    """Yield (offset, destination) for inline and reference-style Markdown images."""
    refs = markdown_reference_definitions(text)
    index = 0

    while True:
        start = text.find("![", index)
        if start < 0:
            return

        alt_end = matching_bracket(text, start + 1, "[", "]")
        if alt_end < 0:
            return

        cursor = alt_end + 1
        while cursor < len(text) and text[cursor] in " \t":
            cursor += 1

        if cursor < len(text) and text[cursor] == "(":
            target_end = matching_bracket(text, cursor, "(", ")")
            if target_end >= 0:
                destination = split_destination(text[cursor + 1 : target_end])
                if destination:
                    yield start, destination
                index = target_end + 1
                continue

        if cursor < len(text) and text[cursor] == "[":
            label_end = matching_bracket(text, cursor, "[", "]")
            if label_end >= 0:
                label = text[cursor + 1 : label_end]
                if not label:
                    label = text[start + 2 : alt_end]
                key = " ".join(label.strip().lower().split())
                destination = refs.get(key)
                if destination:
                    yield start, destination
                index = label_end + 1
                continue

        # Shortcut reference image: ![label] with [label]: destination elsewhere.
        key = " ".join(text[start + 2 : alt_end].strip().lower().split())
        destination = refs.get(key)
        if destination:
            yield start, destination

        index = alt_end + 1


def html_images(text: str):
    for match in HTML_IMG_RE.finditer(text):
        destination = next(group for group in match.groups() if group is not None)
        if destination:
            yield match.start(), destination


def is_local(destination: str) -> bool:
    value = html.unescape(destination.strip())
    return bool(value) and not value.startswith(("#", "//")) and not SCHEME_RE.match(value)


def resolve_local_image(md_path: Path, destination: str) -> Path | None:
    value = html.unescape(destination.strip()).replace("\\ ", " ")
    if not is_local(value):
        return None

    # Queries/fragments are URL metadata, not filesystem path components.
    parsed = urlsplit(value)
    path_text = unquote(parsed.path)
    if not path_text:
        return None

    if path_text.startswith("/"):
        candidate = ROOT / path_text.lstrip("/")
    else:
        candidate = md_path.parent / path_text
    return candidate.resolve()


def line_number(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def main() -> int:
    errors: list[str] = []
    checked = 0
    markdown_files = 0

    for md_path in sorted(ROOT.rglob("*.md")):
        text = md_path.read_text(encoding="utf-8")
        scan_text = without_inline_code(strip_html_comments(without_fenced_code(text)))
        found_local = False

        references = list(markdown_images(scan_text)) + list(html_images(scan_text))
        references.sort(key=lambda item: item[0])
        for offset, destination in references:
            target = resolve_local_image(md_path, destination)
            if target is None:
                continue
            checked += 1
            found_local = True

            try:
                target.relative_to(ROOT)
            except ValueError:
                errors.append(
                    f"{md_path.relative_to(ROOT)}:{line_number(scan_text, offset)}: "
                    f"local image escapes repository: {destination}"
                )
                continue

            if not target.is_file():
                errors.append(
                    f"{md_path.relative_to(ROOT)}:{line_number(scan_text, offset)}: "
                    f"missing local image: {destination} -> {target.relative_to(ROOT)}"
                )

        if found_local:
            markdown_files += 1

    if errors:
        print("Broken local Markdown image references:")
        for error in errors:
            print(f"  {error}")
        print(f"\nChecked {checked} local image reference(s) in {markdown_files} Markdown file(s).")
        return 1

    print(f"Markdown local images OK: {checked} reference(s) in {markdown_files} file(s).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
