#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Validate every non-empty leader diplomacy-audio reference through the mod-local audio tables so a typo cannot silently disable one era's intro music. See KI#1031. (GPT-5.6-Sol) -->

from pathlib import Path
import argparse
import sys
import xml.etree.ElementTree as ET

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root, local_name, child_text_by_local_name


LEADER_INFOS_REL_PATH = Path("Assets/XML/Civilizations/CIV4LeaderHeadInfos.xml")
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


def check_leader_audio(repo_root: Path) -> tuple[list[str], int, int]:
	failures: list[str] = []
	scripts = read_keyed_values(repo_root, AUDIO_SCRIPTS_REL_PATH, "Script2DSound", "ScriptID", "SoundID")
	definitions = read_keyed_values(repo_root, AUDIO_DEFINES_REL_PATH, "SoundData", "SoundID", "Filename")
	leader_count = 0
	reference_count = 0

	for leader_node in parse_xml(repo_root, LEADER_INFOS_REL_PATH).getroot().iter():
		if local_name(leader_node.tag) != "LeaderHeadInfo":
			continue
		leader_count += 1
		leader_type = child_text_by_local_name(leader_node, "Type") or f"LeaderHeadInfo #{leader_count}"
		for node in leader_node.iter():
			if local_name(node.tag) != "DiploScriptId" or not node.text or not node.text.strip():
				continue
			reference_count += 1
			script_id = node.text.strip()
			sound_id = scripts.get(script_id)
			if sound_id is None:
				failures.append(f"{leader_type}: DiploScriptId {script_id} is not defined in {AUDIO_SCRIPTS_REL_PATH}")
				continue
			if not sound_id:
				failures.append(f"{leader_type}: DiploScriptId {script_id} has an empty SoundID")
				continue
			filename = definitions.get(sound_id)
			if filename is None:
				failures.append(f"{leader_type}: {script_id} references SoundID {sound_id}, which is not defined in {AUDIO_DEFINES_REL_PATH}")
			elif not filename:
				failures.append(f"{leader_type}: SoundID {sound_id} has an empty Filename")

	if leader_count == 0:
		failures.append(f"{LEADER_INFOS_REL_PATH}: no LeaderHeadInfo entries found")
	if reference_count == 0:
		failures.append(f"{LEADER_INFOS_REL_PATH}: no non-empty DiploScriptId references found")

	return failures, leader_count, reference_count


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that every non-empty leader diplomacy-audio reference resolves through the mod-local audio tables.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	try:
		failures, leader_count, reference_count = check_leader_audio(args.repo_root)
	except (RuntimeError, ET.ParseError) as exc:
		print("FAIL leader diplomacy audio mappings")
		print(f"  - {exc}")
		return 1

	if failures:
		print("FAIL leader diplomacy audio mappings")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print(f"PASS leader diplomacy audio mappings ({leader_count} leaders, {reference_count} resolved references)")
	return 0


if __name__ == "__main__":
	sys.exit(main())
