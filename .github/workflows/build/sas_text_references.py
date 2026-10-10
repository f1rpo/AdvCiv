#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Removed or renamed technologies, assets, and UI features can leave apparently valid TXT_KEY entries behind, while renamed SAS assets can reference pedia keys that were never defined.
# Audit every SAS-owned GameText file and SAS-owned asset description while explicitly accounting for inherited/EXE references and BUG's dynamically constructed key families; inherited AdvCiv/BUG/BULL files have too much unused-key static-analysis noise for a reliable blocking test and remain covered by the broader manual audit. (GPT-5.6-Sol) -->

from pathlib import Path
import argparse
import re
import sys
import xml.etree.ElementTree as ET


TARGET_GLOB = "Assets/XML/Text/AdvCiv-SAS*.xml"
ALL_TEXT_GLOB = "Assets/XML/Text/*.xml"
REFERENCE_GLOBS = ("Assets/XML/**/*.xml", "Assets/Config/**/*.xml", "Assets/Python/**/*.py", "PrivateMaps/**/*.py", "CvGameCoreDLL/**/*.cpp", "CvGameCoreDLL/**/*.h")
TEXT_BLOCK_RE = re.compile(r"<TEXT>(.*?)</TEXT>", re.DOTALL | re.IGNORECASE)
TAG_DEF_RE = re.compile(r"<Tag>\s*(TXT_KEY_[A-Z0-9_]+)\s*</Tag>", re.IGNORECASE)
TAG_SPAN_RE = re.compile(r"<Tag>.*?</Tag>", re.DOTALL | re.IGNORECASE)
XML_COMMENT_RE = re.compile(r"<!--.*?-->", re.DOTALL)
CPP_BLOCK_COMMENT_RE = re.compile(r"/\*.*?\*/", re.DOTALL)
LINE_COMMENT_RE = re.compile(r"(?m)^\s*(?://|#).*$")
TOKEN_RE = re.compile(r"TXT_KEY_[A-Z0-9_]+")
DYNAMIC_PREFIXES = ("TXT_KEY_BUG_OPT_", "TXT_KEY_BUG_OPTLABEL_")
INHERITED_REFERENCE_KEYS = {
	# Confirmed in inherited BTS/vanilla Python or XML; the GitHub checkout contains the mod only.
	"TXT_KEY_FOREIGN_ADVISOR_SURPLUS_RESOURCES",
	"TXT_KEY_FOREIGN_ADVISOR_TRADE_TABLE",
	"TXT_KEY_MAIN_MENU_CIVILOPEDIA",
	"TXT_KEY_MAIN_MENU_PLAY_NOW",
	"TXT_KEY_MAIN_MENU_SELECT_DIFFICULTY_TITLE",
	"TXT_KEY_MAIN_MENU_SELECT_GAME_SPEED_TITLE",
	"TXT_KEY_MAIN_MENU_SELECT_SEALEVEL_TITLE",
	"TXT_KEY_MAIN_MENU_SELECT_WORLD_SETUP",
	"TXT_KEY_MAIN_MENU_SELECT_WORLD_SIZE_TITLE",
	"TXT_KEY_MENU_SEALEVEL",
	"TXT_KEY_MENU_SIZE",
	"TXT_KEY_PEDIA_SCREEN_TOP",
}
EXE_RUNTIME_REFERENCE_KEYS = {
	# No static mod/base/vanilla reference, but changing the High label changed its Custom Game list text in-game; Low uses the matching EXE convention.
	"TXT_KEY_SEALEVEL_HIGH_RECOMMEND",
	"TXT_KEY_SEALEVEL_LOW_RECOMMEND",
}
INHERITED_PEDIA_REFERENCE_KEYS = {
	# <!-- custom: These descriptions are overridden by SAS, but their Civilopedia text remains inherited from the base game or expansion.
	# The GitHub checkout contains the mod only, so keep the confirmed inherited references explicit. (GPT-5.6-Sol) -->
	"TXT_KEY_BUILDING_EGYPTIAN_OBELISK_PEDIA",
	"TXT_KEY_CIV_AMERICA_PEDIA",
	"TXT_KEY_LEADER_ALEXANDER_PEDIA",
	"TXT_KEY_LEADER_AUGUSTUS_CAESAR_PEDIA",
	"TXT_KEY_LEADER_GENGHIS_KHAN_PEDIA",
	"TXT_KEY_LEADER_ISABELLA_PEDIA",
	"TXT_KEY_LEADER_JUSTINIAN_PEDIA",
	"TXT_KEY_LEADER_PACAL_PEDIA",
	"TXT_KEY_LEADER_SURYAVARMAN_PEDIA",
	"TXT_KEY_LEADER_TOKUGAWA_PEDIA",
	"TXT_KEY_LEADER_WILLEM_VAN_ORANJE_PEDIA",
}
PEDIA_INFO_SPECS = (
	("Assets/XML/Buildings/CIV4BuildingInfos.xml", "BuildingInfo"),
	("Assets/XML/Units/CIV4UnitInfos.xml", "UnitInfo"),
	("Assets/XML/Civilizations/CIV4CivilizationInfos.xml", "CivilizationInfo"),
	("Assets/XML/Civilizations/CIV4LeaderHeadInfos.xml", "LeaderHeadInfo"),
)


def read_text(path: Path) -> str:
	return path.read_text(encoding="utf-8", errors="replace")


def collect_definitions(paths: list[Path]) -> set[str]:
	definitions: set[str] = set()
	for path in paths:
		for block in TEXT_BLOCK_RE.findall(XML_COMMENT_RE.sub("", read_text(path))):
			definitions.update(TAG_DEF_RE.findall(block))
	return definitions


def collect_references(repo_root: Path) -> set[str]:
	references: set[str] = set()
	seen: set[Path] = set()
	for pattern in REFERENCE_GLOBS:
		for path in repo_root.glob(pattern):
			resolved = path.resolve()
			if not path.is_file() or resolved in seen:
				continue
			seen.add(resolved)
			text = read_text(path)
			if path.suffix.casefold() == ".xml":
				text = XML_COMMENT_RE.sub("", TAG_SPAN_RE.sub("", text))
			else:
				text = LINE_COMMENT_RE.sub("", CPP_BLOCK_COMMENT_RE.sub("", text))
			references.update(TOKEN_RE.findall(text))
	return references


def child_text(node: ET.Element, tag_name: str) -> str:
	for child in node:
		if child.tag.rsplit("}", 1)[-1] == tag_name:
			return (child.text or "").strip()
	return ""


def collect_missing_sas_pedia_references(repo_root: Path, sas_definitions: set[str], all_mod_definitions: set[str]) -> tuple[list[str], list[str]]:
	missing: list[str] = []
	inherited: set[str] = set()
	for rel_path, info_tag in PEDIA_INFO_SPECS:
		path = repo_root / rel_path
		if not path.is_file():
			raise RuntimeError(f"missing file: {path}")
		for node in ET.parse(path).getroot().iter():
			if node.tag.rsplit("}", 1)[-1] != info_tag:
				continue
			description = child_text(node, "Description")
			if description not in sas_definitions:
				continue
			pedia = child_text(node, "Civilopedia")
			if pedia in all_mod_definitions:
				continue
			if pedia in INHERITED_PEDIA_REFERENCE_KEYS:
				inherited.add(pedia)
				continue
			info_type = child_text(node, "Type") or "UNKNOWN_INFO"
			missing.append(f"{info_type}: Civilopedia references {pedia or 'an empty key'}, but no matching mod GameText <Tag> exists")
	return sorted(missing), sorted(inherited)


def check_sas_text_references(repo_root: Path) -> tuple[list[str], list[str], list[str], list[str], list[str], list[str], int, int]:
	targets = sorted(path for path in repo_root.glob(TARGET_GLOB) if path.is_file())
	if not targets:
		raise RuntimeError(f"no files match {TARGET_GLOB}")
	definitions = collect_definitions(targets)
	if not definitions:
		raise RuntimeError(f"no TXT_KEY definitions found in {TARGET_GLOB}")
	all_mod_definitions = collect_definitions(sorted(path for path in repo_root.glob(ALL_TEXT_GLOB) if path.is_file()))
	references = collect_references(repo_root)
	inherited = sorted(definitions & INHERITED_REFERENCE_KEYS)
	exe_runtime = sorted(definitions & EXE_RUNTIME_REFERENCE_KEYS)
	dynamic = sorted(key for key in definitions if key not in references and key.startswith(DYNAMIC_PREFIXES))
	unused = sorted(key for key in definitions if key not in references and key not in INHERITED_REFERENCE_KEYS and key not in EXE_RUNTIME_REFERENCE_KEYS and not key.startswith(DYNAMIC_PREFIXES))
	missing_pedia, inherited_pedia = collect_missing_sas_pedia_references(repo_root, definitions, all_mod_definitions)
	return unused, missing_pedia, inherited, inherited_pedia, exe_runtime, dynamic, len(definitions), len(targets)


def print_keys(title: str, keys: list[str]) -> None:
	print(f"{title} ({len(keys)}):")
	for key in keys:
		print(f"  - {key}")


def main() -> int:
	parser = argparse.ArgumentParser(description="Reject unused SAS-owned TXT_KEY entries and missing SAS-owned Civilopedia references.")
	parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[3], help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	try:
		unused, missing_pedia, inherited, inherited_pedia, exe_runtime, dynamic, definition_count, file_count = check_sas_text_references(args.repo_root)
	except RuntimeError as exc:
		print("FAIL SAS GameText references")
		print(f"  - {exc}")
		return 1

	if unused or missing_pedia:
		print("FAIL SAS GameText references")
		for key in unused:
			print(f"  - {key}: defined in {TARGET_GLOB} but not referenced by active mod XML/Config/Python/PrivateMaps/C++")
		for failure in missing_pedia:
			print(f"  - {failure}")
		return 1

	print(f"PASS SAS GameText references ({definition_count} referenced or explicitly accepted keys across {file_count} files)")
	print_keys("Confirmed inherited base references absent from the GitHub mod-only checkout", inherited)
	print_keys("Confirmed inherited Civilopedia references for SAS-overridden descriptions", inherited_pedia)
	print_keys("Empirically confirmed EXE runtime references with no static mod/base/vanilla reference", exe_runtime)
	print_keys("Recognized dynamically constructed BUG keys", dynamic)
	return 0


if __name__ == "__main__":
	sys.exit(main())
