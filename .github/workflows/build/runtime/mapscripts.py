#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
#
# Verify AdvCiv-SAS PrivateMaps coverage in map-script SAS defines.

from __future__ import annotations

import sys
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[4]
GLOBAL_DEFINES_PATH = REPO_ROOT / "Assets" / "XML" / "GlobalDefines_advciv_sas.xml"
PRIVATE_MAPS_DIR = REPO_ROOT / "PrivateMaps"

# These first three defines are gameplay/DLL map-heaviness override inputs.
GAMEPLAY_HEAVINESS_DEFINE_NAMES = (
    "SAS_MAP_SCRIPT_NAMES_LAND_HEAVY",
    "SAS_MAP_SCRIPT_NAMES_ALMOST_ALL_LAND",
    "SAS_MAP_SCRIPT_NAMES_NAVAL_HEAVY",
)

# This fourth define is coverage/documentation only; the DLL does not need to read it.
COVERAGE_ONLY_DEFINE_NAMES = (
    "SAS_MAP_SCRIPT_NAMES_HEAVINESS_UNSPECIFIED",
)

COVERAGE_DEFINE_NAMES = GAMEPLAY_HEAVINESS_DEFINE_NAMES + COVERAGE_ONLY_DEFINE_NAMES

# Files in PrivateMaps that are not actual selectable/playable map scripts should be added here.
IGNORED_PRIVATE_MAP_FILES = {
    "__init__.py",
}


def xml_local_name(tag: str) -> str:
    if "}" in tag:
        return tag.rsplit("}", 1)[1]
    return tag


def child_text(element: ET.Element, child_name: str) -> str | None:
    for child in element:
        if xml_local_name(child.tag) == child_name:
            return child.text or ""
    return None


def load_define_text_values(path: Path) -> dict[str, str]:
    if not path.is_file():
        raise RuntimeError(f"Missing GlobalDefines file: {path}")

    root = ET.parse(path).getroot()
    values: dict[str, str] = {}
    for element in root.iter():
        if xml_local_name(element.tag) != "Define":
            continue
        name = child_text(element, "DefineName")
        if not name:
            continue
        values[name.strip()] = (child_text(element, "DefineTextVal") or "").strip()
    return values


def normalize_map_script_name(value: str) -> str:
    # SAS XML allows plain script names, .py suffixes, and path-like entries.
    value = value.strip().strip('"\'')
    value = value.replace("\\", "/")
    value = Path(value).name
    if value.lower().endswith(".py"):
        value = value[:-3]
    return value


def split_map_define(value: str) -> list[str]:
    names: list[str] = []
    for raw_token in value.split(","):
        name = normalize_map_script_name(raw_token)
        if name:
            names.append(name)
    return names


def playable_private_map_names() -> set[str]:
    if not PRIVATE_MAPS_DIR.is_dir():
        raise RuntimeError(f"Missing PrivateMaps folder: {PRIVATE_MAPS_DIR}")

    names: set[str] = set()
    for path in PRIVATE_MAPS_DIR.glob("*.py"):
        if path.name in IGNORED_PRIVATE_MAP_FILES:
            continue
        names.add(path.stem)
    return names


def main() -> int:
    try:
        define_values = load_define_text_values(GLOBAL_DEFINES_PATH)
        playable_names = playable_private_map_names()
    except Exception as exc:
        print(f"FAIL map-script SAS define coverage setup: {exc}")
        return 1

    errors: list[str] = []
    missing_defines = [name for name in COVERAGE_DEFINE_NAMES if name not in define_values]
    if missing_defines:
        errors.append("missing coverage define(s): " + ", ".join(missing_defines))

    listed_by_script: dict[str, list[str]] = defaultdict(list)
    duplicate_entries_by_define: dict[str, list[str]] = {}

    for define_name in COVERAGE_DEFINE_NAMES:
        names = split_map_define(define_values.get(define_name, ""))
        seen: set[str] = set()
        duplicates: set[str] = set()
        for script_name in names:
            if script_name in seen:
                duplicates.add(script_name)
            seen.add(script_name)
            listed_by_script[script_name].append(define_name)
        if duplicates:
            duplicate_entries_by_define[define_name] = sorted(duplicates)

    for define_name in sorted(duplicate_entries_by_define):
        errors.append(
            f"{define_name}: duplicate entr{'y' if len(duplicate_entries_by_define[define_name]) == 1 else 'ies'} "
            + ", ".join(duplicate_entries_by_define[define_name])
        )

    for script_name in sorted(playable_names):
        define_names = listed_by_script.get(script_name, [])
        if not define_names:
            errors.append(
                f"PrivateMaps/{script_name}.py: missing from SAS map-script heaviness/coverage defines "
                f"({', '.join(COVERAGE_DEFINE_NAMES)})"
            )
        elif len(define_names) > 1:
            errors.append(
                f"PrivateMaps/{script_name}.py: listed {len(define_names)} times across SAS map-script heaviness/coverage defines "
                f"({', '.join(define_names)})"
            )

    for script_name in sorted(listed_by_script):
        if script_name not in playable_names:
            define_names = ", ".join(listed_by_script[script_name])
            errors.append(
                f"{script_name}: listed in SAS map-script heaviness/coverage define(s) but missing from PrivateMaps/ "
                f"({define_names})"
            )

    if errors:
        print("FAIL map-script SAS define coverage")
        for error in errors:
            print(f"  - {error}")
        return 1

    print(
        f"PASS map-script SAS define coverage: {len(playable_names)} playable PrivateMaps scripts "
        f"listed exactly once across {len(COVERAGE_DEFINE_NAMES)} coverage define(s)."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
