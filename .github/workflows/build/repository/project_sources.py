#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: NMake compiles root C++ files through a wildcard, so a successful build cannot reveal missing VC2010 Solution Explorer entries.
# Keep root source/header coverage synchronized with the IDE project; project-local resources and generated/output trees are outside this check. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->

# <!-- custom: argparse handles CLI options, Path resolves source/project files, and ElementTree reads the MSBuild XML entries. (GPT-6.1-Sol) -->
import argparse
from pathlib import Path
import xml.etree.ElementTree as ET


def check(repo_root: Path) -> list[str]:
    source_root = repo_root / "CvGameCoreDLL"
    project = source_root / "Project/AdvCiv.vcxproj"
    try:
        tree = ET.parse(project)
    except (OSError, ET.ParseError) as error:
        return [f"cannot read {project}: {error}"]
    expected = {"ClCompile": {}, "ClInclude": {}}
    for path in source_root.iterdir():
        if path.is_file() and path.suffix.lower() in (".cpp", ".h"):
            kind = "ClCompile" if path.suffix.lower() == ".cpp" else "ClInclude"
            expected[kind][path.name.casefold()] = path.name
    listed = {kind: set() for kind in expected}
    failures = []
    for node in tree.getroot().iter():
        kind = node.tag.rsplit("}", 1)[-1]
        include = node.get("Include")
        if kind not in listed or not include or "$(" in include:
            continue
        path = (project.parent / include.replace("\\", "/")).resolve()
        if path.parent != source_root.resolve() or path.suffix.lower() not in (".cpp", ".h"):
            continue
        key = path.name.casefold()
        listed[kind].add(key)
        if key not in expected[kind]:
            failures.append(f"{kind} lists missing or incorrectly classified root file: {include}")
    for kind, files in expected.items():
        for key in sorted(files.keys() - listed[kind]):
            failures.append(f"missing {kind} entry for CvGameCoreDLL/{files[key]}")
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description="Check root DLL sources/headers appear in the VC2010 project.")
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[4])
    args = parser.parse_args()
    failures = check(args.repo_root)
    if failures:
        print("FAIL IDE source/header coverage")
        for failure in failures:
            print("  - " + failure)
        return 1
    print("PASS IDE source/header coverage: root C++/header files match the VC2010 project")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
