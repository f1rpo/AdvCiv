#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Stray-text checks accepted loose Type/NIF/Button elements when BuildingArtInfo was missing.
# Validate the ArtDefines collection and entry wrappers as well; the Tipi's missing wrapper caused a startup crash. (GPT-6.1-Sol) -->
import argparse
from pathlib import Path
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
COLLECTIONS = {"BonusArtInfos": "BonusArtInfo", "BuildingArtInfos": "BuildingArtInfo", "CivilizationArtInfos": "CivilizationArtInfo", "FeatureArtInfos": "FeatureArtInfo", "ImprovementArtInfos": "ImprovementArtInfo", "InterfaceArtInfos": "InterfaceArtInfo", "LeaderheadArtInfos": "LeaderheadArtInfo", "MiscArtInfos": "MiscArtInfo", "MovieArtInfos": "MovieArtInfo", "UnitArtInfos": "UnitArtInfo"}

def name(node):
    return node.tag.rsplit("}", 1)[-1]

def check_tree(root, expected_collection=None):
    errors = []
    if name(root) != "Civ4ArtDefines":
        errors.append("expected Civ4ArtDefines root")
    if len(root) != 1:
        errors.append("expected exactly one ArtInfos collection")
    for collection in root:
        if expected_collection and name(collection) != expected_collection:
            errors.append(f"expected {expected_collection} collection for this ArtDefines filename")
        expected = COLLECTIONS.get(name(collection))
        if expected is None:
            errors.append(f"unexpected root child {name(collection)}")
            continue
        for entry in collection:
            if name(entry) != expected:
                errors.append(f"{name(collection)} contains loose {name(entry)}; expected {expected} wrapper")
            elif len([child for child in entry if name(child) == "Type" and (child.text or "").strip()]) != 1:
                errors.append(f"{expected} must have exactly one nonempty Type")
    for node in root.iter():
        if (len(node) or name(node) == "Civ4ArtDefines" or name(node) in COLLECTIONS) and (node.text or "").strip():
            errors.append(f"stray text inside {name(node)}")
        for child in node:
            if (child.tail or "").strip():
                errors.append(f"stray text after {name(child)}")
    return errors

def check(repo):
    paths = sorted((repo / "Assets/XML/Art").glob("CIV4ArtDefines_*.xml"))
    if not paths:
        return ["missing ArtDefines XML files"]
    errors = []
    for path in paths:
        try:
            errors.extend(f"{path.relative_to(repo)}: {error}" for error in check_tree(ET.parse(path).getroot(), path.stem.split("_")[-1] + "ArtInfos"))
        except ET.ParseError as error:
            errors.append(f"{path.relative_to(repo)}: {error}")
    return errors

def main():
    parser = argparse.ArgumentParser(description="Validate Civ4 ArtDefines root, collection and entry wrappers.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    errors = check(parser.parse_args().repo_root)
    print("FAIL ArtDefines structure" if errors else "PASS ArtDefines structure")
    for error in errors:
        print("  - " + error)
    return int(bool(errors))

if __name__ == "__main__":
    sys.exit(main())
