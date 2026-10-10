#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Check every tracked file, including inherited references and project files, so encoding signatures and Windows install-path failures cannot hide outside the active-source scan.
# Binary assets are checked only for a leading encoding signature. (GPT-6.1-Sol) -->
import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
INSTALL_ROOT = "C:/Program Files (x86)/Steam/steamapps/common/Sid Meier's Civilization IV Beyond the Sword/Beyond the Sword/Mods/" + ROOT.name
# <!-- custom: Removing the inherited UTF-8 BOMs from AdvCiv.sln and AdvCiv.vcxproj was followed by the SAS project shortcut no longer opening through VSLauncher.
# Preserve these Visual Studio formats' UTF-8 BOMs; Civ4 source/data files still reject BOMs, and UTF-16/32 remain rejected everywhere. (GPT-6.1-Sol) -->
VISUAL_STUDIO_SUFFIXES = {".sln", ".vcxproj"}
# <!-- custom: Cap filenames including extensions at 56 UTF-16 units to keep names manageable and leave installation-path headroom.
# The existing 52-unit Gilgamesh animation filename provides the baseline; 56 adds four units of leeway.
# Gilgamesh.kfm embeds its exact name, so renaming only the KF would break its reference.
# This is our maintenance policy, not a Windows component limit. (GPT-6.1-Sol) -->
MAX_FILENAME_UNITS = 56
BOMS = ((b"\x00\x00\xfe\xff", "UTF-32 BE"), (b"\xff\xfe\x00\x00", "UTF-32 LE"), (b"\xef\xbb\xbf", "UTF-8"), (b"\xff\xfe", "UTF-16 LE"), (b"\xfe\xff", "UTF-16 BE"))

def check(root, install_root=INSTALL_ROOT):
    paths = subprocess.check_output(["git", "ls-files", "-z"], cwd=root).decode("utf-8").split("\0")
    errors = []
    for relative in filter(None, paths):
        path = root / relative
        if not path.is_file():
            errors.append(f"{relative}: tracked file is missing")
            continue
        with path.open("rb") as stream:
            prefix = stream.read(4)
        for signature, encoding in BOMS:
            if prefix.startswith(signature):
                if encoding != "UTF-8" or path.suffix.lower() not in VISUAL_STUDIO_SUFFIXES:
                    errors.append(f"{relative}: {encoding} BOM; save without BOM")
                break
        filename_length = len(path.name.encode("utf-16-le")) // 2
        if filename_length > MAX_FILENAME_UNITS:
            errors.append(f"{relative}: filename uses {filename_length} UTF-16 units including extension (maximum {MAX_FILENAME_UNITS})")
        full_path = install_root.rstrip("/\\") + "/" + relative
        # <!-- custom: MAX_PATH includes the terminating NUL; count UTF-16 code units because non-BMP names occupy two Windows WCHARs; directories need room for an appended 8.3 filename too. (GPT-6.1-Sol) -->
        length = len(full_path.encode("utf-16-le")) // 2
        if length >= 260:
            errors.append(f"{relative}: installed path uses {length} UTF-16 units (maximum 259)")
        for parent in Path(relative).parents:
            if parent == Path("."):
                continue
            directory = install_root.rstrip("/\\") + "/" + parent.as_posix()
            if len(directory.encode("utf-16-le")) // 2 > 247:
                errors.append(f"{relative}: installed parent directory exceeds 247 UTF-16 units")
                break
    return errors

def main():
    parser = argparse.ArgumentParser(description="Reject unsupported tracked BOMs, excessive filename lengths and paths too long for a standard Civ4 Windows install.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--install-root", default=INSTALL_ROOT)
    args = parser.parse_args()
    errors = check(args.repo_root, args.install_root)
    print("FAIL repository hygiene" if errors else "PASS repository hygiene")
    for error in errors:
        print("  - " + error)
    return int(bool(errors))

if __name__ == "__main__":
    sys.exit(main())
