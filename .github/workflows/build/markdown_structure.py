#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Valid link anchors previously let missing menu entries and mismatched titles pass. Check declared menus against body headings, including hierarchy and ordering; Known Issues indexes numbered issue headings rather than every internal investigation subsection. (GPT-6.1-Sol) -->
import argparse
from collections import Counter
from pathlib import Path
import re
import posixpath
import sys
from urllib.parse import unquote, urlsplit
import markdown_links as links

ROOT = Path(__file__).resolve().parents[3]
HEADING = re.compile(r"^(#{1,6})[ \t]+([^\r\n]+?)[ \t]*$", re.MULTILINE)
MENU = {"menu", "contents", "table of contents"}

def scan(text):
    return links.strip_html_comments(links.without_fenced_code(text))

def headings(text):
    clean = scan(text)
    seen = {}
    result = []
    for match in HEADING.finditer(clean):
        title = match.group(2)
        base = links.github_heading_slug(title)
        slug = base
        if slug in seen:
            number = seen.get(base, 0) + 1
            while f"{base}-{number}" in seen:
                number += 1
            slug = f"{base}-{number}"
            seen[base] = number
        seen[slug] = 0
        preceding = clean[max(0, match.start() - 500):match.start()]
        custom = re.search(r'<a (?:id|name)="([^"\n]+)"></a>\s*$', preceding)
        result.append((match.start(), match.end(), len(match.group(1)), title, custom.group(1) if custom else slug, slug))
    return result

def menu_model(text, known_issues=False):
    hs = headings(text)
    menus = [h for h in hs if links.heading_plain_text(h[3]).lower() in MENU]
    if not menus:
        return None
    if len(menus) != 1:
        raise ValueError("expected one declared Menu/Contents section")
    menu = menus[0]
    following = [h for h in hs if h[0] > menu[0]]
    if not following:
        raise ValueError("menu has no body headings")
    body = [h for h in following if not known_issues or (h[2] == 2 and re.match(r"KI#\d", h[3]))]
    if not body:
        raise ValueError("menu has no indexed body headings")
    end = following[0][0]
    custom = re.search(r'<a (?:id|name)="[^"\n]+"></a>\s*$', scan(text)[:end])
    if custom:
        end = custom.start()
    return menu[1], end, body

def is_self_link(destination, document):
    parsed = urlsplit(destination)
    if parsed.scheme or parsed.netloc or not parsed.fragment:
        return False
    path = unquote(parsed.path)
    if not path or document is None:
        return True
    target = path.lstrip("/") if path.startswith("/") else posixpath.join(posixpath.dirname(document), path)
    return posixpath.normpath(target) == document

def menu_errors(text, known_issues=False, document=None):
    model = menu_model(text, known_issues)
    if model is None:
        return []
    start, end, body = model
    by_anchor = {anchor: h for h in body for anchor in (h[4], h[5])}
    base_level = min(h[2] for h in body)
    errors = []
    found = []
    for line in scan(text)[start:end].splitlines():
        parsed = list(links.markdown_links(line))
        if not parsed:
            continue
        if len(parsed) != 1:
            errors.append("menu row must contain one link")
            continue
        offset, destination = parsed[0]
        fragment = unquote(urlsplit(destination).fragment)
        if not is_self_link(destination, document):
            continue
        label_end = links.matching_bracket(line, offset, "[", "]")
        label = line[offset + 1:label_end]
        h = by_anchor.get(fragment)
        if h is None:
            errors.append(f"menu target #{fragment} has no indexed body heading")
            continue
        found.append(h[4])
        if links.heading_plain_text(label) != links.heading_plain_text(h[3]):
            errors.append(f"#{fragment}: menu title differs from heading {h[3]!r}")
        prefix = line[:offset]
        if "&emsp;" in prefix:
            depth = prefix.count("&emsp;")
        elif re.match(r"^\s*[-*+] ", prefix):
            depth = len(prefix) - len(prefix.lstrip())
            if depth % 2:
                errors.append(f"#{fragment}: bullet indentation must use two spaces per heading level")
            depth //= 2
        else:
            depth = 0
        if depth != h[2] - base_level:
            errors.append(f"#{fragment}: menu depth {depth}, expected {h[2] - base_level}")
    expected = [h[4] for h in body]
    for anchor in sorted(set(expected) - set(found)):
        errors.append(f"#{anchor}: body heading missing from menu")
    for anchor, count in Counter(found).items():
        if count > 1:
            errors.append(f"#{anchor}: duplicate menu entry")
    if set(found) == set(expected) and found != expected:
        errors.append("menu order differs from body heading order")
    return errors

def bold_errors(text):
    clean = scan(text)
    clean = re.sub(r"(`+)(.*?)\1", lambda m: "".join("\n" if c == "\n" else " " for c in m.group(0)), clean, flags=re.DOTALL)
    errors = []
    for paragraph_match in re.finditer(r"(?:[^\n]|\n(?![ \t]*\n))+", clean):
        paragraph = paragraph_match.group(0)
        offset = paragraph_match.start()
        markers = list(re.finditer(r"(?<![\\*])\*{2,3}(?!\*)", paragraph))
        if len(markers) % 2:
            line = links.line_number(clean, offset + markers[0].start())
            errors.append(f"line {line}: unpaired ** bold marker in paragraph")
    return errors

def redundant_hard_break_errors(text):
    visible = scan(text)
    lines = visible.splitlines()
    clean_lines = links.without_inline_code(visible).splitlines()
    errors = []
    # <!-- custom: The first check mistook escaped literal backslashes for hard breaks and erased inline-code-only following lines into apparent blanks. Check odd trailing backslash runs outside code, but retain inline code when deciding whether the next line has content. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
    for index, line in enumerate(lines):
        if not re.search(r"(?<!\\)(?:\\\\)*\\$", line):
            continue
        if index >= len(clean_lines) or not clean_lines[index].endswith("\\"):
            continue
        if index + 1 >= len(lines) or not lines[index + 1].strip():
            errors.append(f"line {index + 1}: trailing Markdown hard break is redundant before a blank line or end of file")
    return errors

def render_menu(text, known_issues=False, document=None):
    model = menu_model(text, known_issues)
    if model is None:
        return text
    start, end, body = model
    base_level = min(h[2] for h in body)
    bullet = bool(re.search(r"^\s*[-*+] ", text[start:end], re.MULTILINE))
    destinations = {}
    external_after = {}
    previous = ""
    valid = {a: h[4] for h in body for a in (h[4], h[5])}
    for line in text[start:end].splitlines():
        parsed = list(links.markdown_links(line))
        if len(parsed) != 1:
            continue
        destination = parsed[0][1]
        anchor = unquote(urlsplit(destination).fragment)
        if is_self_link(destination, document) and anchor in valid:
            previous = valid[anchor]
            destinations[previous] = destination
        elif not is_self_link(destination, document):
            external_after.setdefault(previous, []).append(line)
    prose = [line for line in text[start:end].splitlines() if line.strip() and not list(links.markdown_links(line))]
    rows = prose + ([""] if prose else []) + list(external_after.get("", []))
    for _, _, level, title, anchor, _ in body:
        label = title
        prefix = "  " * (level - base_level) + "- " if bullet else "&emsp;" * (level - base_level)
        destination = destinations.get(anchor, "#" + anchor)
        rows.append(f"{prefix}[{label}]({destination})" + ("" if bullet else "\\"))
        rows.extend(external_after.get(anchor, []))
    # <!-- custom: A trailing backslash only forces a hard break before another line. Drop it from the final menu row instead of emitting redundant Markdown before the section-ending blank line. (ChatGPT-5.6-Sol) -->
    if not bullet:
        for index in range(len(rows) - 1, -1, -1):
            if not rows[index].strip():
                continue
            if rows[index].endswith("\\"):
                rows[index] = rows[index][:-1]
            break
    return text[:start] + "\n\n" + "\n".join(rows) + "\n\n" + text[end:]

def paths(root):
    return sorted(set([root / "README.md", root / "AGENTS.md", root / "LLM_Helpers/README.md", root / ".github/workflows/README.md"] + list((root / "_1_AdvCiv-SAS/Docs").rglob("*.md"))))

def main():
    parser = argparse.ArgumentParser(description="Validate declared Markdown menus and balanced bold markers in maintained docs.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--refresh-menus", action="store_true")
    args = parser.parse_args()
    errors = []
    for path in paths(args.repo_root):
        text = path.read_text("utf-8")
        known = path.name == "README_Known_Issues.md"
        document = path.relative_to(args.repo_root).as_posix()
        if args.refresh_menus:
            updated = render_menu(text, known, document)
            if updated != text:
                old = path.read_bytes()
                newline = "\r\n" if old.count(b"\r\n") > old.count(b"\n") // 2 else "\n"
                path.write_bytes(updated.replace("\n", newline).encode("utf-8"))
                text = updated
        try:
            findings = menu_errors(text, known, document) + bold_errors(text) + redundant_hard_break_errors(text)
        except ValueError as error:
            findings = [str(error)]
        errors.extend(f"{path.relative_to(args.repo_root)}: {error}" for error in findings)
    print("FAIL Markdown structure" if errors else "PASS Markdown structure")
    for error in errors:
        print("  - " + error)
    return int(bool(errors))

if __name__ == "__main__":
    sys.exit(main())
