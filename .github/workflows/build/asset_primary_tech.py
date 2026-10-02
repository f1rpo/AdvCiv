#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Tech-tree timing is iGridX (columns), not iGridY (rows). Keep the displayed primary unit/building prerequisite in the latest required column, and ensure a bonus can enter the trade network by the first improvement that connects it. (GPT-6.1-Sol) -->
import argparse
from pathlib import Path
import sys
import xml.etree.ElementTree as ET
from asset_tech_prereq_redundancy import read_era_order, read_techs, guaranteed_prereqs

ROOT = Path(__file__).resolve().parents[3]

def tag(node):
    return node.tag.rsplit("}", 1)[-1]

def value(node, key):
    return next(((c.text or "").strip() for c in node if tag(c) == key), "")

def nodes(repo, path, kind):
    return [n for n in ET.parse(repo / ("Assets/XML/" + path)).getroot().iter() if tag(n) == kind]

def check(repo):
    techs = {value(n, "Type"): int(value(n, "iGridX")) for n in nodes(repo, "Technologies/CIV4TechInfos.xml", "TechInfo")}
    errors = []
    def column(tech, owner):
        if tech in ("", "NONE", "NO_TECH"):
            return -1
        if tech not in techs:
            errors.append(f"{owner}: unknown tech {tech}")
            return -1
        return techs[tech]
    special = {value(n, "Type"): value(n, "TechPrereq") for n in nodes(repo, "Buildings/CIV4SpecialBuildingInfos.xml", "SpecialBuildingInfo")}
    for path, kind in (("Units/CIV4UnitInfos.xml", "UnitInfo"), ("Buildings/CIV4BuildingInfos.xml", "BuildingInfo")):
        for node in nodes(repo, path, kind):
            owner = value(node, "Type")
            primary = value(node, "PrereqTech")
            additional = [(n.text or "").strip() for c in node if tag(c) == "TechTypes" for n in c]
            shared = special.get(value(node, "SpecialBuildingType"), "") if kind == "BuildingInfo" else ""
            # <!-- custom: Religious buildings and Bomb Shelters use a shared special-building prerequisite with direct PrereqTech=NONE. Compare additional techs against that effective primary column; Builds use PrereqTech, whereas SpecialBuildingInfo uses TechPrereq. UnitInfo and BuildingInfo both store additional requirements in TechTypes; scanning a nonexistent PrereqTechs field missed a later unit requirement in the regression test. (GPT-6.1-Sol) -->
            primary_column = max(column(primary, owner), column(shared, owner))
            for tech in additional:
                if column(tech, owner) > primary_column:
                    errors.append(f"{owner}: primary {primary} (column {primary_column}) precedes required {tech} (column {techs[tech]})")
    graph, _ = read_techs(repo, read_era_order(repo))
    cache = {}
    def guaranteed(tech):
        return {tech} | guaranteed_prereqs(tech, graph, cache, set()) if tech in graph else set()
    builds = nodes(repo, "Units/CIV4BuildInfos.xml", "BuildInfo")
    improvements = nodes(repo, "Terrain/CIV4ImprovementInfos.xml", "ImprovementInfo")
    connecting = {}
    for improvement in improvements:
        # <!-- custom: City-like improvements connect every bonus independently of their resource-specific BonusTypeStruct entries. Excluding them keeps this check tied to the XML-defined resource improvement unlock. (GPT-6.1-Sol) -->
        if value(improvement, "bActsAsCity") == "1":
            continue
        unlocked = [value(b, "PrereqTech") for b in builds if value(b, "ImprovementType") == value(improvement, "Type")]
        for build_tech in unlocked:
            column(build_tech, value(improvement, "Type"))
        if not unlocked:
            continue
        for n in improvement.iter():
            if tag(n) == "BonusTypeStruct" and value(n, "bBonusTrade") == "1":
                connecting.setdefault(value(n, "BonusType"), []).extend(unlocked)
    for bonus in nodes(repo, "Terrain/CIV4BonusInfos.xml", "BonusInfo"):
        owner = value(bonus, "Type")
        reveal = value(bonus, "TechReveal")
        trade = value(bonus, "TechCityTrade")
        column(reveal, owner)
        column(trade, owner)
        # <!-- custom: Apply the same prerequisite-graph rule to every resource, including parallel technologies in the same column; named policy exceptions masked stale trade gates when XML changed. (GPT-6.1-Sol) -->
        if trade not in ("", "NONE", "NO_TECH") and owner in connecting:
            for build_tech in connecting[owner]:
                if trade not in guaranteed(reveal) | guaranteed(build_tech):
                    errors.append(f"{owner}: TechCityTrade {trade} is not guaranteed by reveal {reveal} and connecting Build prerequisite {build_tech}")
    return errors

def main():
    parser = argparse.ArgumentParser(description="Check primary asset prerequisites and bonus trade unlock timing.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    errors = check(parser.parse_args().repo_root)
    print("FAIL asset tech timing" if errors else "PASS asset tech timing")
    for error in errors:
        print("  - " + error)
    return int(bool(errors))

if __name__ == "__main__":
    sys.exit(main())
