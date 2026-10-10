#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Civ4 does not inherit missing technology audio entries from the base game's replaced mod-local audio tables.
# Resolve every normal and multiplayer technology sound through Audio2DScripts.xml and AudioDefines.xml so missing, malformed, or wrong-layer references fail in CI instead of becoming silent in-game.
# Also keep each technology's spoken recording distinct. (GPT-5.6-Sol) -->

from pathlib import Path
import argparse
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))

from xml_defines import get_default_repo_root, local_name, child_text_by_local_name


TECH_INFOS_REL_PATH = Path("Assets/XML/Technologies/CIV4TechInfos.xml")
AUDIO_SCRIPTS_REL_PATH = Path("Assets/XML/Audio/Audio2DScripts.xml")
AUDIO_DEFINES_REL_PATH = Path("Assets/XML/Audio/AudioDefines.xml")


def parse_xml(repo_root: Path, relative_path: Path) -> ET.ElementTree:
	path = repo_root / relative_path
	if not path.exists():
		raise RuntimeError(f"missing file: {path}")
	return ET.parse(path)


def read_keyed_values(repo_root: Path, relative_path: Path, parent_name: str, key_name: str, value_name: str) -> dict[str, str]:
	values: dict[str, str] = {}
	for node in parse_xml(repo_root, relative_path).getroot().iter():
		if local_name(node.tag) != parent_name:
			continue
		key = child_text_by_local_name(node, key_name)
		value = child_text_by_local_name(node, value_name)
		if key:
			values[key] = value or ""
	return values


def check_tech_audio(repo_root: Path) -> tuple[list[str], int]:
	failures: list[str] = []
	scripts = read_keyed_values(repo_root, AUDIO_SCRIPTS_REL_PATH, "Script2DSound", "ScriptID", "SoundID")
	definitions = read_keyed_values(repo_root, AUDIO_DEFINES_REL_PATH, "SoundData", "SoundID", "Filename")
	resolved_files: dict[str, dict[str, list[str]]] = {"Sound": {}, "SoundMP": {}}
	used_tech_scripts: set[str] = set()
	tech_count = 0

	for node in parse_xml(repo_root, TECH_INFOS_REL_PATH).getroot().iter():
		if local_name(node.tag) != "TechInfo":
			continue
		tech_count += 1
		tech_type = child_text_by_local_name(node, "Type") or f"TechInfo #{tech_count}"
		for field_name in ("Sound", "SoundMP"):
			script_id = child_text_by_local_name(node, field_name)
			if not script_id:
				failures.append(f"{tech_type}/{field_name}: missing Audio2DScripts ScriptID")
				continue
			used_tech_scripts.add(script_id)
			sound_id = scripts.get(script_id)
			if sound_id is None:
				failures.append(f"{tech_type}/{field_name}: ScriptID {script_id} is not defined in {AUDIO_SCRIPTS_REL_PATH}")
				continue
			if not sound_id:
				failures.append(f"{tech_type}/{field_name}: ScriptID {script_id} has an empty SoundID")
				continue
			filename = definitions.get(sound_id)
			if filename is None:
				failures.append(f"{tech_type}/{field_name}: {script_id} references SoundID {sound_id}, which is not defined in {AUDIO_DEFINES_REL_PATH}")
				continue
			if not filename:
				failures.append(f"{tech_type}/{field_name}: SoundID {sound_id} has an empty Filename")
				continue
			resolved_files[field_name].setdefault(filename.casefold(), []).append(tech_type)

	for field_name, by_filename in resolved_files.items():
		for filename, tech_types in sorted(by_filename.items()):
			if len(tech_types) > 1:
				failures.append(f"{field_name}: spoken recording {filename} is reused by {', '.join(sorted(tech_types))}")

	for script_id in sorted(scripts):
		if script_id.startswith("AS2D_TECH_") and script_id != "AS2D_TECH_GENERIC" and script_id not in used_tech_scripts:
			failures.append(f"{AUDIO_SCRIPTS_REL_PATH}: orphan technology ScriptID {script_id} is not used by any TechInfo Sound/SoundMP field")

	if tech_count == 0:
		failures.append(f"{TECH_INFOS_REL_PATH}: no TechInfo entries found")

	return failures, tech_count


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that every technology has distinct, mod-local normal and multiplayer audio mappings.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	try:
		failures, tech_count = check_tech_audio(args.repo_root)
	except (RuntimeError, ET.ParseError) as exc:
		print("FAIL technology audio mappings")
		print(f"  - {exc}")
		return 1

	if failures:
		print("FAIL technology audio mappings")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print(f"PASS technology audio mappings ({tech_count} technologies, {tech_count * 2} resolved fields)")
	return 0


if __name__ == "__main__":
	sys.exit(main())
