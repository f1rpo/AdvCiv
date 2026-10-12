#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: main-menu opening music must have a valid configured Audio2DScripts entry.

from pathlib import Path
import argparse
import re
import sys
import xml.etree.ElementTree as ET

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root, read_global_define_ints, read_global_define_texts


AUDIO_2D_SCRIPTS_RELATIVE_PATH = Path("Assets/XML/Audio/Audio2DScripts.xml")

SHUFFLE_ENABLE_DEFINE = "SAS_MAIN_MENU_OPENING_MUSIC_SHUFFLE_ENABLE"
SHUFFLE_TRIGGER_SCRIPT_DEFINE = "SAS_MAIN_MENU_OPENING_MUSIC_SHUFFLE_TRIGGER_SCRIPT"
SHUFFLE_SCRIPT_DEFINE_RE = re.compile(r"^SAS_MAIN_MENU_OPENING_MUSIC_SHUFFLE_SCRIPT_(\d+)$")


def local_name(tag: str) -> str:
	return tag.rsplit("}", 1)[-1]


def read_audio_2d_script_ids(repo_root: Path) -> set[str]:
	path = repo_root / AUDIO_2D_SCRIPTS_RELATIVE_PATH
	if not path.exists():
		raise RuntimeError(f"missing file: {path}")

	tree = ET.parse(path)
	script_ids: set[str] = set()

	for node in tree.getroot().iter():
		if local_name(node.tag) != "ScriptID":
			continue

		value = (node.text or "").strip()
		if value:
			script_ids.add(value)

	return script_ids


def read_shuffle_script_defines(define_texts: dict[str, str]) -> list[tuple[str, str]]:
	items: list[tuple[int, str, str]] = []

	for define_name, value in define_texts.items():
		match = SHUFFLE_SCRIPT_DEFINE_RE.match(define_name)
		if not match:
			continue

		items.append((int(match.group(1)), define_name, value.strip()))

	return [(define_name, value) for _, define_name, value in sorted(items)]


def check_opening_music(repo_root: Path) -> list[str]:
	failures: list[str] = []

	int_defines = read_global_define_ints(repo_root)
	define_texts = read_global_define_texts(repo_root)
	audio_script_ids = read_audio_2d_script_ids(repo_root)

	if SHUFFLE_ENABLE_DEFINE not in int_defines:
		failures.append(f"GlobalDefines_advciv_sas.xml: missing {SHUFFLE_ENABLE_DEFINE}")
		shuffle_enabled = False
	else:
		shuffle_enabled = int_defines[SHUFFLE_ENABLE_DEFINE] > 0

	trigger_script = define_texts.get(SHUFFLE_TRIGGER_SCRIPT_DEFINE, "").strip()
	if not trigger_script:
		failures.append(f"GlobalDefines_advciv_sas.xml: missing or empty {SHUFFLE_TRIGGER_SCRIPT_DEFINE}")
	elif trigger_script not in audio_script_ids:
		failures.append(f"GlobalDefines_advciv_sas.xml: {SHUFFLE_TRIGGER_SCRIPT_DEFINE} references missing Audio2DScripts ScriptID {trigger_script}")

	shuffle_script_defines = read_shuffle_script_defines(define_texts)
	if not shuffle_script_defines:
		failures.append("GlobalDefines_advciv_sas.xml: no SAS_MAIN_MENU_OPENING_MUSIC_SHUFFLE_SCRIPT_* defines found")

	active_shuffle_scripts: list[tuple[str, str]] = []
	for define_name, script_id in shuffle_script_defines:
		if not script_id or script_id.upper() == "NONE":
			continue

		active_shuffle_scripts.append((define_name, script_id))

		if script_id not in audio_script_ids:
			failures.append(f"GlobalDefines_advciv_sas.xml: {define_name} references missing Audio2DScripts ScriptID {script_id}")

	if shuffle_enabled and not active_shuffle_scripts:
		failures.append(f"GlobalDefines_advciv_sas.xml: {SHUFFLE_ENABLE_DEFINE} is enabled but no non-NONE shuffle script is configured")

	if not shuffle_enabled and not trigger_script:
		failures.append("GlobalDefines_advciv_sas.xml: shuffle is disabled and no fixed/trigger opening script is configured")

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that AdvCiv-SAS main-menu opening music has valid Audio2DScripts references.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	failures = check_opening_music(args.repo_root)

	if failures:
		print("FAIL main-menu opening music setup")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS main-menu opening music setup")
	return 0


if __name__ == "__main__":
	sys.exit(main())
