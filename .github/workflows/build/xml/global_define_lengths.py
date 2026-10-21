#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Keep all shipped GlobalDefine names/text within the inherited fixed XML scratch buffers; the same loader reads inherited and SAS data. See KI#1001. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->

# <!-- custom: pathlib locates shipped XML, argparse exposes the repository override, sys loads shared helpers, and ElementTree decodes the XML fields before byte counting. (GPT-6.1-Sol) -->
from pathlib import Path
import argparse
# <!-- custom: re recognizes numbered _PART_N families across all loaded define names, without hardcoding the map-list prefix. (GPT-6.1-Sol) -->
import re
import sys
import xml.etree.ElementTree as ET

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (ChatGPT-5.6-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

# <!-- custom: Reuse xml_defines for repository discovery and namespace-safe tag names, but retain raw field whitespace for the buffer-capacity check. (GPT-6.1-Sol) -->
from xml_defines import get_default_repo_root, local_name


MAX_DEFINE_BYTES = 255
ROOT_PATTERNS = ("*GlobalDefines*.xml", "PythonCallbackDefines.xml")
MODULE_PATTERNS = ("*_GlobalDefines.xml", "*_PythonCallbackDefines.xml")
PART_NAME = re.compile(r"^(.+)_PART_([0-9]+)$")


def loaded_define_paths(repo_root: Path) -> list[Path]:
    paths: set[Path] = set()
    xml_root = repo_root / "Assets" / "XML"
    for pattern in ROOT_PATTERNS:
        paths.update(xml_root.glob(pattern))

    modules_root = repo_root / "Assets" / "Modules"
    if modules_root.is_dir():
        for pattern in MODULE_PATTERNS:
            paths.update(modules_root.rglob(pattern))

    return sorted(path for path in paths if path.is_file())


def utf8_size(value: str) -> int:
    return len(value.encode("utf-8"))


# <!-- custom: Do not use the shared stripped-text reader here: surrounding spaces still contribute bytes unless the closed FXML interface is proven to trim them before copying.
# Count the full decoded field conservatively instead of allowing whitespace to hide an over-capacity input. (GPT-6.1-Sol) -->
def raw_child_text(define: ET.Element, tag: str) -> str | None:
    for child in define:
        if local_name(child.tag) == tag:
            return child.text or ""
    return None


# <!-- custom: The initial split paired an unsuffixed base key with _PART_2, hiding that the base was really the first part.
# A single value uses an unsuffixed name; a split family of two or more values uses consecutive numbered parts throughout, with no competing unsuffixed key.
# Collect names across files because inherited overrides and modular definitions share the variable system.
# Check only declared names: this is a naming contract, not proof that a C++ consumer reads every part. (GPT-6.1-Sol) -->
def part_family_errors(names: dict[str, Path]) -> list[str]:
    families: dict[str, set[int]] = {}
    failures = []
    for name, path in sorted(names.items()):
        match = PART_NAME.fullmatch(name)
        if match is None:
            continue
        base, digits = match.groups()
        number = int(digits)
        if number < 1 or digits != str(number):
            failures.append(f"{path}: {name} must use a positive, unpadded _PART_N suffix starting at _PART_1")
        families.setdefault(base, set()).add(number)
    for base, parts in sorted(families.items()):
        if base in names:
            failures.append(f"{names[base]}: {base} coexists with numbered parts; every member of a split family must be numbered, without an unsuffixed base")
        elif len(parts) == 1:
            failures.append(f"{base}: only one numbered key exists; use the unsuffixed name {base} for a single value instead of part numbering")
            continue
        for expected, number in enumerate(sorted(part for part in parts if part >= 1), 1):
            if number != expected:
                failures.append(f"{base}: numbered define family is missing {base}_PART_{expected}; parts must start at 1 without gaps")
                break
    return failures


def check(repo_root: Path) -> tuple[list[str], dict[str, object]]:
    failures: list[str] = []
    paths = loaded_define_paths(repo_root)
    if not paths:
        return ["no shipped GlobalDefines/PythonCallbackDefines XML files found"], {}

    define_count = 0
    max_name: tuple[int, str, Path] = (0, "", Path())
    max_text: tuple[int, str, Path] = (0, "", Path())
    names: dict[str, Path] = {}

    for path in paths:
        relative = path.relative_to(repo_root)
        try:
            root = ET.parse(path).getroot()
        except ET.ParseError as exc:
            failures.append(f"{relative}: XML parse error: {exc}")
            continue

        for index, define in enumerate((node for node in root.iter() if local_name(node.tag) == "Define"), 1):
            define_count += 1
            name = raw_child_text(define, "DefineName") or ""
            name_bytes = utf8_size(name)
            label = name.strip() or f"<Define> #{index}"
            if name.strip():
                names.setdefault(name.strip(), relative)
            if name_bytes > max_name[0]:
                max_name = (name_bytes, label, relative)
            if name_bytes > MAX_DEFINE_BYTES:
                failures.append(f"{relative}: {label} DefineName is {name_bytes} UTF-8 bytes; maximum is {MAX_DEFINE_BYTES}")

            text_value = raw_child_text(define, "DefineTextVal")
            if text_value is None:
                continue
            text_bytes = utf8_size(text_value)
            if text_bytes > max_text[0]:
                max_text = (text_bytes, label, relative)
            if text_bytes > MAX_DEFINE_BYTES:
                failures.append(f"{relative}: {label} DefineTextVal is {text_bytes} UTF-8 bytes; maximum is {MAX_DEFINE_BYTES}")

    failures.extend(part_family_errors(names))
    stats = {
        "file_count": len(paths),
        "define_count": define_count,
        "max_name": max_name,
        "max_text": max_text,
    }
    return failures, stats


def main() -> int:
    parser = argparse.ArgumentParser(description="Check GlobalDefine name/text lengths against Civ4's inherited 256-byte XML scratch buffers.")
    parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
    args = parser.parse_args()

    failures, stats = check(args.repo_root)
    if failures:
        print("FAIL GlobalDefine fixed-buffer lengths")
        for failure in failures:
            print(f"  - {failure}")
        return 1

    max_name = stats["max_name"]
    max_text = stats["max_text"]
    print(
        "PASS GlobalDefine fixed-buffer lengths: "
        f"{stats['define_count']} defines across {stats['file_count']} file(s); "
        f"longest name={max_name[0]} bytes ({max_name[1]} in {max_name[2]}), "
        f"longest text={max_text[0]} bytes ({max_text[1]} in {max_text[2]})."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
