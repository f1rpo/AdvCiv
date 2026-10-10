#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Verify that local files/directories referenced by Markdown links exist in the repository,
# and that fragments targeting Markdown files resolve to GitHub-style heading/custom anchors.

from __future__ import annotations

import html
import re
import sys
import unicodedata
from pathlib import Path
from urllib.parse import unquote, urlsplit


ROOT = Path(__file__).resolve().parents[3]
MANIFEST_PATH = ROOT / "_LLM_REPO_FILE_MANIFEST.txt"
KNOWN_ISSUES_PATH = ROOT / "_1_AdvCiv-SAS" / "Docs" / "README_Known_Issues.md"
CPP_AUDIT_ALBUM_PATH = ROOT / "_1_AdvCiv-SAS" / "Docs" / "Source_Analysis" / "cpp_file_audit_album.txt"

HTML_LINK_RE = re.compile(
    r"<a\b[^>]*?\bhref\s*=\s*(?:\"([^\"]*)\"|'([^']*)'|([^\s>]+))",
    re.IGNORECASE,
)
HTML_ANCHOR_RE = re.compile(
    r"<a\b[^>]*?\b(?:name|id)\s*=\s*(?:\"([^\"]+)\"|'([^']+)'|([^\s>]+))",
    re.IGNORECASE,
)
REFERENCE_DEF_RE = re.compile(r"^\s{0,3}\[([^\]]+)\]:\s*(.+?)\s*$", re.MULTILINE)
ATX_HEADING_RE = re.compile(r"^\s{0,3}#{1,6}(?:[ \t]+(.*?)|[ \t]*)$", re.MULTILINE)
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


def markdown_links(text: str):
    """Yield (offset, destination) for inline and explicit reference-style links."""
    refs = markdown_reference_definitions(text)
    index = 0

    while True:
        start = text.find("[", index)
        if start < 0:
            return
        if start > 0 and text[start - 1] == "!":
            index = start + 1
            continue

        label_end = matching_bracket(text, start, "[", "]")
        if label_end < 0:
            return

        cursor = label_end + 1
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
            ref_end = matching_bracket(text, cursor, "[", "]")
            if ref_end >= 0:
                label = text[cursor + 1 : ref_end]
                if not label:
                    label = text[start + 1 : label_end]
                key = " ".join(label.strip().lower().split())
                destination = refs.get(key)
                if destination:
                    yield start, destination
                index = ref_end + 1
                continue

        index = label_end + 1


def html_links(text: str):
    for match in HTML_LINK_RE.finditer(text):
        destination = next(group for group in match.groups() if group is not None)
        if destination:
            yield match.start(), destination


def is_local(destination: str) -> bool:
    value = html.unescape(destination.strip())
    return bool(value) and not value.startswith("//") and not SCHEME_RE.match(value)


def resolve_local_link(md_path: Path, destination: str) -> tuple[Path | None, str]:
    value = html.unescape(destination.strip()).replace("\\ ", " ")
    if not is_local(value):
        return None, ""

    parsed = urlsplit(value)
    path_text = unquote(parsed.path)
    fragment = unquote(parsed.fragment)

    if not path_text:
        return (md_path.resolve() if fragment else None), fragment

    if path_text.startswith("/"):
        candidate = ROOT / path_text.lstrip("/")
    else:
        candidate = md_path.parent / path_text
    return candidate.resolve(), fragment


def manifest_repository_paths() -> tuple[set[str], set[str]]:
    """Read archive-only git ls-files data when running inside a light-source ZIP."""
    if not MANIFEST_PATH.is_file():
        return set(), set()

    files: set[str] = set()
    in_tracked_files = False
    for line in MANIFEST_PATH.read_text(encoding="utf-8").splitlines():
        if line == "[TRACKED FILES - git ls-files]":
            in_tracked_files = True
            continue
        if in_tracked_files and line.startswith("["):
            break
        if in_tracked_files and line and not line.startswith("#"):
            files.add(line)

    directories: set[str] = set()
    for file_path in files:
        parts = file_path.split("/")
        for index in range(1, len(parts)):
            directories.add("/".join(parts[:index]))
    return files, directories


def repository_path_exists(target: Path, manifest_files: set[str], manifest_directories: set[str]) -> bool:
    if target.exists():
        return True
    try:
        relative = target.relative_to(ROOT).as_posix()
    except ValueError:
        return False
    return relative in manifest_files or relative in manifest_directories


def heading_plain_text(value: str) -> str:
    """Approximate the rendered heading text GitHub uses before generating its anchor."""
    protected: list[str] = []

    def protect_code(match: re.Match[str]) -> str:
        protected.append(match.group(2))
        return f"\x00{len(protected) - 1}\x00"

    value = re.sub(r"(`+)(.*?)\1", protect_code, value)
    value = re.sub(r"[ \t]+#+[ \t]*$", "", value)
    value = re.sub(r"!\[([^\]]*)\]\([^\)]*\)", r"\1", value)
    value = re.sub(r"!\[([^\]]*)\]\[[^\]]*\]", r"\1", value)
    value = re.sub(r"\[([^\]]+)\]\([^\)]*\)", r"\1", value)
    value = re.sub(r"\[([^\]]+)\]\[[^\]]*\]", r"\1", value)
    value = re.sub(r"<[^>]+>", "", value)
    value = value.replace("*", "").replace("~", "")
    value = re.sub(r"(?<!\w)_([^_]+)_(?!\w)", r"\1", value)
    value = re.sub(r"\\([\\`*{}\[\]()#+\-.!_>])", r"\1", value)

    for index, code in enumerate(protected):
        value = value.replace(f"\x00{index}\x00", code)
    return html.unescape(value).strip()


def github_heading_slug(value: str) -> str:
    """Generate the basic GitHub heading slug before duplicate-heading numbering."""
    result: list[str] = []
    for char in heading_plain_text(value).lower():
        if char == " ":
            result.append("-")
            continue
        if char.isspace():
            continue
        if char in "-_":
            result.append(char)
            continue
        category = unicodedata.category(char)
        if category.startswith(("P", "S", "C")):
            continue
        result.append(char)
    return "".join(result)


def markdown_anchors(md_path: Path) -> set[str]:
    """Collect GitHub-style ATX heading anchors plus explicit HTML custom anchors."""
    text = md_path.read_text(encoding="utf-8")
    scan_text = strip_html_comments(without_fenced_code(text))
    anchors: set[str] = set()
    seen: dict[str, int] = {}

    for match in ATX_HEADING_RE.finditer(scan_text):
        base = github_heading_slug(match.group(1) or "")
        anchor = base
        if anchor in seen:
            number = seen.get(base, 0) + 1
            while f"{base}-{number}" in seen:
                number += 1
            anchor = f"{base}-{number}"
            seen[base] = number
        seen[anchor] = 0
        anchors.add(anchor)

    for match in HTML_ANCHOR_RE.finditer(scan_text):
        anchor = next(group for group in match.groups() if group is not None)
        anchors.add(html.unescape(anchor))

    return anchors


def line_number(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


# <!-- custom: A valid heading anchor does not ensure that a Known Issues entry is discoverable from its manually maintained menu.
# Require the menu and body to use the same unique KI identifiers, and require every integer through the highest documented or album-assigned KI number to have an explicit Fixed/Pending/Merged/Rejected/Retired entry.
# This prevents provisional findings such as KI#501 from disappearing between the audit album and the public implementation queue. (GPT-5.6-Sol) -->
def known_issues_menu_errors() -> tuple[list[str], int]:
    text = KNOWN_ISSUES_PATH.read_text(encoding="utf-8")
    first_issue = re.search(r"^##\s+KI#\d", text, re.MULTILINE)
    if first_issue is None:
        return [f"{KNOWN_ISSUES_PATH.relative_to(ROOT)}: no numbered KI headings found"], 0

    menu_text = text[: first_issue.start()]
    menu_ids = re.findall(r"^\[KI#(\d+(?:\.\d+)*)\b", menu_text, re.MULTILINE)
    issue_matches = list(re.finditer(r"^##\s+KI#(\d+(?:\.\d+)*)\b", text[first_issue.start() :], re.MULTILINE))
    issue_ids = [match.group(1) for match in issue_matches]
    errors: list[str] = []
    for match in re.finditer(r"^(?:##\s+|\[)(\d+(?:\.\d+)*)\s+-", text, re.MULTILINE):
        errors.append(
            f"{KNOWN_ISSUES_PATH.relative_to(ROOT)}:{line_number(text, match.start())}: "
            f"legacy KI#{match.group(1)} label; use the greppable KI#number schema"
        )
    for location, identifiers in (("main menu", menu_ids), ("body", issue_ids)):
        duplicates = sorted(identifier for identifier in set(identifiers) if identifiers.count(identifier) > 1)
        for identifier in duplicates:
            errors.append(f"{KNOWN_ISSUES_PATH.relative_to(ROOT)}: duplicate KI#{identifier} in {location}")

    menu_set = set(menu_ids)
    issue_set = set(issue_ids)
    for identifier in sorted(issue_set - menu_set, key=lambda value: [int(part) for part in value.split(".")]):
        errors.append(f"{KNOWN_ISSUES_PATH.relative_to(ROOT)}: KI#{identifier} heading is missing from the main menu")
    for identifier in sorted(menu_set - issue_set, key=lambda value: [int(part) for part in value.split(".")]):
        errors.append(f"{KNOWN_ISSUES_PATH.relative_to(ROOT)}: KI#{identifier} main-menu entry has no body heading")

    for identifier in issue_set:
        expected = rf'^<a id="ki-{re.escape(identifier)}"></a>\n\n##\s+KI#{re.escape(identifier)}\b'
        if re.search(expected, text, re.MULTILINE) is None:
            errors.append(f"{KNOWN_ISSUES_PATH.relative_to(ROOT)}: KI#{identifier} lacks its stable #ki-{identifier} anchor")
    for identifier, fragment in re.findall(
        r"^\[KI#(\d+(?:\.\d+)*)\b.*?\]\([^\r\n#]+#([^\r\n)]+)\)", menu_text, re.MULTILINE
    ):
        if fragment != f"ki-{identifier}":
            errors.append(
                f"{KNOWN_ISSUES_PATH.relative_to(ROOT)}: KI#{identifier} main-menu link uses #{fragment}; "
                f"expected stable #ki-{identifier}"
            )

    integer_ids = {int(identifier) for identifier in issue_set if "." not in identifier}
    album_text = CPP_AUDIT_ALBUM_PATH.read_text(encoding="utf-8")
    # <!-- custom: Audit checkpoints reserve the next F/KI pair while marking it UNUSED.
    # Only a promoted provisional finding extends the KI ledger; otherwise every open cursor would require a fictitious issue entry. (GPT-5.6-Sol) -->
    album_ids = {int(number) for number in re.findall(r"^F\d+\s+/\s+(?:PROVISIONAL|provisional)\s+KI#(\d+)\b(?![^\r\n]*\bUNUSED\b)", album_text, re.MULTILINE)}
    highest = max(integer_ids | album_ids)
    for number in sorted(set(range(1, highest + 1)) - integer_ids):
        errors.append(
            f"{KNOWN_ISSUES_PATH.relative_to(ROOT)}: KI#{number} is missing from the continuous "
            f"integer ledger through KI#{highest}; add its Pending/Merged/Rejected/Retired disposition"
        )
    return errors, len(issue_set)


def main() -> int:
    errors: list[str] = []
    checked = 0
    checked_fragments = 0
    markdown_files = 0
    anchor_cache: dict[Path, set[str]] = {}
    manifest_files, manifest_directories = manifest_repository_paths()

    for md_path in sorted(ROOT.rglob("*.md")):
        text = md_path.read_text(encoding="utf-8")
        scan_text = without_inline_code(strip_html_comments(without_fenced_code(text)))
        found_local = False

        references = list(markdown_links(scan_text)) + list(html_links(scan_text))
        references.sort(key=lambda item: item[0])
        for offset, destination in references:
            target, fragment = resolve_local_link(md_path, destination)
            if target is None:
                continue
            checked += 1
            found_local = True

            try:
                relative_target = target.relative_to(ROOT)
            except ValueError:
                errors.append(
                    f"{md_path.relative_to(ROOT)}:{line_number(scan_text, offset)}: "
                    f"local link escapes repository: {destination}"
                )
                continue

            if not repository_path_exists(target, manifest_files, manifest_directories):
                errors.append(
                    f"{md_path.relative_to(ROOT)}:{line_number(scan_text, offset)}: "
                    f"missing local link target: {destination} -> {relative_target}"
                )
                continue

            if fragment and target.suffix.lower() == ".md" and target.is_file():
                checked_fragments += 1
                anchors = anchor_cache.get(target)
                if anchors is None:
                    anchors = markdown_anchors(target)
                    anchor_cache[target] = anchors
                if fragment not in anchors:
                    errors.append(
                        f"{md_path.relative_to(ROOT)}:{line_number(scan_text, offset)}: "
                        f"missing Markdown anchor: {destination} -> {relative_target}#{fragment}"
                    )

        if found_local:
            markdown_files += 1

    menu_errors, known_issue_count = known_issues_menu_errors()
    errors.extend(menu_errors)

    if errors:
        print("Markdown validation errors:")
        for error in errors:
            print(f"  {error}")
        print(
            f"\nChecked {checked} local link reference(s), including {checked_fragments} Markdown "
            f"fragment(s), in {markdown_files} Markdown file(s)."
        )
        return 1

    print(
        f"Markdown local links OK: {checked} reference(s), including {checked_fragments} Markdown "
        f"fragment(s), in {markdown_files} file(s)."
    )
    print(f"Known Issues main menu complete: {known_issue_count} numbered KI identifier(s) indexed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
