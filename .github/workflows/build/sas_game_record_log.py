#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: SASGameRecord report logging must be disabled by default, the public revision/current history marker must stay synchronized.
# Canonical readable AI-strategy/AreaAI/contact diagnostics must match their native enums, exact AI target-city provenance/checkpoints must cover every writer/effective clear, periodic AI-attitude provenance must stay synchronized with AI_updateAttitude, and strategic/vote/contact/Great-Person decision schemas must remain present.

from pathlib import Path
import argparse
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))

from xml_defines import get_default_repo_root, read_global_define_ints, require_int_values


REVISION_HEADER = Path("CvGameCoreDLL/SASGameRecordLog.h")
REVISION_SOURCE = Path("CvGameCoreDLL/SASGameRecordLog.cpp")
REVISION_HISTORY = Path("_1_AdvCiv-SAS/Docs/README_SASGameRecord_Revisions.md")
CITY_DELTA_EXPANDER = Path("LLM_Helpers/expand_sasgamerecord_city_deltas.py")
AI_STRATEGIES_HEADER = Path("CvGameCoreDLL/AIStrategies.h")
CV_ENUMS_HEADER = Path("CvGameCoreDLL/CvEnums.h")
GAME_CORE_UTILS_SOURCE = Path("CvGameCoreDLL/CvGameCoreUtils.cpp")
PLAYER_AI_SOURCE = Path("CvGameCoreDLL/CvPlayerAI.cpp")
PLAYER_AI_HEADER = Path("CvGameCoreDLL/CvPlayerAI.h")
GAME_SOURCE = Path("CvGameCoreDLL/CvGame.cpp")
PLAYER_SOURCE = Path("CvGameCoreDLL/CvPlayer.cpp")
PLOT_SOURCE = Path("CvGameCoreDLL/CvPlot.cpp")
CITY_SOURCE = Path("CvGameCoreDLL/CvCity.cpp")
TEAM_AI_SOURCE = Path("CvGameCoreDLL/CvTeamAI.cpp")
UWAI_AGENT_SOURCE = Path("CvGameCoreDLL/UWAIAgent.cpp")
UNIT_AI_HEADER = Path("CvGameCoreDLL/CvUnitAI.h")
UNIT_AI_SOURCE = Path("CvGameCoreDLL/CvUnitAI.cpp")
CITY_SITE_EVALUATOR_HEADER = Path("CvGameCoreDLL/CitySiteEvaluator.h")



def check_revision(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, REVISION_HISTORY):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing SASGameRecord revision file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	revision_matches = re.findall(r"\bSAS_GAME_RECORD_REVISION\s*=\s*(\d+)\b", header_text)
	if len(revision_matches) != 1:
		return [f"{REVISION_HEADER}: expected exactly one SAS_GAME_RECORD_REVISION assignment, found {len(revision_matches)}"]
	revision = int(revision_matches[0])

	history_text = (repo_root / REVISION_HISTORY).read_text(encoding="utf-8", errors="replace")
	history_revisions = [int(value) for value in re.findall(r"^### Revision (\d+) - SAS practical ", history_text, flags=re.MULTILINE)]
	if not history_revisions:
		failures.append(f"{REVISION_HISTORY}: no revision-history headings found")
	else:
		if history_revisions[0] != revision:
			failures.append(f"revision mismatch: source={revision}, newest history={history_revisions[0]}")
		expected = list(range(revision, 0, -1))
		if history_revisions != expected:
			failures.append(f"{REVISION_HISTORY}: expected contiguous latest-first revisions {revision}..1")
	documented_revision_matches = re.findall(r"GAME_RECORD_SOURCE_CONTEXT recordRevision=(\d+) \.\.\.", history_text)
	if len(documented_revision_matches) != 1:
		failures.append(f"{REVISION_HISTORY}: expected exactly one current emitted recordRevision example, found {len(documented_revision_matches)}")
	elif int(documented_revision_matches[0]) != revision:
		failures.append(f"{REVISION_HISTORY}: current emitted recordRevision example={documented_revision_matches[0]}, source={revision}")

	source_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	if 'GAME_RECORD_SOURCE_CONTEXT recordRevision=%d %s' not in source_text or 'SAS_GAME_RECORD_REVISION' not in source_text:
		failures.append(f"{REVISION_SOURCE}: missing emitted recordRevision SOURCE_CONTEXT field tied to SAS_GAME_RECORD_REVISION")
	return failures


def check_context_consumer_contracts(repo_root: Path) -> list[str]:
	failures = []
	if not (repo_root / REVISION_SOURCE).is_file():
		return [f"missing SASGameRecord context-contract file: {REVISION_SOURCE}"]
	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")

	# <!-- custom: Keep the compact support-context topology stable for downstream parsers without claiming that Windows' primary adapter proves the exact Direct3D device selected by Civ4. (ChatGPT-5.6-Sol) -->
	display_fields = "gpuAdapterCount=%d gpuDesktopAdapterCount=%d gpuVendorFamilyCount=%d gpuPrimaryVendor=%s gpuVendors=%s"
	if display_fields not in record_text:
		failures.append(f"{REVISION_SOURCE}: GAME_RECORD_DISPLAY_CONTEXT missing multi-GPU topology fields")
	display_placeholders = "gpuAdapterCount=-1 gpuDesktopAdapterCount=-1 gpuVendorFamilyCount=-1 gpuPrimaryVendor=- gpuVendors=-"
	if display_placeholders not in record_text:
		failures.append(f"{REVISION_SOURCE}: disabled/lower-level GAME_RECORD_DISPLAY_CONTEXT placeholders changed")
	for required in ("getSASGameRecordDisplayAdapterContext", "DISPLAY_DEVICE_MIRRORING_DRIVER", "DISPLAY_DEVICE_ATTACHED_TO_DESKTOP", "DISPLAY_DEVICE_PRIMARY_DEVICE"):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: display-adapter context missing {required}")

	# <!-- custom: Structured text-map rows are independently grep/parser friendly; raw |...| drawing rows intentionally remain undecorated. (ChatGPT-5.6-Sol) -->
	if record_text.count('GAME_RECORD_MAP_ASCII_LEGEND turn=%d') != 6:
		failures.append(f"{REVISION_SOURCE}: expected turn on all 6 structured text-map legend rows")
	if record_text.count('GAME_RECORD_MAP_ASCII_LAYER_BEGIN turn=%d') != 1:
		failures.append(f"{REVISION_SOURCE}: expected turn on structured text-map layer-begin row")
	if record_text.count('GAME_RECORD_MAP_ASCII_LAYER_END turn=%d') != 2:
		failures.append(f"{REVISION_SOURCE}: expected turn on both structured text-map layer-end formats")
	if 'GAME_RECORD_MAP_ASCII_CONFIG_ERROR turn=%d' not in record_text:
		failures.append(f"{REVISION_SOURCE}: text-map config-error row must remain self-locating with turn")
	return failures

def check_city_delta_contracts(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_SOURCE, CITY_DELTA_EXPANDER):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing SASGameRecord city-delta file: {relative_path}")
	if failures:
		return failures
	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"SAS_GAME_RECORD_CITY_FULL_SNAPSHOT_CADENCE = 10", "compactSASGameRecordCityDetailRow",
		"previousTurn=%d fullBaseTurn=%d changed=%d", "g_mapSASGameRecordCityDeltaStates.clear()",
		"g_mapSASGameRecordCityIdentityStates.clear()", 'g_iSASGameRecordLogLevel >= 3 && szLine.find("GAME_RECORD_CITY") == 0',
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing bounded city-delta contract token {required}")
	for row_type in ("GAME_RECORD_CITY", "GAME_RECORD_CITY_DEVELOPMENT", "GAME_RECORD_CITY_HAPPINESS", "GAME_RECORD_CITY_HEALTH", "GAME_RECORD_CITY_BUILDINGS"):
		if f'szType == "{row_type}"' not in record_text:
			failures.append(f"{REVISION_SOURCE}: city-delta family missing {row_type}")
	if 'szType == "GAME_RECORD_CITY_TRADE_PARTNERS"' in record_text:
		failures.append(f"{REVISION_SOURCE}: trade-partner rows must remain full; delta metadata is larger than their one-field payload")
	for identity_field in ("x", "y", "originalOwner", "foundedTurn", "acquiredTurn"):
		if f'getSASGameRecordDeltaFieldValue(aFields, "{identity_field}")' not in record_text:
			failures.append(f"{REVISION_SOURCE}: city delta generation guard missing {identity_field}")
	expander_text = (repo_root / CITY_DELTA_EXPANDER).read_text(encoding="utf-8", errors="replace")
	for required in ("previousTurn", "fullBaseTurn", "changed", "GAME_RECORD_CITY_DEVELOPMENT", "GAME_RECORD_CITY_BUILDINGS"):
		if required not in expander_text:
			failures.append(f"{CITY_DELTA_EXPANDER}: reference city-delta expander missing {required}")
	return failures


def check_ai_strategy_diagnostics(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (AI_STRATEGIES_HEADER, GAME_CORE_UTILS_SOURCE, REVISION_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI-strategy diagnostic file: {relative_path}")
	if failures:
		return failures

	enum_text = (repo_root / AI_STRATEGIES_HEADER).read_text(encoding="utf-8", errors="replace")
	enum_match = re.search(r"enum\s+AIStrategy\b[^\{]*\{(?P<body>.*?)\};", enum_text, flags=re.DOTALL)
	if enum_match is None:
		return [f"{AI_STRATEGIES_HEADER}: could not locate enum AIStrategy"]
	enum_entries = re.findall(
		r"^\s*(NO_AI_STRATEGY|AI_DEFAULT_STRATEGY|AI_STRATEGY_[A-Z0-9_]+)\s*=\s*([^,]+),",
		enum_match.group("body"), flags=re.MULTILINE)
	if not enum_entries:
		return [f"{AI_STRATEGIES_HEADER}: no explicitly assigned AIStrategy enumerators found"]
	enum_tokens = [token for token, _ in enum_entries]
	if len(enum_tokens) != len(set(enum_tokens)):
		failures.append(f"{AI_STRATEGIES_HEADER}: duplicate AIStrategy enumerator names found")
	if enum_tokens[0] != "NO_AI_STRATEGY" or len(enum_tokens) < 2 or enum_tokens[1] != "AI_DEFAULT_STRATEGY":
		failures.append(f"{AI_STRATEGIES_HEADER}: expected NO_AI_STRATEGY then AI_DEFAULT_STRATEGY at the start of enum AIStrategy")
	for iEntry, (token, expression) in enumerate(enum_entries):
		normalized_expression = re.sub(r"\s+", "", expression)
		if token == "NO_AI_STRATEGY":
			if normalized_expression != "0":
				failures.append(f"{AI_STRATEGIES_HEADER}: NO_AI_STRATEGY must remain 0 for complete bit scans")
			continue
		bit_match = re.fullmatch(r"\(?1<<(?P<bit>\d+)\)?", normalized_expression)
		expected_bit = iEntry - 1
		if bit_match is None or int(bit_match.group("bit")) != expected_bit:
			failures.append(
				f"{AI_STRATEGIES_HEADER}: {token} must remain the contiguous bit (1 << {expected_bit}); "
				f"complete SASGameRecord scans advance with iStrategy <<= 1")
	strategy_tokens = [token for token in enum_tokens if token.startswith("AI_STRATEGY_")]
	if not strategy_tokens:
		failures.append(f"{AI_STRATEGIES_HEADER}: no non-default AI_STRATEGY_* enumerators found")
		return failures
	last_strategy = strategy_tokens[-1]

	utils_text = (repo_root / GAME_CORE_UTILS_SOURCE).read_text(encoding="utf-8", errors="replace")
	helper_match = re.search(
		r"char\s+const\*\s+getSASAIStrategyType\s*\([^)]*\)\s*\{(?P<body>.*?)^\}",
		utils_text, flags=re.DOTALL | re.MULTILINE)
	if helper_match is None:
		failures.append(f"{GAME_CORE_UTILS_SOURCE}: missing getSASAIStrategyType definition")
	else:
		case_pairs = re.findall(r'case\s+(NO_AI_STRATEGY|AI_DEFAULT_STRATEGY|AI_STRATEGY_[A-Z0-9_]+)\s*:\s*return\s+"([^"]+)"\s*;', helper_match.group("body"))
		case_tokens = [token for token, _ in case_pairs]
		if len(case_tokens) != len(set(case_tokens)):
			failures.append(f"{GAME_CORE_UTILS_SOURCE}: duplicate getSASAIStrategyType case(s) found")
		missing = [token for token in enum_tokens if token not in case_tokens]
		extra = [token for token in case_tokens if token not in enum_tokens]
		if missing:
			failures.append(f"{GAME_CORE_UTILS_SOURCE}: getSASAIStrategyType missing {', '.join(missing)}")
		if extra:
			failures.append(f"{GAME_CORE_UTILS_SOURCE}: getSASAIStrategyType has non-enum case(s): {', '.join(extra)}")
		for token, label in case_pairs:
			if token != label:
				failures.append(f"{GAME_CORE_UTILS_SOURCE}: {token} maps to {label!r}, expected identical raw enum token")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	loop_matches = re.findall(
		r"for\s*\(\s*int\s+iStrategy\s*=\s*(AI_DEFAULT_STRATEGY|AI_STRATEGY_DAGGER)\s*;\s*"
		r"iStrategy\s*<=\s*(AI_STRATEGY_[A-Z0-9_]+)\s*;\s*iStrategy\s*<<=\s*1\s*\)",
		record_text, flags=re.DOTALL)
	if len(loop_matches) != 3:
		failures.append(f"{REVISION_SOURCE}: expected 3 complete AIStrategy scans (CORE/snapshot/transitions), found {len(loop_matches)}")
	else:
		starts = [start for start, _ in loop_matches]
		if starts.count("AI_DEFAULT_STRATEGY") != 1 or starts.count("AI_STRATEGY_DAGGER") != 2:
			failures.append(f"{REVISION_SOURCE}: expected one CORE scan from AI_DEFAULT_STRATEGY and two readable scans from AI_STRATEGY_DAGGER")
		for start, end in loop_matches:
			if end != last_strategy:
				failures.append(f"{REVISION_SOURCE}: strategy scan starting at {start} ends at {end}, current enum ends at {last_strategy}")
	for required in ("GAME_RECORD_AI_STRATEGIES", "GAME_RECORD_AI_STRATEGY_CHANGE", "getSASAIStrategyType"):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI-strategy diagnostic token {required}")
	return failures


def check_area_ai_diagnostics(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (CV_ENUMS_HEADER, GAME_CORE_UTILS_SOURCE, TEAM_AI_SOURCE, UWAI_AGENT_SOURCE, REVISION_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AreaAI diagnostic file: {relative_path}")
	if failures:
		return failures

	enum_text = (repo_root / CV_ENUMS_HEADER).read_text(encoding="utf-8", errors="replace")
	enum_match = re.search(r"ENUM_START\(AreaAI,\s*AREAAI\)(?P<body>.*?)ENUM_END\(AreaAI,\s*AREAAI\)", enum_text, flags=re.DOTALL)
	if enum_match is None:
		return [f"{CV_ENUMS_HEADER}: could not locate AreaAI enum block"]
	enum_tokens = ["NO_AREAAI"] + re.findall(r"^\s*(AREAAI_[A-Z0-9_]+)\s*,", enum_match.group("body"), flags=re.MULTILINE)
	if len(enum_tokens) != len(set(enum_tokens)) or len(enum_tokens) < 2:
		failures.append(f"{CV_ENUMS_HEADER}: invalid/duplicate AreaAI enumerators found")

	utils_text = (repo_root / GAME_CORE_UTILS_SOURCE).read_text(encoding="utf-8", errors="replace")
	helper_match = re.search(r"char\s+const\*\s+getSASAreaAIType\s*\([^)]*\)\s*\{(?P<body>.*?)^\}", utils_text, flags=re.DOTALL | re.MULTILINE)
	if helper_match is None:
		failures.append(f"{GAME_CORE_UTILS_SOURCE}: missing getSASAreaAIType definition")
	else:
		case_pairs = re.findall(r'case\s+(NO_AREAAI|AREAAI_[A-Z0-9_]+)\s*:\s*return\s+"([^"]+)"\s*;', helper_match.group("body"))
		case_tokens = [token for token, _ in case_pairs]
		missing = [token for token in enum_tokens if token not in case_tokens]
		extra = [token for token in case_tokens if token not in enum_tokens]
		if missing:
			failures.append(f"{GAME_CORE_UTILS_SOURCE}: getSASAreaAIType missing {', '.join(missing)}")
		if extra:
			failures.append(f"{GAME_CORE_UTILS_SOURCE}: getSASAreaAIType has non-enum case(s): {', '.join(extra)}")
		for token, label in case_pairs:
			if token != label:
				failures.append(f"{GAME_CORE_UTILS_SOURCE}: {token} maps to {label!r}, expected identical raw enum token")

	team_text = (repo_root / TEAM_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	uwai_text = (repo_root / UWAI_AGENT_SOURCE).read_text(encoding="utf-8", errors="replace")
	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	# AI_updateAreaStrategies deliberately duplicates the one-line setter between its disabled fast path and logged path so recorder-off gameplay pays no per-area diagnostic branch/read. UWAI has one setter in its alignment writer.
	if team_text.count("setAreaAIType(") != 2 or uwai_text.count("setAreaAIType(") != 1:
		failures.append(f"AreaAI writer shape changed: expected two guarded setter call sites in {TEAM_AI_SOURCE} and one in {UWAI_AGENT_SOURCE}")
	if "logSASGameRecordAreaAIChanged(" not in team_text or '"CALCULATED"' not in team_text:
		failures.append(f"{TEAM_AI_SOURCE}: missing calculated AreaAI transition bridge/source provenance")
	if ("logSASGameRecordAreaAIChanged(" not in uwai_text or '"UWAI_ALIGN_NAVAL"' not in uwai_text or
			'"UWAI_ALIGN_LAND"' not in uwai_text):
		failures.append(f"{UWAI_AGENT_SOURCE}: missing UWAI AreaAI transition bridge/source provenance")
	for required in ("GAME_RECORD_AREA_AI_CHANGE", "GAME_RECORD_AREA_AI", "getSASAreaAIType"):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AreaAI diagnostic token {required}")
	if "logSASGameRecordAreaAISnapshot(BARBARIAN_TEAM, iGameTurn);" not in record_text:
		failures.append(f"{REVISION_SOURCE}: missing Barbarian AreaAI periodic checkpoint bridge")
	return failures


def check_ai_target_city_provenance(repo_root: Path) -> list[str]:
	failures = []
	required_paths = (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE, PLAYER_SOURCE, PLOT_SOURCE, CITY_SOURCE)
	for relative_path in required_paths:
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI target-city provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	expected_sources = [
		"SAS_AI_TARGET_CITY_AREA_SEARCH",
		"SAS_AI_TARGET_CITY_RANDOM_CLEAR",
		"SAS_AI_TARGET_CITY_DIPLO_COORDINATION",
		"SAS_AI_TARGET_CITY_AREA_REASSIGN_CLEAR",
		"SAS_AI_TARGET_CITY_CITY_REMOVED",
	]
	enum_match = re.search(
		r"enum\s+SASGameRecordAITargetCityChangeSource\s*\{(?P<body>.*?)\};",
		header_text, flags=re.DOTALL)
	if enum_match is None:
		failures.append(f"{REVISION_HEADER}: missing SASGameRecordAITargetCityChangeSource enum")
	else:
		source_tokens = re.findall(r"\b(SAS_AI_TARGET_CITY_[A-Z0-9_]+)\b", enum_match.group("body"))
		if source_tokens != expected_sources:
			failures.append(
				f"{REVISION_HEADER}: AI target-city source vocabulary changed; "
				f"expected {expected_sources}, found {source_tokens}")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	mapping_match = re.search(
		r"getSASGameRecordAITargetCityChangeSource\s*\([^)]*\)\s*\{(?P<body>.*?)^\}",
		record_text, flags=re.DOTALL | re.MULTILINE)
	if mapping_match is None:
		failures.append(f"{REVISION_SOURCE}: missing AI target-city source-name mapping")
	else:
		mapped_tokens = re.findall(
			r'case\s+(SAS_AI_TARGET_CITY_[A-Z0-9_]+)\s*:\s*return\s+"[A-Z0-9_]+"\s*;',
			mapping_match.group("body"))
		if mapped_tokens != expected_sources:
			failures.append(
				f"{REVISION_SOURCE}: AI target-city source-name mapping changed; "
				f"expected {expected_sources}, found {mapped_tokens}")

	# The normal updater keeps one target-refresh loop and caches its level-2 gate once.
	# Provenance-only old-target/value work remains conditional; writer counts guard new state-changing paths.
	writer_files = {}
	for source_path in (repo_root / "CvGameCoreDLL").glob("*.cpp"):
		if source_path.name == "CvArea.cpp":
			continue  # contains the setter definition, not a caller
		source_text = source_path.read_text(encoding="utf-8", errors="replace")
		count = source_text.count("AI_setTargetCity(")
		if count:
			writer_files[source_path.name] = count
	expected_writer_files = {"CvPlayerAI.cpp": 1, "CvPlayer.cpp": 1, "CvPlot.cpp": 1}
	if writer_files != expected_writer_files:
		failures.append(
			f"AI target-city writer shape changed; expected {expected_writer_files}, found {writer_files}. "
			"Update exact transition provenance/checkpoints together with any new writer.")

	player_ai_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	player_text = (repo_root / PLAYER_SOURCE).read_text(encoding="utf-8", errors="replace")
	plot_text = (repo_root / PLOT_SOURCE).read_text(encoding="utf-8", errors="replace")
	city_text = (repo_root / CITY_SOURCE).read_text(encoding="utf-8", errors="replace")
	for source_text, relative_path, required in (
		(player_ai_text, PLAYER_AI_SOURCE, (
			"SAS_AI_TARGET_CITY_AREA_SEARCH", "SAS_AI_TARGET_CITY_RANDOM_CLEAR",
			"bLogTargetCityChanges", "&iSelectionValue", "logSASGameRecordAITargetCityChanged(",
		)),
		(player_text, PLAYER_SOURCE, (
			"SAS_AI_TARGET_CITY_DIPLO_COORDINATION", "logSASGameRecordAITargetCityChanged(",
		)),
		(plot_text, PLOT_SOURCE, (
			"SAS_AI_TARGET_CITY_AREA_REASSIGN_CLEAR", "logSASGameRecordAITargetCityChanged(",
		)),
		(city_text, CITY_SOURCE, (
			"SAS_AI_TARGET_CITY_CITY_REMOVED", "logSASGameRecordAITargetCityChanged(",
		)),
	):
		for token in required:
			if token not in source_text:
				failures.append(f"{relative_path}: missing AI target-city provenance token {token}")

	for required in (
		"GAME_RECORD_AI_TARGET_CITY_CHANGE", "GAME_RECORD_AI_TARGET_CITIES",
		"selectionValue=%d", "areaAI=%s", "newTargetWarPlan=%s",
		"logSASGameRecordAITargetCities(ePlayer, iGameTurn);",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI target-city diagnostic/checkpoint token {required}")
	return failures


def _strip_cpp_comments(text: str) -> str:
	text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
	return re.sub(r"//.*", "", text)


def check_ai_attitude_breakdown(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (PLAYER_AI_SOURCE, REVISION_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI-attitude diagnostic file: {relative_path}")
	if failures:
		return failures

	expected_getters = [
		"AI_getFirstImpressionAttitude", "AI_getTeamSizeAttitude", "AI_getRankDifferenceAttitude",
		"AI_getCloseBordersAttitude", "AI_getPeaceAttitude", "AI_getSameReligionAttitude",
		"AI_getDifferentReligionAttitude", "AI_getBonusTradeAttitude", "AI_getOpenBordersAttitude",
		"AI_getDefensivePactAttitude", "AI_getRivalDefensivePactAttitude", "AI_getRivalVassalAttitude",
		"AI_getExpansionistAttitude", "AI_getShareWarAttitude", "AI_getFavoriteCivicAttitude",
		"AI_getTradeAttitude", "AI_getRivalTradeAttitude", "AI_getMemoryAttitude",
		"AI_getAttitudeExtra", "AI_getWarAttitude",
	]
	expected_labels = [
		"firstImpression", "teamSize", "rankDifference", "closeBorders", "peace", "sameReligion",
		"differentReligion", "bonusTrade", "openBorders", "defensivePact", "rivalDefensivePact",
		"rivalVassal", "expansionist", "sharedWar", "favoriteCivic", "trade", "rivalTrade",
		"memories", "extra", "war",
	]

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	start = player_text.find("void CvPlayerAI::AI_updateAttitude(PlayerTypes ePlayer")
	end = player_text.find("// for making minor adjustments", start)
	if start < 0 or end < 0:
		return [f"{PLAYER_AI_SOURCE}: could not isolate CvPlayerAI::AI_updateAttitude(PlayerTypes,...)"]
	update_body = _strip_cpp_comments(player_text[start:end])
	getters = re.findall(
		r"(?:int\s+iAttitude\s*=|iAttitude\s*\+=)\s*(AI_get[A-Za-z0-9_]+)\s*\(",
		update_body)
	if getters != expected_getters:
		failures.append(
			f"{PLAYER_AI_SOURCE}: AI_updateAttitude component sequence changed; "
			f"expected {expected_getters}, found {getters}. Update GAME_RECORD_DIPLO_ATTITUDE_BREAKDOWN and this check together.")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	start = record_text.find("static void logSASGameRecordDiplomaticAttitudes(PlayerTypes ePlayer")
	end = record_text.find("static void logSASGameRecordDiploStatus(PlayerTypes ePlayer", start)
	if start < 0 or end < 0:
		return failures + [f"{REVISION_SOURCE}: could not isolate logSASGameRecordDiplomaticAttitudes"]
	record_body = _strip_cpp_comments(record_text[start:end])
	labels = re.findall(r'appendSASGameRecordAttitudeComponent\s*\([^;]*?"([A-Za-z0-9_]+)"', record_body)
	if labels != expected_labels:
		failures.append(
			f"{REVISION_SOURCE}: attitude-breakdown component labels/order changed; "
			f"expected {expected_labels}, found {labels}")
	for required in (
		"GAME_RECORD_DIPLO_ATTITUDE_BREAKDOWN", "componentSum=%+d", "componentValue=%+d",
		"cachedRawValue=%+d", "effectiveValue=%+d", "forcedDelta=%+d",
	):
		if required not in record_body:
			failures.append(f"{REVISION_SOURCE}: missing AI-attitude diagnostic token {required}")
	return failures


def check_strategic_trade_market(repo_root: Path) -> list[str]:
	failures = []
	record_path = repo_root / REVISION_SOURCE
	if not record_path.is_file():
		return [f"missing strategic trade-market diagnostic file: {REVISION_SOURCE}"]

	record_text = record_path.read_text(encoding="utf-8", errors="replace")
	start = record_text.find("static void logSASGameRecordStrategicTradeMarketPair")
	end = record_text.find("static void logSASGameRecordTradeMarket", start)
	if start < 0 or end < 0:
		return [f"{REVISION_SOURCE}: could not isolate logSASGameRecordStrategicTradeMarketPair"]
	record_body = _strip_cpp_comments(record_text[start:end])
	expected_items = [
		"TRADE_MAPS", "TRADE_VASSAL", "TRADE_SURRENDER", "TRADE_OPEN_BORDERS",
		"TRADE_DEFENSIVE_PACT", "TRADE_PERMANENT_ALLIANCE", "TRADE_DISENGAGE",
	]
	items = re.findall(
		r"appendSASGameRecordStrategicTradeStatus\s*\([^;]*?,\s*(TRADE_[A-Z0-9_]+)\s*\);",
		record_body, flags=re.DOTALL)
	if items != expected_items:
		failures.append(
			f"{REVISION_SOURCE}: strategic bilateral trade-status sequence changed; "
			f"expected {expected_items}, found {items}")
	for required in (
		"GAME_RECORD_TRADE_STRATEGIC", "bilateral=%s", "citiesWillCede=%s", "cityDenials=%s",
		"glanceWarTargets=%s", "glanceWarTargetDenials=%s",
	):
		if required not in record_body:
			failures.append(f"{REVISION_SOURCE}: missing strategic trade-market diagnostic token {required}")

	market_start = record_text.find("static void logSASGameRecordTradeMarket(int iGameTurn)")
	market_end = record_text.find("static void logSASGameRecordEnvironment", market_start)
	if market_start < 0 or market_end < 0:
		failures.append(f"{REVISION_SOURCE}: could not isolate logSASGameRecordTradeMarket")
	else:
		market_body = _strip_cpp_comments(record_text[market_start:market_end])
		for required in (
			"if (!isSASGameRecordTradeMarketEnabled())",
			"logSASGameRecordStrategicTradeMarketPair(",
		):
			if required not in market_body:
				failures.append(f"{REVISION_SOURCE}: missing strategic trade-market gate/bridge token {required}")
	return failures


def check_uwai_war_plan_decisions(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, UWAI_AGENT_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing UWAI war-plan provenance file: {relative_path}")
	if failures:
		return failures

	expected = [
		("SAS_UWAI_WAR_PLAN_ILLEGAL_TARGET", "ILLEGAL_TARGET"),
		("SAS_UWAI_WAR_PLAN_VICTORY_DENIAL_DIRECT", "VICTORY_DENIAL_DIRECT"),
		("SAS_UWAI_WAR_PLAN_IMMINENT_NEGATIVE_UTILITY", "IMMINENT_NEGATIVE_UTILITY"),
		("SAS_UWAI_WAR_PLAN_IMMINENT_TIMEOUT", "IMMINENT_TIMEOUT"),
		("SAS_UWAI_WAR_PLAN_PREPARATION_DEADLINE_REACHED", "PREPARATION_DEADLINE_REACHED"),
		("SAS_UWAI_WAR_PLAN_PREPARATION_DEADLINE_NEGATIVE_UTILITY", "PREPARATION_DEADLINE_NEGATIVE_UTILITY"),
		("SAS_UWAI_WAR_PLAN_SEVERE_NEGATIVE_UTILITY", "SEVERE_NEGATIVE_UTILITY"),
		("SAS_UWAI_WAR_PLAN_TARGET_SWITCH", "TARGET_SWITCH"),
		("SAS_UWAI_WAR_PLAN_ATTACKED_RECENT_MATURED", "ATTACKED_RECENT_MATURED"),
		("SAS_UWAI_WAR_PLAN_ACTIVE_TYPE_SWITCH", "ACTIVE_TYPE_SWITCH"),
		("SAS_UWAI_WAR_PLAN_DIRECT_UTILITY_THRESHOLD", "DIRECT_UTILITY_THRESHOLD"),
	]
	expected_tokens = [token for token, _ in expected]

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	enum_match = re.search(r"enum\s+SASGameRecordUWAIWarPlanDecisionReason\s*\{(?P<body>.*?)\};", header_text, flags=re.DOTALL)
	if enum_match is None:
		failures.append(f"{REVISION_HEADER}: missing SASGameRecordUWAIWarPlanDecisionReason")
	else:
		enum_tokens = re.findall(r"^\s*(SAS_UWAI_WAR_PLAN_[A-Z0-9_]+)\s*,?\s*$", enum_match.group("body"), flags=re.MULTILINE)
		if enum_tokens != expected_tokens:
			failures.append(f"{REVISION_HEADER}: UWAI war-plan decision reasons changed; expected {expected_tokens}, found {enum_tokens}")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	mapping_match = re.search(r"getSASGameRecordUWAIWarPlanDecisionReason\s*\([^)]*\)\s*\{(?P<body>.*?)^\}", record_text, flags=re.DOTALL | re.MULTILINE)
	if mapping_match is None:
		failures.append(f"{REVISION_SOURCE}: missing UWAI war-plan reason stringifier")
	else:
		pairs = re.findall(r'case\s+(SAS_UWAI_WAR_PLAN_[A-Z0-9_]+)\s*:\s*return\s+"([A-Z0-9_]+)"\s*;', mapping_match.group("body"))
		if pairs != expected:
			failures.append(f"{REVISION_SOURCE}: UWAI war-plan reason mapping changed; expected {expected}, found {pairs}")
	for required in (
		"GAME_RECORD_AI_WAR_PLAN_DECISION", "planner=UWAI", "action=%s", "reason=%s",
		"comparisonTargetTeam=%d", "comparisonUtility=%d", "decisionValue=%d",
		"decisionThreshold=%d", "victoryDenialBoost=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing foreground-UWAI war-plan diagnostic token {required}")

	uwai_text = (repo_root / UWAI_AGENT_SOURCE).read_text(encoding="utf-8", errors="replace")
	bridge_count = uwai_text.count("logSASGameRecordUWAIWarPlanDecision(")
	if bridge_count != len(expected):
		failures.append(f"{UWAI_AGENT_SOURCE}: expected {len(expected)} realized UWAI war-plan decision bridges, found {bridge_count}")
	for token in expected_tokens:
		if uwai_text.count(token) != 1:
			failures.append(f"{UWAI_AGENT_SOURCE}: expected exactly one realized bridge for {token}, found {uwai_text.count(token)}")
	return failures


def check_ai_diplo_vote_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE, PLAYER_AI_HEADER, TEAM_AI_SOURCE, GAME_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI diplomatic-vote provenance file: {relative_path}")
	if failures:
		return failures

	expected = [
		("SAS_AI_DIPLO_VOTE_TEAM_SELF_ELIGIBLE", "TEAM_SELF_ELIGIBLE"),
		("SAS_AI_DIPLO_VOTE_TEAM_VASSAL_MASTER", "TEAM_VASSAL_MASTER"),
		("SAS_AI_DIPLO_VOTE_TEAM_OWN_DIPLO_VICTORY_ABSTAIN", "TEAM_OWN_DIPLO_VICTORY_ABSTAIN"),
		("SAS_AI_DIPLO_VOTE_TEAM_ATTITUDE_TIE_ABSTAIN", "TEAM_ATTITUDE_TIE_ABSTAIN"),
		("SAS_AI_DIPLO_VOTE_TEAM_BEST_ATTITUDE", "TEAM_BEST_ATTITUDE"),
		("SAS_AI_DIPLO_VOTE_SECRETARY_SELF_OR_MASTER", "SECRETARY_SELF_OR_MASTER"),
		("SAS_AI_DIPLO_VOTE_FRIENDLY_SECRETARY", "FRIENDLY_SECRETARY"),
		("SAS_AI_DIPLO_VOTE_FORCE_CIVIC", "FORCE_CIVIC"),
		("SAS_AI_DIPLO_VOTE_TRADE_ROUTES", "TRADE_ROUTES"),
		("SAS_AI_DIPLO_VOTE_NO_NUKES", "NO_NUKES"),
		("SAS_AI_DIPLO_VOTE_FREE_TRADE", "FREE_TRADE"),
		("SAS_AI_DIPLO_VOTE_OPEN_BORDERS", "OPEN_BORDERS"),
		("SAS_AI_DIPLO_VOTE_DEFENSIVE_PACT", "DEFENSIVE_PACT"),
		("SAS_AI_DIPLO_VOTE_FORCE_PEACE", "FORCE_PEACE"),
		("SAS_AI_DIPLO_VOTE_EMBARGO_RECENT_DEAL", "EMBARGO_RECENT_DEAL"),
		("SAS_AI_DIPLO_VOTE_EMBARGO_UNMET_ABSTAIN", "EMBARGO_UNMET_ABSTAIN"),
		("SAS_AI_DIPLO_VOTE_EMBARGO", "EMBARGO"),
		("SAS_AI_DIPLO_VOTE_FORCE_WAR", "FORCE_WAR"),
		("SAS_AI_DIPLO_VOTE_ASSIGN_CITY", "ASSIGN_CITY"),
		("SAS_AI_DIPLO_VOTE_DEFAULT", "DEFAULT"),
	]
	expected_tokens = [token for token, _ in expected]

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	enum_match = re.search(r"enum\s+SASGameRecordAIDiploVoteReason\s*\{(?P<body>.*?)\};", header_text, flags=re.DOTALL)
	if enum_match is None:
		failures.append(f"{REVISION_HEADER}: missing SASGameRecordAIDiploVoteReason")
	else:
		enum_tokens = re.findall(r"^\s*(SAS_AI_DIPLO_VOTE_[A-Z0-9_]+)\s*,?\s*$", enum_match.group("body"), flags=re.MULTILINE)
		if enum_tokens != expected_tokens:
			failures.append(f"{REVISION_HEADER}: AI diplomatic-vote reasons changed; expected {expected_tokens}, found {enum_tokens}")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	mapping_match = re.search(r"getSASGameRecordAIDiploVoteReason\s*\([^)]*\)\s*\{(?P<body>.*?)^\}", record_text, flags=re.DOTALL | re.MULTILINE)
	if mapping_match is None:
		failures.append(f"{REVISION_SOURCE}: missing AI diplomatic-vote reason stringifier")
	else:
		pairs = re.findall(r'case\s+(SAS_AI_DIPLO_VOTE_[A-Z0-9_]+)\s*:\s*return\s+"([A-Z0-9_]+)"\s*;', mapping_match.group("body"))
		if pairs != expected:
			failures.append(f"{REVISION_SOURCE}: AI diplomatic-vote reason mapping changed; expected {expected}, found {pairs}")
	for required in (
		"GAME_RECORD_AI_DIPLO_VOTE", "triggeredId=%d", "choice=%s", "reason=%s",
		"decisionValue=%s", "decisionThreshold=%s", "randomRoll=%s",
		"GAME_RECORD_AI_ELECTION_CHOICE", "selectionId=%d", "selectedRandomValue=%d", "selectedVictoryBoost=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI diplomatic-vote/proposal diagnostic token {required}")

	player_header = (repo_root / PLAYER_AI_HEADER).read_text(encoding="utf-8", errors="replace")
	if "bool bPropose, int iTriggeredVoteId = -1" not in player_header:
		failures.append(f"{PLAYER_AI_HEADER}: AI_diploVote must keep optional triggered-vote identity for real-ballot provenance")
	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	if "!isHuman() && iTriggeredVoteId >= 0 && getSASGameRecordLogLevel() >= 2" not in player_text:
		failures.append(f"{PLAYER_AI_SOURCE}: actual AI ballot logging must remain pre-gated by non-human, triggered id and level 2+")
	for token in expected_tokens:
		if token not in player_text and token != "SAS_AI_DIPLO_VOTE_DEFAULT":
			failures.append(f"{PLAYER_AI_SOURCE}: no live AI_diploVote path references {token}")

	game_text = (repo_root / GAME_SOURCE).read_text(encoding="utf-8", errors="replace")
	actual_ballot_calls = game_text.count("AI_diploVote(kOptionData, eVoteSource, false, pData->getID())")
	if actual_ballot_calls != 2:
		failures.append(f"{GAME_SOURCE}: expected two real ballot calls to pass triggered vote id, found {actual_ballot_calls}")

	team_text = (repo_root / TEAM_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	choose_match = re.search(r"int\s+CvTeamAI::AI_chooseElection\([^)]*\)\s+const\s*\{(?P<body>.*?)^\}", team_text, flags=re.DOTALL | re.MULTILINE)
	if choose_match is None:
		failures.append(f"{TEAM_AI_SOURCE}: could not locate AI_chooseElection")
	else:
		body = choose_match.group("body")
		if body.count("SyncRandNum(10000)") != 1:
			failures.append(f"{TEAM_AI_SOURCE}: AI_chooseElection must retain exactly one existing proposal-selection RNG call site")
		if body.count("logSASGameRecordAIElectionChoice(") != 1:
			failures.append(f"{TEAM_AI_SOURCE}: expected exactly one compact AI election-choice recorder bridge")
	return failures


def check_ai_conquer_city_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI captured-city provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	expected_outcomes = [
		"SAS_AI_CONQUER_CITY_KEEP", "SAS_AI_CONQUER_CITY_RAZE", "SAS_AI_CONQUER_CITY_LIBERATE",
	]
	expected_reasons = [
		"SAS_AI_CONQUER_CITY_CANNOT_RAZE", "SAS_AI_CONQUER_CITY_DOMINATION3_PRIMARY_AREA_KEEP",
		"SAS_AI_CONQUER_CITY_CULTURE_VICTORY", "SAS_AI_CONQUER_CITY_UNLIKELY_LONG_TERM_BENEFIT",
		"SAS_AI_CONQUER_CITY_EARLY_REMOTE_BARB", "SAS_AI_CONQUER_CITY_EARLY_REMOTE_NONBARB",
		"SAS_AI_CONQUER_CITY_BARBARIAN_VALUE", "SAS_AI_CONQUER_CITY_NORMAL_VALUE",
		"SAS_AI_CONQUER_CITY_LIBERATION", "SAS_AI_CONQUER_CITY_LIBERATION_WITHHELD_HOSTAGE",
	]
	for enum_name, expected in (("SASGameRecordAIConquerCityOutcome", expected_outcomes), ("SASGameRecordAIConquerCityReason", expected_reasons)):
		m = re.search(r"enum\s+" + enum_name + r"\s*\{(?P<body>.*?)\};", header_text, flags=re.DOTALL)
		if m is None:
			failures.append(f"{REVISION_HEADER}: missing {enum_name}")
			continue
		tokens = re.findall(r"^\s*(SAS_AI_CONQUER_CITY_[A-Z0-9_]+)\s*,?\s*$", m.group("body"), flags=re.MULTILINE)
		if tokens != expected:
			failures.append(f"{REVISION_HEADER}: {enum_name} changed; expected {expected}, found {tokens}")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_CONQUER_CITY_DECISION", "outcome=%s", "reason=%s", "valueValid=%d",
		"razeValueBeforeRandom=%d", "random=%d", "razeValue=%d", "componentsValid=%d",
		"distanceAndLocalPower=%d", "maintenanceDelta=%+d", "populationDelta=%+d",
		"personalityDominationDelta=%+d", "otherDelta=%+d", "barbarianRollPassed=%d", "liberationPlayer=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI captured-city diagnostic token {required}")
	for token in expected_outcomes + expected_reasons:
		if token not in record_text:
			failures.append(f"{REVISION_SOURCE}: captured-city stringifier missing {token}")

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	for token in expected_reasons:
		if token not in player_text:
			failures.append(f"{PLAYER_AI_SOURCE}: no AI_conquerCity path references {token}")
	if "bLogSASConquerCityDecision = (gGameRecordLogLevel >= 2)" not in player_text:
		failures.append(f"{PLAYER_AI_SOURCE}: captured-city valuation logging must retain the cached level-2 gate")
	if player_text.count("SyncRandSuccess100(iRazeValue)") != 1:
		failures.append(f"{PLAYER_AI_SOURCE}: inherited Barbarian raze percentage roll must remain a single live RNG call site")
	if player_text.count("SyncRandNum(6)") < 1:
		failures.append(f"{PLAYER_AI_SOURCE}: normal raze valuation lost its existing SyncRandNum(6) random component")
	return failures


def check_ai_great_person_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, UNIT_AI_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI Great Person provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	expected_actions = [
		"SAS_AI_GREAT_PERSON_DISCOVER_TECH", "SAS_AI_GREAT_PERSON_TRADE_MISSION", "SAS_AI_GREAT_PERSON_MOVE_TO_TRADE_MISSION",
		"SAS_AI_GREAT_PERSON_GREAT_WORK", "SAS_AI_GREAT_PERSON_MOVE_TO_GREAT_WORK", "SAS_AI_GREAT_PERSON_GOLDEN_AGE",
		"SAS_AI_GREAT_PERSON_JOIN_CITY", "SAS_AI_GREAT_PERSON_MOVE_TO_JOIN_CITY", "SAS_AI_GREAT_PERSON_CONSTRUCT_BUILDING",
		"SAS_AI_GREAT_PERSON_MOVE_TO_CONSTRUCT_BUILDING", "SAS_AI_GREAT_PERSON_HURRY_BUILDING", "SAS_AI_GREAT_PERSON_MOVE_TO_HURRY_BUILDING",
		"SAS_AI_GREAT_PERSON_DANGER_DISCOVER_TECH", "SAS_AI_GREAT_PERSON_RECON_SPY", "SAS_AI_GREAT_PERSON_RETREAT",
		"SAS_AI_GREAT_PERSON_STRANDED", "SAS_AI_GREAT_PERSON_SAFETY", "SAS_AI_GREAT_PERSON_SKIP",
	]
	m = re.search(r"enum\s+SASGameRecordAIGreatPersonAction\s*\{(?P<body>.*?)\};", header_text, flags=re.DOTALL)
	if m is None:
		failures.append(f"{REVISION_HEADER}: missing SASGameRecordAIGreatPersonAction")
	else:
		tokens = re.findall(r"^\s*(SAS_AI_GREAT_PERSON_[A-Z0-9_]+)\s*,?\s*$", m.group("body"), flags=re.MULTILINE)
		if tokens != expected_actions:
			failures.append(f"{REVISION_HEADER}: Great Person action vocabulary changed; expected {expected_actions}, found {tokens}")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_GREAT_PERSON_DECISION", "action=%s", "choiceRank=%d", "selectedValue=%d", "threshold=%d",
		"slow=%d", "slowBaseValue=%d", "slowPathTurns=%d", "slowMissionAI=%d", "slowCityId=%d",
		"specialist=%s", "building=%s", "discover=%d", "discoverTech=%s", "goldenAge=%d", "trade=%d", "culture=%d",
		"targetX=%d", "targetY=%d", "previousMissionAI=%d", "previousTargetX=%d", "previousTargetY=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI Great Person diagnostic token {required}")
	for token in expected_actions:
		if token not in record_text:
			failures.append(f"{REVISION_SOURCE}: Great Person action stringifier missing {token}")

	unit_text = (repo_root / UNIT_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	if "bLogSASGreatPersonDecision = (gGameRecordLogLevel >= 2)" not in unit_text:
		failures.append(f"{UNIT_AI_SOURCE}: Great Person provenance must retain the cached level-2 gate")
	if "if (bLogCultureGreatArtistDecision || bLogSASGreatPersonDecision)" not in unit_text:
		failures.append(f"{UNIT_AI_SOURCE}: previous Great Person mission state must remain shared/gated between Artist diagnostics and SASGameRecord")
	for token in expected_actions:
		if token not in unit_text:
			failures.append(f"{UNIT_AI_SOURCE}: no AI_greatPersonMove path references {token}")
	if not re.search(r"SAS_AI_GREAT_PERSON_DANGER_DISCOVER_TECH\s*,\s*-1\s*,\s*-1\s*,", unit_text):
		failures.append(f"{UNIT_AI_SOURCE}: danger-discover fallback must keep choiceRank/selectedValue at -1")
	# Fallback helpers can execute AI movement immediately; the row schema calls targetX/targetY the realized waypoint/action plot, so do not reuse often-empty or longer-term MissionAI target metadata here.
	for action in ("RECON_SPY", "RETREAT", "SAFETY"):
		if not re.search(rf"SAS_AI_GREAT_PERSON_{action}[^;]*&getPlot\(\)[^;]*ePreviousMissionAI", unit_text):
			failures.append(f"{UNIT_AI_SOURCE}: Great Person {action} fallback must report the realized post-helper waypoint/action plot")
	if len(re.findall(r"SAS_AI_GREAT_PERSON_STRANDED[^;]*&getPlot\(\)[^;]*ePreviousMissionAI", unit_text)) != 2:
		failures.append(f"{UNIT_AI_SOURCE}: both Great Person stranded fallbacks must report the realized post-helper waypoint/action plot")
	# Great Generals deliberately remain on their separate AI_generalMove path. Guard the schema boundary rather than folding unrelated BBAI logic into this row.
	gp_function = re.search(r"void\s+CvUnitAI::AI_greatPersonMove\(\)\s*\{(?P<body>.*?)^\}", unit_text, flags=re.DOTALL | re.MULTILINE)
	if gp_function is None:
		failures.append(f"{UNIT_AI_SOURCE}: could not locate AI_greatPersonMove")
	else:
		gp_body = gp_function.group("body")
		if "logSASGameRecordAIGreatPersonDecision" not in gp_body:
			failures.append(f"{UNIT_AI_SOURCE}: Great Person provenance bridge missing from AI_greatPersonMove")
		# The recorder must consume the live comparison, never re-run GP valuation/path/action helpers. These counts describe the inherited gameplay pass itself.
		for call, expected_count in (
			("generatePath(", 1), ("AI_calculateGoldenAgeValue(", 1), ("AI_tradeMissionValue(", 1), ("AI_greatWorkValue(", 1),
			("AI_doTradeMission(", 1), ("AI_doGreatWork(", 1), ("AI_goldenAge(", 1), ("AI_discover()", 1),
			("AI_reconSpy(5)", 1), ("AI_retreatToCity()", 1), ("AI_handleStranded()", 2), ("AI_safety()", 1),
		):
			actual_count = gp_body.count(call)
			if actual_count != expected_count:
				failures.append(f"{UNIT_AI_SOURCE}: AI_greatPersonMove {call} count changed; expected {expected_count}, found {actual_count}")
		if "SyncRand" in gp_body:
			failures.append(f"{UNIT_AI_SOURCE}: AI_greatPersonMove gained a direct synchronized RNG call; provenance must not add RNG")
	general_function = re.search(r"void\s+CvUnitAI::AI_generalMove\(\)\s*\{(?P<body>.*?)^\}", unit_text, flags=re.DOTALL | re.MULTILINE)
	if general_function is None:
		failures.append(f"{UNIT_AI_SOURCE}: could not locate separate AI_generalMove Great-General path")
	elif "logSASGameRecordAIGreatPersonDecision" in general_function.group("body"):
		failures.append(f"{UNIT_AI_SOURCE}: ordinary Great Person provenance must not be bridged into AI_generalMove")
	return failures


def check_ai_great_general_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, UNIT_AI_HEADER, UNIT_AI_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI Great General provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	expected_stages = [
		"SAS_AI_GREAT_GENERAL_PREFERRED_INSTRUCTOR", "SAS_AI_GREAT_GENERAL_FIRST_ACADEMY", "SAS_AI_GREAT_GENERAL_FIRST_INSTRUCTOR",
		"SAS_AI_GREAT_GENERAL_DANGER_LEAD",
		"SAS_AI_GREAT_GENERAL_OFFENSE_LEAD_ATTACK_CITY", "SAS_AI_GREAT_GENERAL_OFFENSE_LEAD_ATTACK",
		"SAS_AI_GREAT_GENERAL_JOIN_LIMIT_2", "SAS_AI_GREAT_GENERAL_ACADEMY_LIMIT_2", "SAS_AI_GREAT_GENERAL_JOIN_LIMIT_4",
		"SAS_AI_GREAT_GENERAL_RANDOM_CONSTRUCT", "SAS_AI_GREAT_GENERAL_FINAL_JOIN", "SAS_AI_GREAT_GENERAL_RETREAT",
		"SAS_AI_GREAT_GENERAL_STRANDED", "SAS_AI_GREAT_GENERAL_SAFETY", "SAS_AI_GREAT_GENERAL_SKIP",
	]
	expected_actions = [
		"SAS_AI_GREAT_GENERAL_ACTION_JOIN", "SAS_AI_GREAT_GENERAL_ACTION_CONSTRUCT", "SAS_AI_GREAT_GENERAL_ACTION_LEAD",
		"SAS_AI_GREAT_GENERAL_ACTION_RETREAT", "SAS_AI_GREAT_GENERAL_ACTION_STRANDED", "SAS_AI_GREAT_GENERAL_ACTION_SAFETY",
		"SAS_AI_GREAT_GENERAL_ACTION_SKIP",
	]
	for enum_name, expected in (("SASGameRecordAIGreatGeneralStage", expected_stages), ("SASGameRecordAIGreatGeneralAction", expected_actions)):
		m = re.search(rf"enum\s+{enum_name}\s*\{{(?P<body>.*?)\}};", header_text, flags=re.DOTALL)
		if m is None:
			failures.append(f"{REVISION_HEADER}: missing {enum_name}")
		else:
			tokens = re.findall(r"^\s*(SAS_AI_GREAT_GENERAL_[A-Z0-9_]+)\s*,?\s*$", m.group("body"), flags=re.MULTILINE)
			if tokens != expected:
				failures.append(f"{REVISION_HEADER}: {enum_name} vocabulary changed; expected {expected}, found {tokens}")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_GREAT_GENERAL_DECISION", "unitAI=%s", "area=%d", "areaAI=%d", "action=%s", "stage=%s", "move=%d", "selectedValue=%d", "limit=%d",
		"valueThreshold=%d", "minStrength=%d", "minHealing=%d", "preferInstructorFirst=%d", "preferThroughEra=%d",
		"randomConstructRoll=%d", "specialist=%s", "building=%s", "targetCityId=%d", "targetUnitId=%d", "targetUnitAI=%s",
		"targetStrengthScore=%d", "targetHealing=%d", "leadByHealing=%d", "targetX=%d", "targetY=%d", "waypointX=%d",
		"waypointY=%d", "previousMissionAI=%d", "previousTargetX=%d", "previousTargetY=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI Great General diagnostic token {required}")
	for token in expected_stages + expected_actions:
		if token not in record_text:
			failures.append(f"{REVISION_SOURCE}: Great General stringifier missing {token}")

	# Recorder owns the full context schema; CvUnitAI.h should expose only the opaque optional pointer used by the three Great-General-only helpers.
	for required in ("struct SASGreatGeneralChoiceContext", "resetChoice()", "void initialize(", "void prepare("):
		if required not in header_text:
			failures.append(f"{REVISION_HEADER}: missing recorder-owned Great General context token {required}")
	unit_header_text = (repo_root / UNIT_AI_HEADER).read_text(encoding="utf-8", errors="replace")
	for required in ("struct SASGreatGeneralChoiceContext;", "pSASChoiceContext = NULL"):
		if required not in unit_header_text:
			failures.append(f"{UNIT_AI_HEADER}: missing opaque optional Great General helper context token {required}")

	unit_text = (repo_root / UNIT_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	if "bLogSASGreatGeneralDecision = (gGameRecordLogLevel >= 2)" not in unit_text:
		failures.append(f"{UNIT_AI_SOURCE}: Great General provenance must retain the cached level-2 gate")
	for token in expected_stages:
		if token not in unit_text:
			failures.append(f"{UNIT_AI_SOURCE}: no AI_generalMove path references {token}")
	general_function = re.search(r"void\s+CvUnitAI::AI_generalMove\(\)\s*\{(?P<body>.*?)^\}", unit_text, flags=re.DOTALL | re.MULTILINE)
	if general_function is None:
		failures.append(f"{UNIT_AI_SOURCE}: could not locate AI_generalMove")
	else:
		body = general_function.group("body")
		if "logSASGameRecordAIGreatGeneralDecision" not in body:
			failures.append(f"{UNIT_AI_SOURCE}: Great General provenance bridge missing from AI_generalMove")
		# Preserve the policy/RNG structure after practical 6495 removes the exact duplicate Academy/Instructor passes accidentally introduced in practical 5081. Optional helper context may expose live results, but must not add another helper/search pass.
		for call, expected_count in (
			("AI_join(", 5), ("AI_construct(", 3), ("AI_lead(", 3), ("AI_retreatToCity(", 1),
			("AI_handleStranded(", 1), ("AI_safety(", 1), ("SyncRandOneChanceIn(3)", 1),
		):
			actual_count = body.count(call)
			if actual_count != expected_count:
				failures.append(f"{UNIT_AI_SOURCE}: AI_generalMove {call} count changed; expected {expected_count}, found {actual_count}")
		if body.count("prepareSASGreatGeneralChoiceContext(") != 11:
			failures.append(f"{UNIT_AI_SOURCE}: expected 11 prepared helper stages before Great General Join/Construct/Lead passes after redundant-pass removal")
		if body.count("logSASGameRecordAIGreatGeneralDecision") != 4:
			failures.append(f"{UNIT_AI_SOURCE}: expected four direct non-consuming/fallback Great General bridges (retreat/stranded/safety/skip)")
		# Retreat/safety often have no MissionAI plot and stranded can retain a longer-term MissionAI destination; waypoint must be the realized post-helper unit plot.
		if body.count("kSASGreatGeneralChoice.pWaypointPlot = &getPlot();") != 3:
			failures.append(f"{UNIT_AI_SOURCE}: Great General retreat/stranded/safety fallbacks must report the realized post-helper waypoint")
		if body.count("kSASGreatGeneralChoice.bMove = (pSASGreatGeneralDecisionPlot != NULL && kSASGreatGeneralChoice.pWaypointPlot != pSASGreatGeneralDecisionPlot);") != 3:
			failures.append(f"{UNIT_AI_SOURCE}: Great General fallback move flag must compare realized waypoint against the saved decision origin")
		if not re.search(r"bool\s+const\s+bRandomConstructRoll\s*=\s*SyncRandOneChanceIn\(3\);", body):
			failures.append(f"{UNIT_AI_SOURCE}: late construct gate must preserve the existing single 1-in-3 RNG result for provenance")
	# The three helper scans are Great-General-only in this codebase; their optional context must observe the selected live candidate rather than trigger a second search.
	for helper_name in ("AI_join", "AI_construct", "AI_lead"):
		m = re.search(rf"bool\s+CvUnitAI::{helper_name}\([^)]*\)\s*\{{(?P<body>.*?)^\}}", unit_text, flags=re.DOTALL | re.MULTILINE)
		if m is None:
			failures.append(f"{UNIT_AI_SOURCE}: could not locate {helper_name}")
		else:
			helper_body = m.group("body")
			if "pSASChoiceContext" not in helper_body:
				failures.append(f"{UNIT_AI_SOURCE}: {helper_name} no longer exposes its selected live target/value to the optional recorder context")
			emit_token = "logSASGameRecordAIGreatGeneralDecision(*this, *pSASChoiceContext)"
			if helper_body.count(emit_token) != 1:
				failures.append(f"{UNIT_AI_SOURCE}: {helper_name} must emit exactly one successful prepared Great General row")
			emit_pos = helper_body.find(emit_token)
			gate_pos = helper_body.rfind("if (pSASChoiceContext != NULL)", 0, emit_pos)
			if emit_pos >= 0 and gate_pos < 0:
				failures.append(f"{UNIT_AI_SOURCE}: {helper_name} Great General recorder emission must remain pre-gated by the optional context")
			action_positions = [pos for pos in (helper_body.find("pushMission("), helper_body.find("pushGroupMoveTo(")) if pos >= 0]
			if emit_pos >= 0 and action_positions and emit_pos > min(action_positions):
				failures.append(f"{UNIT_AI_SOURCE}: {helper_name} Great General provenance must emit before its existing mission/move push")
	return failures


def check_ai_diplo_contact_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (CV_ENUMS_HEADER, GAME_CORE_UTILS_SOURCE, REVISION_SOURCE, PLAYER_AI_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI diplomatic-contact provenance file: {relative_path}")
	if failures:
		return failures

	enum_text = (repo_root / CV_ENUMS_HEADER).read_text(encoding="utf-8", errors="replace")
	enum_match = re.search(r"ENUM_START\(Contact,\s*CONTACT\)(?P<body>.*?)ENUM_END\(Contact,\s*CONTACT\)", enum_text, flags=re.DOTALL)
	if enum_match is None:
		return [f"{CV_ENUMS_HEADER}: could not locate Contact enum block"]
	enum_tokens = re.findall(r"^\s*(CONTACT_[A-Z0-9_]+)\s*,", enum_match.group("body"), flags=re.MULTILINE)
	if not enum_tokens or len(enum_tokens) != len(set(enum_tokens)):
		failures.append(f"{CV_ENUMS_HEADER}: invalid/duplicate Contact enumerators found")

	utils_text = (repo_root / GAME_CORE_UTILS_SOURCE).read_text(encoding="utf-8", errors="replace")
	helper_match = re.search(r"char\s+const\*\s+getSASContactType\s*\([^)]*\)\s*\{(?P<body>.*?)^\}", utils_text, flags=re.DOTALL | re.MULTILINE)
	if helper_match is None:
		failures.append(f"{GAME_CORE_UTILS_SOURCE}: missing getSASContactType definition")
	else:
		case_pairs = re.findall(r'case\s+(CONTACT_[A-Z0-9_]+)\s*:\s*return\s+"([^"]+)"\s*;', helper_match.group("body"))
		case_tokens = [token for token, _ in case_pairs]
		missing = [token for token in enum_tokens if token not in case_tokens]
		extra = [token for token in case_tokens if token not in enum_tokens]
		if missing:
			failures.append(f"{GAME_CORE_UTILS_SOURCE}: getSASContactType missing {', '.join(missing)}")
		if extra:
			failures.append(f"{GAME_CORE_UTILS_SOURCE}: getSASContactType has non-enum case(s): {', '.join(extra)}")
		if len(case_tokens) != len(set(case_tokens)):
			failures.append(f"{GAME_CORE_UTILS_SOURCE}: duplicate getSASContactType case(s) found")
		for token, label in case_pairs:
			if token != label:
				failures.append(f"{GAME_CORE_UTILS_SOURCE}: {token} maps to {label!r}, expected identical raw enum token")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_DIPLO_CONTACT", "contact=%s", "delivery=%s", "attitudeValue=%d",
		"subject=%s", "aiGives=%s", "aiReceives=%s", "getSASContactType",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI diplomatic-contact diagnostic token {required}")

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	bridged_contacts = re.findall(
		r"logSASGameRecordAIDiploContactIntent\s*\([^;]*?\b(CONTACT_[A-Z0-9_]+)\b",
		player_text, flags=re.DOTALL)
	bridged_set = set(bridged_contacts)
	expected_generic = {
		"CONTACT_RELIGION_PRESSURE", "CONTACT_CIVIC_PRESSURE", "CONTACT_JOIN_WAR",
		"CONTACT_STOP_TRADING", "CONTACT_GIVE_HELP", "CONTACT_ASK_FOR_HELP",
		"CONTACT_DEMAND_TRIBUTE", "CONTACT_OPEN_BORDERS", "CONTACT_DEFENSIVE_PACT",
		"CONTACT_PERMANENT_ALLIANCE", "CONTACT_TRADE_TECH", "CONTACT_TRADE_MAP",
	}
	missing_generic = sorted(expected_generic - bridged_set)
	unexpected_generic = sorted(bridged_set - expected_generic)
	if missing_generic:
		failures.append(f"{PLAYER_AI_SOURCE}: generic realized-contact provenance missing {', '.join(missing_generic)}")
	if unexpected_generic:
		failures.append(f"{PLAYER_AI_SOURCE}: unexpected generic realized-contact class(es): {', '.join(unexpected_generic)}")
	for delivery in ('"HUMAN_CONTACT"', '"AI_DEAL"'):
		if delivery not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing realized diplomatic-contact delivery marker {delivery}")
	if "gGameRecordLogLevel >= 2" not in player_text:
		failures.append(f"{PLAYER_AI_SOURCE}: realized diplomatic-contact bridges must remain level-2 pre-gated")

	# Peace and proactive resource exchange already have richer dedicated provenance;
	# keeping them out of the generic row prevents duplicate rows for the same intent.
	for specialized in ("GAME_RECORD_AI_PEACE_DECISION", "GAME_RECORD_AI_BONUS_TRADE_DECISION"):
		if specialized not in record_text:
			failures.append(f"{REVISION_SOURCE}: specialized contact provenance missing {specialized}")
	if "CONTACT_PEACE_TREATY" in bridged_set or "CONTACT_TRADE_BONUS" in bridged_set:
		failures.append(f"{PLAYER_AI_SOURCE}: peace/resource intent should remain in specialized provenance rather than duplicate GAME_RECORD_AI_DIPLO_CONTACT")
	return failures


def check_ai_help_tribute_request_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE, PLAYER_AI_HEADER, UWAI_AGENT_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI help/tribute request provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	expected_origins = ["SAS_AI_HUMAN_REQUEST_AI_DO_DIPLO", "SAS_AI_HUMAN_REQUEST_UWAI_AMEND_TENSIONS"]
	m = re.search(r"enum\s+SASGameRecordAIHumanRequestOrigin\s*\{(?P<body>.*?)\};", header_text, flags=re.DOTALL)
	if m is None:
		failures.append(f"{REVISION_HEADER}: missing SASGameRecordAIHumanRequestOrigin")
	else:
		tokens = re.findall(r"^\s*(SAS_AI_HUMAN_REQUEST_[A-Z0-9_]+)\s*,?\s*$", m.group("body"), flags=re.MULTILINE)
		if tokens != expected_origins:
			failures.append(f"{REVISION_HEADER}: AI human-request origin vocabulary changed; expected {expected_origins}, found {tokens}")
	for token in ("SASGameRecordAIHelpRequestContext", "SASGameRecordAITributeRequestContext", "logSASGameRecordAIHelpRequest", "logSASGameRecordAITributeRequest"):
		if token not in header_text:
			failures.append(f"{REVISION_HEADER}: missing help/tribute provenance declaration {token}")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_HELP_REQUEST", "origin=%s", "humanAssets=%d", "aiTeamAssets=%d", "bestTech=%s",
		"techValue=%d", "bestCityId=%d", "cityValue=%d", "bestCityLiberation=%d", "cityChoiceRoll=%d", "selected=%s:%s",
		"GAME_RECORD_AI_TRIBUTE_REQUEST", "demand=%s", "minValueX100=%d", "finalDealValue=%d", "mostUsefulTech=%s",
		"selectedTech=%s", "selectedTechSortScore=%d", "selectedTechTradeValue=%d", "mapTradeValue=%d", "mapAdded=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI help/tribute request diagnostic token {required}")
	for token in expected_origins:
		if record_text.count(f"case {token}:") != 1:
			failures.append(f"{REVISION_SOURCE}: human-request origin {token} must have exactly one stringifier case")
	for demand in ("DEMAND_GOLD", "DEMAND_MAP", "DEMAND_TECH", "DEMAND_BONUS", "DEMAND_GOLD_PER_TURN", "DEMAND_CITY"):
		if record_text.count(f"case {demand}:") != 1:
			failures.append(f"{REVISION_SOURCE}: tribute demand {demand} must have exactly one recorder stringifier case")

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	player_header = (repo_root / PLAYER_AI_HEADER).read_text(encoding="utf-8", errors="replace")
	for signature in (
		"AI_askHelp(PlayerTypes eHuman, SASGameRecordAIHelpRequestContext* pSASContext = NULL)",
		"AI_demandTribute(PlayerTypes eHuman, AIDemandTypes eDemand, SASGameRecordAITributeRequestContext* pSASContext = NULL)",
	):
		if signature not in player_header:
			failures.append(f"{PLAYER_AI_HEADER}: missing optional recorder helper signature {signature}")

	ask = re.search(r"bool\s+CvPlayerAI::AI_askHelp\([^)]*\)\s*\{(?P<body>.*?)^\}\s*\n\n/\*\s*Same conditions ensured by the caller", player_text, flags=re.DOTALL | re.MULTILINE)
	if ask is None:
		failures.append(f"{PLAYER_AI_SOURCE}: could not locate AI_askHelp")
	else:
		body = ask.group("body")
		for token, expected in (("AI_bestRequestCity(", 1), ("AI_bestTech(", 1), ("AI_techTradeVal(", 1), ("AI_cityTradeVal(", 1), ("SyncRandSuccess(", 2), ("logSASGameRecordAIHelpRequest(", 1), ("logSASGameRecordAIDiploContactIntent(", 1), ("gDLL->beginDiplomacy(", 1)):
			actual = body.count(token)
			if actual != expected:
				failures.append(f"{PLAYER_AI_SOURCE}: AI_askHelp live call count for {token} changed; expected {expected}, found {actual}")
		if "if (pSASContext != NULL) logSASGameRecordAIHelpRequest" not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_askHelp specialized row must remain optional-context pre-gated")
		if "iCityChoiceRoll = (bCityChoiceRollReached ? (mainItem.m_eItemType == TRADE_CITIES ? 1 : 0) : -1)" not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_askHelp must infer the existing 2/3 city-roll result without drawing recorder RNG")
		if body.find("logSASGameRecordAIHelpRequest(") > body.find("gDLL->beginDiplomacy("):
			failures.append(f"{PLAYER_AI_SOURCE}: AI help-request provenance must emit before beginDiplomacy")

	tribute = re.search(r"bool\s+CvPlayerAI::AI_demandTribute\([^)]*\)\s*\{(?P<body>.*?)^\}\s*\n\n// advc\.ctr: For tribute and help requests", player_text, flags=re.DOTALL | re.MULTILINE)
	if tribute is None:
		failures.append(f"{PLAYER_AI_SOURCE}: could not locate AI_demandTribute")
	else:
		body = tribute.group("body")
		for token, expected in (("AI_bestTech(", 1), ("AI_techTradeVal(", 1), ("AI_mapTradeVal(", 1), ("AI_dealVal(", 1), ("AI_bestRequestCity(", 1), ("SyncRandNum(10000)", 1), ("SyncRandFract(", 1), ("logSASGameRecordAITributeRequest(", 1), ("logSASGameRecordAIDiploContactIntent(", 1), ("gDLL->beginDiplomacy(", 1)):
			actual = body.count(token)
			if actual != expected:
				failures.append(f"{PLAYER_AI_SOURCE}: AI_demandTribute live call count for {token} changed; expected {expected}, found {actual}")
		if "int const iSelectedTechTradeValue = GET_TEAM(getTeam()).AI_techTradeVal" not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: tribute tech valuation must be cached from the existing live call instead of repeated for logging")
		if "int const iMapTradeValue = GET_TEAM(getTeam()).AI_mapTradeVal" not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: tribute map valuation must be cached from the existing live call instead of repeated for logging")
		if "if (pSASContext != NULL)" not in body or "pSASContext->iFinalDealValue = iDemandDealValue" not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: tribute recorder output must remain optional and use the existing final deal value")
		if body.find("logSASGameRecordAITributeRequest(") > body.find("gDLL->beginDiplomacy("):
			failures.append(f"{PLAYER_AI_SOURCE}: AI tribute-request provenance must emit before beginDiplomacy")
		if body.count("logSASGameRecordAIBonusDemandDecision(") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: revision-87 bonus-demand detail must remain authoritative and emitted exactly once")

	for token in expected_origins:
		if player_text.count(token) + (repo_root / UWAI_AGENT_SOURCE).read_text(encoding="utf-8", errors="replace").count(token) < 1:
			failures.append(f"AI help/tribute callers no longer assign origin {token}")
	uwai_text = (repo_root / UWAI_AGENT_SOURCE).read_text(encoding="utf-8", errors="replace")
	m = re.search(r"bool\s+UWAI::Player::amendTensions\([^)]*\)\s*\{(?P<body>.*?)^\}\s*\n", uwai_text, flags=re.DOTALL | re.MULTILINE)
	if m is None:
		failures.append(f"{UWAI_AGENT_SOURCE}: could not locate amendTensions")
	else:
		body = m.group("body")
		if body.count("SAS_AI_HUMAN_REQUEST_UWAI_AMEND_TENSIONS") != 2:
			failures.append(f"{UWAI_AGENT_SOURCE}: amendTensions must tag both tribute and help request helpers with the UWAI origin")
		if body.count("AI_demandTribute(eHuman, eLoopAIDemand, pSASTributeRequest)") != 1:
			failures.append(f"{UWAI_AGENT_SOURCE}: UWAI tribute helper must pass the optional recorder context exactly once")
		if body.count("AI_askHelp(eHuman, pSASHelpRequest)") != 1:
			failures.append(f"{UWAI_AGENT_SOURCE}: UWAI help helper must pass the optional recorder context exactly once")
	return failures


def check_ai_give_help_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI give-help provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	expected_origins = [
		"SAS_AI_GIVE_HELP_RELATION_RESOURCE",
		"SAS_AI_GIVE_HELP_RELATION_TECH",
		"SAS_AI_GIVE_HELP_PROACTIVE_TECH",
	]
	m = re.search(r"enum\s+SASGameRecordAIGiveHelpOrigin\s*\{(?P<body>.*?)\};", header_text, flags=re.DOTALL)
	if m is None:
		failures.append(f"{REVISION_HEADER}: missing SASGameRecordAIGiveHelpOrigin")
	else:
		tokens = re.findall(r"^\s*(SAS_AI_GIVE_HELP_[A-Z0-9_]+)\s*,?\s*$", m.group("body"), flags=re.MULTILINE)
		if tokens != expected_origins:
			failures.append(f"{REVISION_HEADER}: AI give-help origin vocabulary changed; expected {expected_origins}, found {tokens}")
	if "logSASGameRecordAIGiveHelpDecision" not in header_text:
		failures.append(f"{REVISION_HEADER}: missing give-help provenance logger declaration")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_GIVE_HELP_DECISION", "origin=%s", "sameTeam=%d", "targetVassal=%d", "giverVassal=%d",
		"attitudeValue=%d", "proactiveAttitude=%s", "proactiveAttitudeThreshold=%s", "giverAssets=%d", "targetAssets=%d", "selectedBonus=%s", "targetHadBonus=%d",
		"selectedTech=%s", "selectedTechCost=%d", "selectionScore=%d", "techScoreRatioX1000=%d", "giftProbX1000=%d",
		"contactProbMultX1000=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI give-help diagnostic token {required}")
	for token in expected_origins:
		if record_text.count(f"case {token}:") != 1:
			failures.append(f"{REVISION_SOURCE}: give-help origin {token} must have exactly one stringifier case")
	m = re.search(r"void\s+logSASGameRecordAIGiveHelpDecision\([^)]*\)\s*\{(?P<body>.*?)^\}", record_text, flags=re.DOTALL | re.MULTILINE)
	if m is None:
		failures.append(f"{REVISION_SOURCE}: could not locate give-help provenance logger body")
	else:
		body = m.group("body")
		if body.count("AI_getAttitudeVal(eTarget)") != 1 or "AI_getAttitude(eTarget)" in body:
			failures.append(f"{REVISION_SOURCE}: give-help logger must query raw target attitude only once and derive its category without a second lookup")
		if "int const iAttitudeValue = (bProactive ? kPlayer.AI_getAttitudeVal(eTarget) : -1);" not in body:
			failures.append(f"{REVISION_SOURCE}: give-help attitude context must stay proactive-only so relationship gifts do not log an unrelated/forced attitude value")
		if body.count("AI_getAttitudeFromValue(iAttitudeValue)") != 1:
			failures.append(f"{REVISION_SOURCE}: give-help logger must derive proactiveAttitude from the cached raw attitude value")

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	if player_text.count("logSASGameRecordAIGiveHelpDecision(") != 6:
		failures.append(f"{PLAYER_AI_SOURCE}: expected 6 realized give-help provenance bridges (human/AI for three live algorithms)")
	for token, expected in (
		("SAS_AI_GIVE_HELP_RELATION_RESOURCE", 2),
		("SAS_AI_GIVE_HELP_RELATION_TECH", 2),
		("SAS_AI_GIVE_HELP_PROACTIVE_TECH", 2),
	):
		actual = player_text.count(token)
		if actual != expected:
			failures.append(f"{PLAYER_AI_SOURCE}: expected {expected} {token} bridges, found {actual}")
	if player_text.count("logSASGameRecordAIDiploContactIntent(*this, ePlayer, CONTACT_GIVE_HELP") != 6:
		failures.append(f"{PLAYER_AI_SOURCE}: revision-100 CONTACT_GIVE_HELP package bridges changed unexpectedly")
	if player_text.count("if (gGameRecordLogLevel >= 2)") < 6:
		failures.append(f"{PLAYER_AI_SOURCE}: give-help specialized bridges must remain level-2 pre-gated")

	# Preserve the live chooser/contact RNG structure inside AI_doDiplo; revision 110 may reuse winners but must not add recorder draws.
	m = re.search(r"void\s+CvPlayerAI::AI_doDiplo\(\)\s*\{(?P<body>.*?)^\}\s*\n\n", player_text, flags=re.DOTALL | re.MULTILINE)
	if m is None:
		failures.append(f"{PLAYER_AI_SOURCE}: could not locate AI_doDiplo for give-help RNG guard")
	else:
		body = m.group("body")
		for token, expected in (("SyncRandNum(10000)", 5), ("-SyncRandNum(iCost)", 2), ("SyncRandSuccess(rGiftProb)", 1), ("AI_contactRoll(CONTACT_GIVE_HELP", 1)):
			actual = body.count(token)
			if actual != expected:
				failures.append(f"{PLAYER_AI_SOURCE}: expected {expected} AI_doDiplo occurrence(s) of {token}, found {actual}")
	return failures


def check_ai_tech_trade_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI tech-trade provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	expected_origins = [
		"SAS_AI_TECH_TRADE_RANDOM_DIRECT",
		"SAS_AI_TECH_TRADE_RANDOM_COUNTERPROPOSE",
		"SAS_AI_TECH_TRADE_PROGRESS_GOLD",
	]
	m = re.search(r"enum\s+SASGameRecordAITechTradeOrigin\s*\{(?P<body>.*?)\};", header_text, flags=re.DOTALL)
	if m is None:
		failures.append(f"{REVISION_HEADER}: missing SASGameRecordAITechTradeOrigin")
	else:
		tokens = re.findall(r"^\s*(SAS_AI_TECH_TRADE_[A-Z0-9_]+)\s*,?\s*$", m.group("body"), flags=re.MULTILINE)
		if tokens != expected_origins:
			failures.append(f"{REVISION_HEADER}: AI tech-trade origin vocabulary changed; expected {expected_origins}, found {tokens}")
	if "logSASGameRecordAITechTradeDecision" not in header_text:
		failures.append(f"{REVISION_HEADER}: missing tech-trade provenance logger declaration")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_TECH_TRADE_DECISION", "origin=%s", "contactProbMultX1000=%d", "bestKnownTechScorePercent=%d",
		"randomReceiveTech=%s", "randomReceiveScore=%d", "randomReceiveLocusSuppressed=%d", "progressTech=%s",
		"progressResearchPoints=%d", "progressLocusSuppressed=%d", "giveTech=%s", "ourReceiveTechValue=%d", "targetReceiveTechValue=%d", "giveMatchDeltaValue=%d",
		"progressReceiveTechValue=%d", "progressMaxGold=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI tech-trade diagnostic token {required}")
	for token in expected_origins:
		if record_text.count(f"case {token}:") != 1:
			failures.append(f"{REVISION_SOURCE}: AI tech-trade origin {token} must have exactly one stringifier case")
	m = re.search(r"void\s+logSASGameRecordAITechTradeDecision\([^)]*\)\s*\{(?P<body>.*?)^\}", record_text, flags=re.DOTALL | re.MULTILINE)
	if m is None:
		failures.append(f"{REVISION_SOURCE}: could not locate AI tech-trade provenance logger body")
	else:
		body = m.group("body")
		for forbidden in ("SyncRand", "AI_techTradeVal(", "AI_counterPropose(", "AI_maxGoldTrade("):
			if forbidden in body:
				failures.append(f"{REVISION_SOURCE}: tech-trade formatter must not repeat live chooser/valuation work ({forbidden})")

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	start = player_text.find("if (AI_getContactTimer(ePlayer, CONTACT_TRADE_TECH) == 0)")
	end = player_text.find("if ((!abContacted[kPlayer.getTeam()] || !bPlayerHuman) &&\n\t\t\t\tAI_contactRoll(CONTACT_TRADE_BONUS))", start)
	if start < 0 or end < 0:
		return failures + [f"{PLAYER_AI_SOURCE}: could not isolate CONTACT_TRADE_TECH block"]
	body = player_text[start:end]
	if body.count("logSASGameRecordAITechTradeDecision(") != 3:
		failures.append(f"{PLAYER_AI_SOURCE}: expected 3 realized tech-trade provenance bridges (human/AI main proposal plus AI progress purchase)")
	if body.count("logSASGameRecordAIDiploContactIntent(*this, ePlayer, CONTACT_TRADE_TECH") != 3:
		failures.append(f"{PLAYER_AI_SOURCE}: revision-100 CONTACT_TRADE_TECH package bridges changed unexpectedly")
	if body.count("SAS_AI_TECH_TRADE_RANDOM_DIRECT") != 2 or body.count("SAS_AI_TECH_TRADE_RANDOM_COUNTERPROPOSE") != 2 or body.count("SAS_AI_TECH_TRADE_PROGRESS_GOLD") != 1:
		failures.append(f"{PLAYER_AI_SOURCE}: realized tech-trade origin bridges changed unexpectedly")
	for token, expected in (
		("if (AI_contactRoll(CONTACT_TRADE_TECH, rContactProbMult))", 1),
		("int iValue = 1 + SyncRandNum(10000);", 1),
		("SyncRandNum(21)", 1),
		("SyncRandSuccess100(36)", 1),
		("kPlayer.AI_counterPropose(getID(), weGive, theyGive, true, true)", 1),
		("AI_techTradeVal(eBestProgressTech", 1),
		("AI_maxGoldTrade(kPlayer.getID())", 1),
	):
		actual = body.count(token)
		if actual != expected:
			failures.append(f"{PLAYER_AI_SOURCE}: CONTACT_TRADE_TECH live call count for {token} changed; expected {expected}, found {actual}")
	if "bool const bInitialDeal =" not in body or "bool bDeal = bInitialDeal;" not in body:
		failures.append(f"{PLAYER_AI_SOURCE}: tech-trade provenance must retain the original direct-vs-counterproposal decision without reevaluating acceptability")
	if "TechTypes const eRandomReceiveTech = eBestReceiveTech;" not in body:
		failures.append(f"{PLAYER_AI_SOURCE}: tech-trade provenance must preserve the pre-locus random receive-tech winner")
	if "TechTypes const eProgressTech = eBestProgressTech;" not in body:
		failures.append(f"{PLAYER_AI_SOURCE}: tech-trade provenance must preserve the pre-locus progress-tech winner")
	if "int const iMaxGoldTrade = AI_maxGoldTrade(kPlayer.getID());" not in body:
		failures.append(f"{PLAYER_AI_SOURCE}: progress-tech affordability context must reuse the single live max-gold query")
	return failures


def check_ai_deal_cancellation_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI deal-cancellation provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	expected_reasons = [
		"SAS_AI_DEAL_CANCEL_OFFER_REJECTED_RENEGOTIATE",
		"SAS_AI_DEAL_CANCEL_DUAL_DENIAL",
		"SAS_AI_DEAL_CANCEL_AI_RESOURCE_DENIAL",
		"SAS_AI_DEAL_CANCEL_OTHER_RESOURCE_JOKING",
		"SAS_AI_DEAL_CANCEL_GPT_LIMIT",
	]
	m = re.search(r"enum\s+SASGameRecordAIDealCancellationReason\s*\{(?P<body>.*?)\};", header_text, flags=re.DOTALL)
	if m is None:
		failures.append(f"{REVISION_HEADER}: missing SASGameRecordAIDealCancellationReason")
	else:
		tokens = re.findall(r"^\s*(SAS_AI_DEAL_CANCEL_[A-Z0-9_]+)\s*,?\s*$", m.group("body"), flags=re.MULTILINE)
		if tokens != expected_reasons:
			failures.append(f"{REVISION_HEADER}: AI deal-cancellation reason vocabulary changed; expected {expected_reasons}, found {tokens}")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_DEAL_CANCELLATION_DECISION", "reason=%s", "dealId=%d", "dealAge=%d", "vassalDeal=%d",
		"triggerSide=%s", "triggerItem=%s", "denial=%s", "dualCancelChanceX1000=%d",
		"gptOverdraftBefore=%d", "dealGpt=%d", "gptOverdraftAfter=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI deal-cancellation diagnostic token {required}")
	for token in expected_reasons:
		if record_text.count(f"case {token}:") != 1:
			failures.append(f"{REVISION_SOURCE}: AI deal-cancellation reason {token} must have exactly one stringifier case")
	m = re.search(r"void\s+logSASGameRecordAIDealCancellationDecision\([^)]*\)\s*\{(?P<body>.*?)^\}", record_text, flags=re.DOTALL | re.MULTILINE)
	if m is None:
		failures.append(f"{REVISION_SOURCE}: could not locate AI deal-cancellation logger body")
	else:
		body = m.group("body")
		for forbidden in ("AI_considerOffer(", "getTradeDenial(", "AI_bonusTrade(", "SyncRand"):
			if forbidden in body:
				failures.append(f"{REVISION_SOURCE}: deal-cancellation formatter must not repeat live decision work ({forbidden})")

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	start = player_text.find("CvPlayerAI::CancelCode CvPlayerAI::AI_checkCancel")
	end = player_text.find("// <!-- custom: AdvCiv queued AI-human cancellations", start)
	if start < 0 or end < 0:
		failures.append(f"{PLAYER_AI_SOURCE}: could not isolate AI_checkCancel")
	else:
		body = player_text[start:end]
		if body.count("logSASGameRecordAIDealCancellationDecision(") != 4:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_checkCancel must retain exactly four realized cancellation-reason bridges")
		for token in expected_reasons[:4]:
			if body.count(token) != 1:
				failures.append(f"{PLAYER_AI_SOURCE}: AI_checkCancel must use {token} exactly once")
		for token, expected in (("AI_considerOffer(", 1), ("getTradeDenial(", 1), ("SyncRandSuccess(", 1), ("AI_bonusTrade(", 2)):
			actual = body.count(token)
			if actual != expected:
				failures.append(f"{PLAYER_AI_SOURCE}: AI_checkCancel live call count for {token} changed; expected {expected}, found {actual}")
		if body.count("if (gGameRecordLogLevel >= 2) logSASGameRecordAIDealCancellationDecision") != 4:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_checkCancel cancellation provenance must remain success-only and level-2 gated")

	start = player_text.find("// Enforce GPT limit")
	end = player_text.find("// One diplo popup for all canceled non-vassal deals", start)
	if start < 0 or end < 0:
		failures.append(f"{PLAYER_AI_SOURCE}: could not isolate AI_doDeals GPT-limit pass")
	else:
		body = player_text[start:end]
		if body.count("SAS_AI_DEAL_CANCEL_GPT_LIMIT") != 1 or body.count("logSASGameRecordAIDealCancellationDecision(") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: GPT-limit cancellation must retain exactly one realized provenance bridge")
		if body.count("AI_maxGoldPerTurnTrade(eOther, true)") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: GPT-limit provenance must reuse the single live AI_maxGoldPerTurnTrade query")
		if "iOverdraft - aiiDealsByGPT[i].first" not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: GPT-limit provenance must derive post-cancel overdraft from the already-selected deal amount")
	return failures


def check_deal_invalidation_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, "CvGameCoreDLL/CvDeal.cpp", "CvGameCoreDLL/CvDeal.h"):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing deal-invalidation provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	expected_reasons = [
		"SAS_DEAL_INVALID_RESOURCE_SUPPLY_DEFICIT",
		"SAS_DEAL_INVALID_TRADE_NETWORK_LOST",
		"SAS_DEAL_INVALID_GIVER_BONUS_OBSOLETE",
		"SAS_DEAL_INVALID_RECIPIENT_BONUS_OBSOLETE",
		"SAS_DEAL_INVALID_GPT_INSOLVENT",
		"SAS_DEAL_INVALID_PEACE_TREATY_EXPIRED",
	]
	m = re.search(r"enum\s+SASGameRecordDealInvalidationReason\s*\{(?P<body>.*?)\};", header_text, flags=re.DOTALL)
	if m is None:
		failures.append(f"{REVISION_HEADER}: missing SASGameRecordDealInvalidationReason")
	else:
		tokens = re.findall(r"^\s*(SAS_DEAL_INVALID_[A-Z0-9_]+)\s*,?\s*$", m.group("body"), flags=re.MULTILINE)
		if tokens != expected_reasons:
			failures.append(f"{REVISION_HEADER}: deal-invalidation reason vocabulary changed; expected {expected_reasons}, found {tokens}")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_DEAL_INVALIDATION", "dealId=%d", "dealAge=%d", "reason=%s", "giver=%d", "recipient=%d",
		"triggerItem=%s", "tradeableBonusCount=%d", "goldPercent=%d", "gold=%d", "gptDue=%d", "goldRate=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing deal-invalidation diagnostic token {required}")
	for token in expected_reasons:
		if record_text.count(f"case {token}:") != 1:
			failures.append(f"{REVISION_SOURCE}: deal-invalidation reason {token} must have exactly one stringifier case")
	m = re.search(r"void\s+logSASGameRecordDealInvalidation\([^)]*\)\s*\{(?P<body>.*?)^\}", record_text, flags=re.DOTALL | re.MULTILINE)
	if m is None:
		failures.append(f"{REVISION_SOURCE}: could not locate deal-invalidation logger body")
	else:
		body = m.group("body")
		for forbidden in ("getNumTradeableBonuses(", "canTradeNetworkWith(", "isBonusObsolete(", "calculateGoldRate(", "isCancelable(", "isPeaceDeal(", "SyncRand"):
			if forbidden in body:
				failures.append(f"{REVISION_SOURCE}: deal-invalidation formatter must not repeat live validity work ({forbidden})")

	deal_text = (repo_root / "CvGameCoreDLL/CvDeal.cpp").read_text(encoding="utf-8", errors="replace")
	start = deal_text.find("void CvDeal::verify()")
	end = deal_text.find("// <!-- custom: AdvCiv's simplified traversal", start)
	if start < 0 or end < 0:
		failures.append("CvGameCoreDLL/CvDeal.cpp: could not isolate CvDeal::verify overloads")
	else:
		body = deal_text[start:end]
		for token in expected_reasons:
			if body.count(token) != 1:
				failures.append(f"CvGameCoreDLL/CvDeal.cpp: verify path must use {token} exactly once")
		for token, expected in (("getNumTradeableBonuses(", 1), ("canTradeNetworkWith(", 1), ("isBonusObsolete(", 2), ("getCommercePercent(COMMERCE_GOLD)", 1), ("getGold()", 1), ("isAnarchy()", 1), ("calculateGoldRate()", 1), ("isCancelable(NO_PLAYER)", 1), ("isPeaceDeal()", 1), ("logSASGameRecordDealInvalidation(", 1), ("kill();", 1)):
			actual = body.count(token)
			if actual != expected:
				failures.append(f"CvGameCoreDLL/CvDeal.cpp: verify live call count for {token} changed; expected {expected}, found {actual}")
		if "SASGameRecordDealInvalidationContext* pSASContext = (gGameRecordLogLevel >= 2 ? &kSASContext : NULL);" not in body:
			failures.append("CvGameCoreDLL/CvDeal.cpp: deal-invalidation capture must remain level-2 pre-gated")
	return failures


def check_ai_joint_war_request_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI joint-war request provenance file: {relative_path}")
	if failures:
		return failures

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_JOINT_WAR_REQUEST", "human=%d", "humanTeam=%d", "attitudeValue=%d",
		"meanAtWarTurnsX1000=%d", "contactRand=%d", "targetTeam=%d", "targetScore=%d",
		"targetRandomScore=%d", "targetAtWarCounter=%d", "humanPeaceCounter=%d", "uwai=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI joint-war request diagnostic token {required}")

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	m = re.search(r"bool\s+CvPlayerAI::AI_proposeJointWar\([^)]*\)\s*\{(?P<body>.*?)^\}\s*\n\n/\*\s*Caller ensures that both players", player_text, flags=re.DOTALL | re.MULTILINE)
	if m is None:
		return failures + [f"{PLAYER_AI_SOURCE}: could not locate AI_proposeJointWar"]
	body = m.group("body")
	if body.count("logSASGameRecordAIJointWarRequest(") != 1:
		failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeJointWar must emit exactly one realized joint-war request bridge")
	if body.count("logSASGameRecordAIDiploContactIntent(") != 1:
		failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeJointWar must retain exactly one generic CONTACT_JOIN_WAR subject bridge")
	if not re.search(r"if\s*\(gGameRecordLogLevel\s*>=\s*2\)\s*\{[^{}]*logSASGameRecordAIJointWarRequest\([^;]+;[^{}]*logSASGameRecordAIDiploContactIntent", body, flags=re.DOTALL):
		failures.append(f"{PLAYER_AI_SOURCE}: joint-war request provenance must remain level-2 pre-gated and precede the generic contact row")
	for token, expected in (("AI_contactRoll(CONTACT_JOIN_WAR", 1), ("SyncRandSuccess(", 1), ("SyncRandNum(10000)", 1), ("declareWarTrade(", 1), ("AI_proposeEmbargo(", 2), ("gDLL->beginDiplomacy(", 1)):
		actual = body.count(token)
		if actual != expected:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeJointWar live call count for {token} changed; expected {expected}, found {actual}")
	if "iTargetVal += std::min(20, kOurTeam.AI_getAtWarCounter(eTarget)) * 1000" not in body:
		failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeJointWar must retain its war-duration-weighted target score")
	if "(rMeanAtWarTurns * 1000).round()" not in body:
		failures.append(f"{PLAYER_AI_SOURCE}: joint-war request bridge must preserve the live shared-war-adjusted mean war age without recomputing it")
	log_pos = body.find("logSASGameRecordAIJointWarRequest(")
	diplo_pos = body.find("gDLL->beginDiplomacy(")
	if log_pos >= 0 and diplo_pos >= 0 and log_pos > diplo_pos:
		failures.append(f"{PLAYER_AI_SOURCE}: joint-war request provenance must emit before beginDiplomacy")
	return failures


def check_ai_embargo_request_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI embargo request provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	expected_origins = [
		"SAS_AI_EMBARGO_DIRECT_CONTACT",
		"SAS_AI_EMBARGO_JOINT_WAR_ATTITUDE_REDIRECT",
		"SAS_AI_EMBARGO_JOINT_WAR_UWAI_REDIRECT",
	]
	m = re.search(r"enum\s+SASGameRecordAIEmbargoRequestOrigin\s*\{(?P<body>.*?)\};", header_text, flags=re.DOTALL)
	if m is None:
		failures.append(f"{REVISION_HEADER}: missing SASGameRecordAIEmbargoRequestOrigin")
	else:
		tokens = re.findall(r"^\s*(SAS_AI_EMBARGO_[A-Z0-9_]+)\s*,?\s*$", m.group("body"), flags=re.MULTILINE)
		if tokens != expected_origins:
			failures.append(f"{REVISION_HEADER}: AI embargo origin vocabulary changed; expected {expected_origins}, found {tokens}")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_EMBARGO_REQUEST", "origin=%s", "embargoTargetTeam=%d", "teamAttitudeHuman=%d",
		"teamAttitudeTarget=%d", "attitudeGap=%d", "jointWarTargetTeam=%d", "jointWarTargetScore=%d",
		"jointWarTargetRandomScore=%d", "jointWarTargetAtWarCounter=%d", "jointWarTargetIsEmbargoTarget=%d",
		"jointWarDenial=%s", "getSASGameRecordDenialType",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI embargo request diagnostic token {required}")
	for token in expected_origins:
		if record_text.count(f"case {token}:") != 1:
			failures.append(f"{REVISION_SOURCE}: AI embargo origin {token} must have exactly one stringifier case")

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	m = re.search(r"bool\s+CvPlayerAI::AI_proposeJointWar\([^)]*\)\s*\{(?P<body>.*?)^\}\s*\n\n/\*\s*Caller ensures that both players", player_text, flags=re.DOTALL | re.MULTILINE)
	if m is None:
		failures.append(f"{PLAYER_AI_SOURCE}: could not locate AI_proposeJointWar for embargo provenance")
	else:
		body = m.group("body")
		if body.count("logSASGameRecordAIEmbargoRequest(") != 2:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeJointWar must retain exactly two realized embargo-redirect bridges")
		for token in expected_origins[1:]:
			if body.count(token) != 1:
				failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeJointWar must use {token} exactly once")
		if body.count("AI_proposeEmbargo(eHuman)") != 2:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeJointWar live embargo fallback count changed")
		if body.count("declareWarTrade(eBestTarget, getTeam())") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeJointWar must evaluate UWAI joint-war denial exactly once")
		if "DenialTypes const eJointWarDenial" not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: UWAI embargo redirect must retain the already-computed denial instead of reevaluating it for logging")

	if player_text.count("SAS_AI_EMBARGO_DIRECT_CONTACT") != 1:
		failures.append(f"{PLAYER_AI_SOURCE}: ordinary STOP_TRADING path must bridge the direct embargo origin exactly once")
	if not re.search(r"abContacted\[kPlayer\.getTeam\(\)\]\s*=\s*AI_proposeEmbargo\(ePlayer\);\s*if\s*\(gGameRecordLogLevel\s*>=\s*2\s*&&\s*abContacted\[kPlayer\.getTeam\(\)\]\)\s*logSASGameRecordAIEmbargoRequest\([^;]+SAS_AI_EMBARGO_DIRECT_CONTACT", player_text, flags=re.DOTALL):
		failures.append(f"{PLAYER_AI_SOURCE}: direct embargo provenance must remain success-only and level-2 pre-gated")

	m = re.search(r"bool\s+CvPlayerAI::AI_proposeEmbargo\([^)]*\)\s*\{(?P<body>.*?)^\}\s*\n\n// <!-- custom: Recorder-only resource chooser", player_text, flags=re.DOTALL | re.MULTILINE)
	if m is None:
		failures.append(f"{PLAYER_AI_SOURCE}: could not locate AI_proposeEmbargo")
	else:
		body = m.group("body")
		if body.count("logSASGameRecordAIDiploContactIntent(") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeEmbargo must retain exactly one generic CONTACT_STOP_TRADING subject bridge")
		if "logSASGameRecordAIEmbargoRequest(" in body:
			failures.append(f"{PLAYER_AI_SOURCE}: embargo origin must remain caller-owned so AI_proposeEmbargo's public gameplay signature stays unchanged")
		if re.search(r"SyncRand|getSorenRand|MapRand", body):
			failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeEmbargo must remain RNG-free")
	return failures


def check_ai_vassalage_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE, PLAYER_AI_HEADER, UWAI_AGENT_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI vassalage provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	expected_origins = [
		"SAS_AI_VASSALAGE_VOLUNTARY_CONTACT", "SAS_AI_VASSALAGE_UWAI_FIND_MASTER",
		"SAS_AI_VASSALAGE_LEGACY_CAPITULATION", "SAS_AI_VASSALAGE_UWAI_CAPITULATION",
	]
	m = re.search(r"enum\s+SASGameRecordAIVassalageOrigin\s*\{(?P<body>.*?)\};", header_text, flags=re.DOTALL)
	if m is None:
		failures.append(f"{REVISION_HEADER}: missing SASGameRecordAIVassalageOrigin")
	else:
		tokens = re.findall(r"^\s*(SAS_AI_VASSALAGE_[A-Z0-9_]+)\s*,?\s*$", m.group("body"), flags=re.MULTILINE)
		if tokens != expected_origins:
			failures.append(f"{REVISION_HEADER}: AI vassalage origin vocabulary changed; expected {expected_origins}, found {tokens}")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_VASSALAGE_DECISION", "origin=%s", "enemyTeam=%d", "enemyAtWarCounter=%d",
		"contactProbMultX1000=%d", "targetRank=%d", "aliveCivs=%d", "ourWars=%d", "counterProposal=%d",
		"aiGives=%s", "aiReceives=%s",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI vassalage diagnostic token {required}")
	for token in expected_origins:
		if record_text.count(f"case {token}:") != 1:
			failures.append(f"{REVISION_SOURCE}: AI vassalage origin {token} must have exactly one stringifier case")

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	player_header = (repo_root / PLAYER_AI_HEADER).read_text(encoding="utf-8", errors="replace")
	uwai_text = (repo_root / UWAI_AGENT_SOURCE).read_text(encoding="utf-8", errors="replace")
	if "void AI_offerCapitulation(PlayerTypes eTo, bool bUWAI);" not in player_header:
		failures.append(f"{PLAYER_AI_HEADER}: AI_offerCapitulation must retain explicit caller-family provenance")
	m = re.search(r"void\s+CvPlayerAI::AI_offerCapitulation\([^)]*\)\s*\{(?P<body>.*?)^\}\s*// </advc\.104h>", player_text, flags=re.DOTALL | re.MULTILINE)
	if m is None:
		failures.append(f"{PLAYER_AI_SOURCE}: could not locate AI_offerCapitulation")
	else:
		body = m.group("body")
		if body.count("logSASGameRecordAIVassalageDecision(") != 2:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_offerCapitulation must bridge AI and human realized surrender delivery exactly once each")
		if body.count("AI_counterPropose(") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_offerCapitulation counterproposal call count changed")
		if body.count("implementDeal(") != 1 or body.count("beginDiplomacy(") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_offerCapitulation realized AI/human delivery structure changed")
		if "bUWAI ? SAS_AI_VASSALAGE_UWAI_CAPITULATION : SAS_AI_VASSALAGE_LEGACY_CAPITULATION" not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: capitulation row must preserve explicit legacy/UWAI origin")
	if player_text.count("AI_offerCapitulation(eTarget, false)") != 1:
		failures.append(f"{PLAYER_AI_SOURCE}: legacy AI_doPeace must identify its capitulation caller exactly once")
	if player_text.count("SAS_AI_VASSALAGE_VOLUNTARY_CONTACT") != 2:
		failures.append(f"{PLAYER_AI_SOURCE}: voluntary-vassal human/AI delivery paths must each retain one provenance bridge")
	if player_text.count("int const iTargetRank = kGame.getPlayerRank(ePlayer);") != 1 or player_text.count("int const iAliveCivs = kGame.countCivPlayersAlive();") != 1:
		failures.append(f"{PLAYER_AI_SOURCE}: voluntary-vassal contact multiplier inputs must remain cached from the live formula")
	if uwai_text.count("AI_offerCapitulation(kTargetPlayer.getID(), true)") != 1:
		failures.append(f"{UWAI_AGENT_SOURCE}: UWAI capitulation must identify its caller exactly once")
	if uwai_text.count("SAS_AI_VASSALAGE_UWAI_FIND_MASTER") != 1:
		failures.append(f"{UWAI_AGENT_SOURCE}: tryFindingMaster must emit exactly one realized protector-selection bridge")
	if re.search(r"logSASGameRecordAIVassalageDecision\([^;]+SAS_AI_VASSALAGE_UWAI_FIND_MASTER", uwai_text, flags=re.DOTALL) is None:
		failures.append(f"{UWAI_AGENT_SOURCE}: missing UWAI protector-selection vassalage bridge")
	return failures


def check_ai_colony_split_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE, UWAI_AGENT_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI colony-split provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	if "logSASGameRecordAIColonySplitDecision(" not in header_text:
		failures.append(f"{REVISION_HEADER}: missing AI colony-split provenance declaration")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_COLONY_SPLIT_DECISION", "origin=%s", "areaId=%d", "areaValue=%d",
		"areaCities=%d", "areaPopulation=%d", "playerCitiesBefore=%d", "playerPopulationBefore=%d",
		'"UWAI_FORCED_PRE_CAPITULATION"', '"NORMAL_POSITIVE_VALUE"',
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI colony-split diagnostic token {required}")
	m_record = re.search(r"void logSASGameRecordAIColonySplitDecision\([^\)]*\)\s*\{(?P<body>.*?)^\}", record_text, flags=re.DOTALL | re.MULTILINE)
	if m_record is None:
		failures.append(f"{REVISION_SOURCE}: could not locate AI colony-split provenance definition")
	elif "gGameRecordLogLevel" in m_record.group("body"):
		failures.append(f"{REVISION_SOURCE}: AI colony-split serializer must rely on caller-side level-2 pre-gating")

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	m_split = re.search(r"void CvPlayerAI::AI_doSplit\([^\)]*\)(?P<body>.*?)^\}", player_text, flags=re.DOTALL | re.MULTILINE)
	if m_split is None:
		failures.append(f"{PLAYER_AI_SOURCE}: could not locate AI_doSplit")
	else:
		body = m_split.group("body")
		if body.count("logSASGameRecordAIColonySplitDecision(") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_doSplit must retain exactly one realized colony-split provenance bridge")
		if body.count("if (gGameRecordLogLevel >= 2)") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: colony-split provenance must remain level-2 pre-gated only at the committed split boundary")
		if body.count("AI_splitEmpireValue()") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_doSplit must retain exactly one live AI_splitEmpireValue evaluation site")
		if "logSASGameRecordAIColonySplitDecision(*this, kArea, it->second, bForce);" not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: colony-split provenance must reuse the selected area's already-computed aggregate value and force flag")
		if "if (it->second <= 0 /* advc.104r: */ && !bForce)" not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: ordinary colony splitting must retain its positive-value gate")
		if body.find("logSASGameRecordAIColonySplitDecision(") > body.find("splitEmpire(kArea);"):
			failures.append(f"{PLAYER_AI_SOURCE}: colony-split decision row must be emitted before splitEmpire destroys pre-transfer context")

	uwai_text = (repo_root / UWAI_AGENT_SOURCE).read_text(encoding="utf-8", errors="replace")
	if uwai_text.count("AI_doSplit(true);") != 1:
		failures.append(f"{UWAI_AGENT_SOURCE}: expected exactly one forced AI_doSplit caller for the UWAI pre-capitulation origin")
	return failures


def check_ai_spaceship_launch_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI spaceship-launch provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	if "logSASGameRecordAISpaceshipLaunchDecision(" not in header_text:
		failures.append(f"{REVISION_HEADER}: missing AI spaceship-launch provenance declaration")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_SPACESHIP_LAUNCH_DECISION", "victory=%s", "reason=%s", "launchSuccessPercent=%d",
		"nearestRivalTeam=%d", "nearestRivalCountdown=%d", "rivalAtOrBeforeOurArrival=%d",
		'"FULL_SUCCESS"', '"RIVAL_DEADLINE"',
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI spaceship-launch diagnostic token {required}")
	m_record = re.search(r"void logSASGameRecordAISpaceshipLaunchDecision\([^\)]*\)\s*\{(?P<body>.*?)^\}", record_text, flags=re.DOTALL | re.MULTILINE)
	if m_record is None:
		failures.append(f"{REVISION_SOURCE}: could not locate AI spaceship-launch provenance definition")
	else:
		body = m_record.group("body")
		if "gGameRecordLogLevel" in body:
			failures.append(f"{REVISION_SOURCE}: spaceship-launch serializer must rely on caller-side level-2 pre-gating")
		if "TeamIter<" in body or "getLaunchSuccessRate(" in body or "getVictoryDelay(" in body:
			failures.append(f"{REVISION_SOURCE}: spaceship-launch serializer must reuse caller-supplied live gate values rather than rescanning rivals/project state")
		if 'iLaunchSuccessPercent >= 100 ? "FULL_SUCCESS" : "RIVAL_DEADLINE"' not in body:
			failures.append(f"{REVISION_SOURCE}: spaceship-launch reason must remain classified from the live success percentage")

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	m_launch = re.search(r"void CvPlayerAI::AI_launch\([^\)]*\)(?P<body>.*?)^\}", player_text, flags=re.DOTALL | re.MULTILINE)
	if m_launch is None:
		failures.append(f"{PLAYER_AI_SOURCE}: could not locate AI_launch")
	else:
		body = m_launch.group("body")
		if body.count("logSASGameRecordAISpaceshipLaunchDecision(") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_launch must retain exactly one realized spaceship-launch provenance bridge")
		if body.count("if (gGameRecordLogLevel >= 2)") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: spaceship-launch provenance must remain caller-pre-gated only at the realized launch boundary")
		if body.count("TeamIter<CIV_ALIVE,NOT_SAME_TEAM_AS>") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_launch nearest-rival scan count changed")
		if body.count("getVictoryCountdown(eVictory)") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_launch must retain exactly one live rival countdown read site")
		if body.count("getVictoryDelay(eVictory)") != 2:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_launch must retain one live victory-delay gate plus the inherited commented-out reference")
		if body.count("getLaunchSuccessRate(eVictory)") != 3:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_launch must retain the inherited commented-out success reference plus native and level-2 urgent-launch reads")
		if "if (iLaunchSuccessPercent < 0)" not in body or "if (gGameRecordLogLevel >= 2)" not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: urgent-rival launch success must remain collected only inside the realized level-2 recorder gate")
		if "bUrgentRival = (iBestArrival <= iVictoryDelay);" not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_launch must retain the native rival-deadline comparison explicitly")
		if body.count("launch(eVictory);") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_launch realized launch call count changed")
		if "logSASGameRecordAISpaceshipLaunchDecision(*this, eVictory, iLaunchSuccessPercent, eBestTeam, eBestTeam == NO_TEAM ? -1 : iBestArrival, bUrgentRival);" not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: spaceship-launch provenance must reuse the already-selected success/rival/deadline context and normalize no-rival to -1")
		log_pos = body.find("logSASGameRecordAISpaceshipLaunchDecision(")
		launch_pos = body.find("launch(eVictory);")
		if log_pos < 0 or launch_pos < 0 or log_pos > launch_pos:
			failures.append(f"{PLAYER_AI_SOURCE}: spaceship-launch decision row must emit before launch mutates victory state")
		if re.search(r"SyncRand|getSorenRand|MapRand", body):
			failures.append(f"{PLAYER_AI_SOURCE}: AI_launch should remain RNG-free")
	return failures


def check_ai_religion_spread_target_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, "CvGameCoreDLL/CvUnitAI.cpp"):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI religion-spread target provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	if "logSASGameRecordAIReligionSpreadTarget(" not in header_text:
		failures.append(f"{REVISION_HEADER}: missing AI religion-spread target provenance declaration")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_RELIGION_SPREAD_TARGET", "religion=%s", "targetScope=%s", "targetPlayer=%d",
		"playerMultiplierPercent=%d", "pathTurns=%d", "targetScore=%d", "action=%s",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI religion-spread target diagnostic token {required}")
	m_record = re.search(r"void logSASGameRecordAIReligionSpreadTarget\([^\)]*\)\s*\{(?P<body>.*?)^\}", record_text, flags=re.DOTALL | re.MULTILINE)
	if m_record is None:
		failures.append(f"{REVISION_SOURCE}: could not locate AI religion-spread target provenance definition")
	elif "gGameRecordLogLevel" in m_record.group("body"):
		failures.append(f"{REVISION_SOURCE}: religion-spread target serializer must rely on caller-side level-2 pre-gating")

	unit_text = (repo_root / "CvGameCoreDLL/CvUnitAI.cpp").read_text(encoding="utf-8", errors="replace")
	m_spread = re.search(r"bool CvUnitAI::AI_spreadReligion\(\)(?P<body>.*?)^\}", unit_text, flags=re.DOTALL | re.MULTILINE)
	if m_spread is None:
		failures.append("CvGameCoreDLL/CvUnitAI.cpp: could not locate AI_spreadReligion")
	else:
		body = m_spread.group("body")
		if body.count("generatePath(") != 1:
			failures.append("CvGameCoreDLL/CvUnitAI.cpp: AI_spreadReligion path-search call count changed")
		if body.count("logSASGameRecordAIReligionSpreadTarget(") != 1:
			failures.append("CvGameCoreDLL/CvUnitAI.cpp: ordinary Missionary target selection must retain exactly one provenance bridge")
		if body.count("AI_getMissionAIType()") != 1 or body.count("AI_getMissionAIPlot()") != 1:
			failures.append("CvGameCoreDLL/CvUnitAI.cpp: Missionary continuing-target suppression must inspect existing MissionAI state exactly once each")
		for required in (
			"if (bCaptureSASReligionSpreadTarget)", "iSASBestPlayerMultiplierPercent = iPlayerMultiplierPercent;",
			"iSASBestPathTurns = iPathTurns;", "iSASBestTargetScore = iValue;",
			"pGroup->AI_getMissionAIType() != MISSIONAI_SPREAD", "pGroup->AI_getMissionAIPlot() != pBestSpreadPlot",
		):
			if required not in body:
				failures.append(f"CvGameCoreDLL/CvUnitAI.cpp: missing Missionary target provenance invariant {required}")
		if "SyncRand" in body:
			failures.append("CvGameCoreDLL/CvUnitAI.cpp: ordinary Missionary land targeting should remain synchronized-RNG-free")

	m_airlift = re.search(r"bool CvUnitAI::AI_spreadReligionAirlift\(\)(?P<body>.*?)^\}", unit_text, flags=re.DOTALL | re.MULTILINE)
	if m_airlift is None:
		failures.append("CvGameCoreDLL/CvUnitAI.cpp: could not locate AI_spreadReligionAirlift")
	else:
		body = m_airlift.group("body")
		if body.count("logSASGameRecordAIReligionSpreadTarget(") != 1 or '"AIRLIFT"' not in body:
			failures.append("CvGameCoreDLL/CvUnitAI.cpp: Missionary airlift must retain one realized AIRLIFT provenance bridge")
		if body.count("if (gGameRecordLogLevel >= 2)") != 1:
			failures.append("CvGameCoreDLL/CvUnitAI.cpp: Missionary airlift provenance must remain caller-pre-gated at level 2")
		if "SyncRand" in body:
			failures.append("CvGameCoreDLL/CvUnitAI.cpp: Missionary airlift target selection should remain synchronized-RNG-free")
	return failures


def check_ai_map_trade_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI map-trade provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	if "logSASGameRecordAIMapTradeDecision(" not in header_text:
		failures.append(f"{REVISION_HEADER}: missing AI map-trade provenance declaration")
	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	m_record = re.search(r"void logSASGameRecordAIMapTradeDecision\([^\)]*\)\s*\{(?P<body>.*?)^\}", record_text, flags=re.DOTALL | re.MULTILINE)
	if m_record is None:
		failures.append(f"{REVISION_SOURCE}: could not locate AI map-trade provenance definition")
	else:
		record_body = m_record.group("body")
		if "gGameRecordLogLevel" in record_body:
			failures.append(f"{REVISION_SOURCE}: map-trade serializer must rely on the caller's level-2 pre-gate")
		if "if (eTarget == NO_PLAYER)" not in record_body:
			failures.append(f"{REVISION_SOURCE}: map-trade serializer must retain the cheap NO_PLAYER safety before GET_PLAYER")
	for required in (
		"GAME_RECORD_AI_MAP_TRADE_DECISION", "ourReceiveMapValue=%d", "targetReceiveMapValue=%d",
		"ourValueAdvantage=%d", "targetReceiveMinExclusive=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI map-trade diagnostic token {required}")

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	m = re.search(r"if \(AI_getContactTimer\(ePlayer, CONTACT_TRADE_MAP\) == 0\)(?P<body>.*?)^\t\t\t\}", player_text, flags=re.DOTALL | re.MULTILINE)
	if m is None:
		failures.append(f"{PLAYER_AI_SOURCE}: could not locate CONTACT_TRADE_MAP block")
	else:
		body = m.group("body")
		if body.count("AI_mapTradeVal(") != 2:
			failures.append(f"{PLAYER_AI_SOURCE}: proactive map trade must retain exactly two live AI_mapTradeVal evaluations")
		if body.count("logSASGameRecordAIMapTradeDecision(") != 2:
			failures.append(f"{PLAYER_AI_SOURCE}: realized human/AI map-trade delivery paths must each retain one specialized provenance bridge")
		if body.count("if (gGameRecordLogLevel >= 2)") != 2:
			failures.append(f"{PLAYER_AI_SOURCE}: both realized map-trade delivery paths must pre-gate specialized/generic recording at level 2")
		if "if (iOurReceiveValue >= iTargetReceiveValue)" not in body or "if (iTargetReceiveValue > iTargetReceiveMinExclusive)" not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: proactive map-trade asymmetric value/threshold gate changed")
		if body.count("GC.getDefineINT(CvGlobals::DIPLOMACY_VALUE_REMAINDER)") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: proactive map-trade threshold lookup count changed")
		else:
			iValueGate = body.find("if (iOurReceiveValue >= iTargetReceiveValue)")
			iThresholdLookup = body.find("GC.getDefineINT(CvGlobals::DIPLOMACY_VALUE_REMAINDER)")
			iThresholdGate = body.find("if (iTargetReceiveValue > iTargetReceiveMinExclusive)")
			if not (0 <= iValueGate < iThresholdLookup < iThresholdGate):
				failures.append(f"{PLAYER_AI_SOURCE}: proactive map-trade threshold must remain short-circuited behind the proposer-value gate")
		if body.count("AI_contactRoll(CONTACT_TRADE_MAP)") != 1:
			failures.append(f"{PLAYER_AI_SOURCE}: proactive map-trade contact RNG structure changed")
	return failures


def check_unit_completion_sources(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, CITY_SOURCE, PLAYER_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing unit-completion provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	for token in ("SASGameRecordUnitCompletionSource", "SAS_UNIT_COMPLETION_PRODUCTION", "SAS_UNIT_COMPLETION_CONSCRIPT", "SAS_UNIT_COMPLETION_COLONY_FREE_DEFENDER"):
		if token not in header_text:
			failures.append(f"{REVISION_HEADER}: missing explicit unit-completion source token {token}")
	if "bool bConscripted" in header_text:
		failures.append(f"{REVISION_HEADER}: unit-completion recorder must not collapse completion provenance back to a conscripted boolean")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for token in (
		'"PRODUCTION"', '"CONSCRIPT"', '"COLONY_FREE_DEFENDER"',
		"colonyFreeDefenders=%d", "colonyFreeDefenderProductionNeeded=%d", "colonyFreeDefenderUnitTypes=%s",
	):
		if token not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing explicit unit-completion/colony-flow token {token}")
	m_record = re.search(r"void logSASGameRecordUnitCompleted\([^\)]*\)\s*\{(?P<body>.*?)^\}", record_text, flags=re.DOTALL | re.MULTILINE)
	if m_record is None:
		failures.append(f"{REVISION_SOURCE}: could not locate unit-completion recorder definition")
	else:
		body = m_record.group("body")
		for token in ("SAS_UNIT_COMPLETION_PRODUCTION", "SAS_UNIT_COMPLETION_CONSCRIPT", "SAS_UNIT_COMPLETION_COLONY_FREE_DEFENDER"):
			if body.count(token) != 1:
				failures.append(f"{REVISION_SOURCE}: unit-completion source {token} must have exactly one accounting branch")

	city_text = (repo_root / CITY_SOURCE).read_text(encoding="utf-8", errors="replace")
	m_city = re.search(r"CvUnit\* CvCity::initConscriptedUnit\(bool bColonyFreeDefender\)(?P<body>.*?)^\}", city_text, flags=re.DOTALL | re.MULTILINE)
	if m_city is None:
		failures.append(f"{CITY_SOURCE}: initConscriptedUnit must retain explicit colony-source context")
	else:
		body = m_city.group("body")
		if "bColonyFreeDefender ? SAS_UNIT_COMPLETION_COLONY_FREE_DEFENDER : SAS_UNIT_COMPLETION_CONSCRIPT" not in body:
			failures.append(f"{CITY_SOURCE}: shared conscript-unit initializer must distinguish true drafts from colony bootstrap defenders")
	if city_text.count("SAS_UNIT_COMPLETION_PRODUCTION") != 1:
		failures.append(f"{CITY_SOURCE}: ordinary city production must retain exactly one explicit PRODUCTION completion source")

	player_text = (repo_root / PLAYER_SOURCE).read_text(encoding="utf-8", errors="replace")
	if player_text.count("initConscriptedUnit(/* bColonyFreeDefender=*/true)") != 1:
		failures.append(f"{PLAYER_SOURCE}: split-empire free defenders must pass explicit colony bootstrap provenance exactly once")
	return failures


def check_ai_draft_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, "CvGameCoreDLL/CvCityAI.cpp"):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI draft provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	for token in ("SASGameRecordAIDraftReason", "logSASGameRecordAIDraftDecision("):
		if token not in header_text:
			failures.append(f"{REVISION_HEADER}: missing AI draft provenance declaration token {token}")
	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_DRAFT_DECISION", "reason=%s", "unit=%s", "population=%d", "highestPopulation=%d",
		"conscriptPopulation=%d", "conscriptAngerTimer=%d", "danger=%d", "landWar=%d", "goodValue=%d",
		"tooMuchPop=%d", "poorPlots=%d", "happyDiff=%d", "unitCostPerMil=%d", "localDefense=%d",
		"localEnemyOffense=%d", "buildUnitProb=%d", '"FORCED_CALLER"', '"TURTLE_STRATEGY"',
		'"LOCAL_DANGER"', '"NONCRITICAL_RANDOM_VALUE"',
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI draft diagnostic token {required}")
	m_record = re.search(r"void logSASGameRecordAIDraftDecision\([^\)]*\)\s*\{(?P<body>.*?)^\}", record_text, flags=re.DOTALL | re.MULTILINE)
	if m_record is None:
		failures.append(f"{REVISION_SOURCE}: could not locate AI draft provenance definition")
	elif "gGameRecordLogLevel" in m_record.group("body"):
		failures.append(f"{REVISION_SOURCE}: AI draft serializer must rely on realized caller-side level-2 pre-gating")

	city_text = (repo_root / "CvGameCoreDLL/CvCityAI.cpp").read_text(encoding="utf-8", errors="replace")
	m = re.search(r"void CvCityAI::AI_doDraft\(bool bForce\)(?P<body>.*?)^\}", city_text, flags=re.DOTALL | re.MULTILINE)
	if m is None:
		return failures + ["CvGameCoreDLL/CvCityAI.cpp: could not locate AI_doDraft"]
	body = m.group("body")
	if body.count("conscript();") != 2:
		failures.append("CvGameCoreDLL/CvCityAI.cpp: AI_doDraft realized conscript exit count changed")
	if body.count("logSASGameRecordAIDraftDecision(") != 2:
		failures.append("CvGameCoreDLL/CvCityAI.cpp: forced and ordinary realized AI_doDraft exits must each retain one provenance bridge")
	if body.count("if (gGameRecordLogLevel >= 2)") != 2:
		failures.append("CvGameCoreDLL/CvCityAI.cpp: AI draft provenance must remain level-2 pre-gated only at realized conscript exits")
	if "int const iPoorPlots = AI_countWorkedPoorPlots();" not in body:
		failures.append("CvGameCoreDLL/CvCityAI.cpp: AI_doDraft must cache the live poor-plot count once for gameplay/BBAI/recorder reuse")
	for token, expected in (("AI_localDefenceStrength(", 1), ("AI_localAttackStrength(", 1), ("AI_buildUnitProb(true)", 1), ("SyncRandSuccess100(", 1)):
		actual = body.count(token)
		if actual != expected:
			failures.append(f"CvGameCoreDLL/CvCityAI.cpp: AI_doDraft live call count for {token} changed; expected {expected}, found {actual}")
	for token in ("SAS_AI_DRAFT_TURTLE_STRATEGY", "SAS_AI_DRAFT_LOCAL_DANGER", "SAS_AI_DRAFT_NONCRITICAL_RANDOM_VALUE"):
		if body.count(token) != 1:
			failures.append(f"CvGameCoreDLL/CvCityAI.cpp: realized AI draft reason {token} must be assigned exactly once")
	if "SyncRandSuccess100(iSASBuildUnitProb)" not in body:
		failures.append("CvGameCoreDLL/CvCityAI.cpp: non-critical draft RNG must reuse the cached live AI_buildUnitProb value")
	return failures


def check_ai_hurry_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, "CvGameCoreDLL/CvCityAI.cpp"):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI hurry provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	for token in ("SASGameRecordAIHurryReason", "logSASGameRecordAIHurryDecision("):
		if token not in header_text:
			failures.append(f"{REVISION_HEADER}: missing AI hurry provenance declaration token {token}")
	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_HURRY_DECISION", "reason=%s", "hurry=%s", "targetKind=%s", "target=%s",
		"unitAI=%s", "happyBalance=%d", "happyDiff=%d", "foodDifference=%d",
		"subjectivePopCost=%d", "subjectiveGoldCost=%d", "decisionValue=%d", "overflowValue=%d", "decisionMargin=%d",
		'"FORCED_PANIC"', '"UNHAPPINESS_RELIEF"', '"FOOD_LOSS_RELIEF"', '"UNIT_VALUE"', '"BUILDING_VALUE"',
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI hurry diagnostic token {required}")
	m_record = re.search(r"void logSASGameRecordAIHurryDecision\([^\)]*\)\s*\{(?P<body>.*?)^\}", record_text, flags=re.DOTALL | re.MULTILINE)
	if m_record is None:
		failures.append(f"{REVISION_SOURCE}: could not locate AI hurry provenance definition")
	elif "gGameRecordLogLevel" in m_record.group("body"):
		failures.append(f"{REVISION_SOURCE}: AI hurry serializer must rely on realized caller-side level-2 pre-gating")

	city_text = (repo_root / "CvGameCoreDLL/CvCityAI.cpp").read_text(encoding="utf-8", errors="replace")
	m = re.search(r"void CvCityAI::AI_doHurry\(bool bForce\)(?P<body>.*?)^\}", city_text, flags=re.DOTALL | re.MULTILINE)
	if m is None:
		failures.append("CvGameCoreDLL/CvCityAI.cpp: could not locate AI_doHurry")
		return failures
	body = m.group("body")
	if body.count("hurry(eHurry);") != 5:
		failures.append("CvGameCoreDLL/CvCityAI.cpp: AI_doHurry realized hurry exit count changed")
	if body.count("logSASGameRecordAIHurryDecision(") != 5:
		failures.append("CvGameCoreDLL/CvCityAI.cpp: every realized AI_doHurry exit must retain one specialized provenance bridge")
	if body.count("if (gGameRecordLogLevel >= 2)") != 5:
		failures.append("CvGameCoreDLL/CvCityAI.cpp: AI hurry provenance must stay pre-gated only at the five realized hurry exits")
	if "AI_buildingValue(eProductionBuilding, 0, 0, false" not in body:
		failures.append("CvGameCoreDLL/CvCityAI.cpp: native building-hurry valuation structure changed")
	if "if (iHurryCost < iValue + /* advc.121b: */ iOverflow)" not in body or "if (iValue + iOverflow > iHurryCost)" not in body:
		failures.append("CvGameCoreDLL/CvCityAI.cpp: native unit/building hurry value gates changed")
	return failures


def check_ai_vassal_resource_tribute_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI vassal-resource tribute provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	if "logSASGameRecordAIVassalResourceTributeDecision(" not in header_text:
		failures.append(f"{REVISION_HEADER}: missing AI vassal-resource tribute provenance declaration")
	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_VASSAL_RESOURCE_TRIBUTE", "master=%d", "vassal=%d", "vassalHuman=%d",
		"vassalCapitulated=%d", "delivery=%s", "selectedBonus=%s", "selectedValue=%d",
		"runnerUpBonus=%s", "runnerUpValue=%d", "selectedAdvantage=%d", "candidateCount=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI vassal-resource tribute diagnostic token {required}")
	m_record = re.search(r"void logSASGameRecordAIVassalResourceTributeDecision\([^\)]*\)\s*\{(?P<body>.*?)^\}", record_text, flags=re.DOTALL | re.MULTILINE)
	if m_record is None:
		failures.append(f"{REVISION_SOURCE}: could not locate AI vassal-resource tribute provenance definition")
	else:
		record_body = m_record.group("body")
		if "gGameRecordLogLevel" in record_body:
			failures.append(f"{REVISION_SOURCE}: vassal-resource tribute serializer must rely on the caller's level-2 pre-gate")
		if "if (eVassal == NO_PLAYER || eSelectedBonus == NO_BONUS)" not in record_body:
			failures.append(f"{REVISION_SOURCE}: vassal-resource tribute serializer must retain cheap target/selected-bonus safety")

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	m = re.search(r"if \(GET_TEAM\(ePlayer\)\.isVassal\(getTeam\(\)\) &&\s*\n\s*kPlayer\.canPossiblyTradeItem\(getID\(\), TRADE_RESOURCES\)\).*?(?=\n\s*if \(kOurTeam\.getLeaderID\(\) == getID\(\) &&)", player_text, flags=re.DOTALL)
	if m is None:
		failures.append(f"{PLAYER_AI_SOURCE}: could not locate automatic vassal-resource tribute block")
		return failures
	body = m.group(0)
	for token, expected in (
		("AI_bonusTradeVal(eLoopBonus, ePlayer, 1)", 1),
		("logSASGameRecordAIVassalResourceTributeDecision(", 2),
		("BUTTONPOPUP_VASSAL_GRANT_TRIBUTE", 1),
		("kGame.implementDeal(getID(), ePlayer, weGive, theyGive)", 1),
	):
		actual = body.count(token)
		if actual != expected:
			failures.append(f"{PLAYER_AI_SOURCE}: vassal-resource tribute live call count for {token} changed; expected {expected}, found {actual}")
	for required in (
		"kPlayer.getNumTradeableBonuses(eLoopBonus) > 0",
		"getNumAvailableBonuses(eLoopBonus) == 0",
		"kPlayer.canTradeItem(getID(), TradeData(",
		"bool const bLogSASVassalResourceTribute = (gGameRecordLogLevel >= 2);",
		"if (bLogSASVassalResourceTribute)",
	):
		if required not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: missing vassal-resource tribute chooser/pre-gate invariant {required}")
	if any(token in body for token in ("SyncRand", "AI_counterPropose(")):
		failures.append(f"{PLAYER_AI_SOURCE}: automatic vassal-resource tribute chooser should remain RNG/counterproposal-free")
	return failures


def check_ai_war_trade_intent_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI war-trade intent provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	expected_paths = ["SAS_AI_WAR_TRADE_TECH_GOLD", "SAS_AI_WAR_TRADE_CITY_COUNTERPROPOSE"]
	m = re.search(r"enum\s+SASGameRecordAIWarTradePaymentPath\s*\{(?P<body>.*?)\};", header_text, flags=re.DOTALL)
	if m is None:
		failures.append(f"{REVISION_HEADER}: missing SASGameRecordAIWarTradePaymentPath")
	else:
		tokens = re.findall(r"^\s*(SAS_AI_WAR_TRADE_[A-Z0-9_]+)\s*,?\s*$", m.group("body"), flags=re.MULTILINE)
		if tokens != expected_paths:
			failures.append(f"{REVISION_HEADER}: AI war-trade payment vocabulary changed; expected {expected_paths}, found {tokens}")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_WAR_TRADE_INTENT", "source=%s", "minAtWarCounter=%d", "offerDenominator=%d",
		"targetTeam=%d", "uwaiTargetValue=%d", "legacyTargetScore=%d", "hirePrice=%d", "paymentPath=%s",
		"candidateTech1=%s", "candidateTech2=%s", "warSuccessRating=%d", "cityId=%d", "cityFitness=%d",
		"finalWarSideValue=%d", "finalPaymentSideValue=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI war-trade intent diagnostic token {required}")
	for token in expected_paths:
		if record_text.count(f"case {token}:") != 1:
			failures.append(f"{REVISION_SOURCE}: AI war-trade payment path {token} must have exactly one stringifier case")

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	m = re.search(r"void\s+CvPlayerAI::AI_proposeWarTrade\([^)]*\)\s*\{(?P<body>.*?)^\}\s*\n\n//  Same assumptions as for contactReligion", player_text, flags=re.DOTALL | re.MULTILINE)
	if m is None:
		return failures + [f"{PLAYER_AI_SOURCE}: could not locate AI_proposeWarTrade"]
	body = m.group("body")
	if body.count("logSASGameRecordAIWarTradeIntent(") != 2:
		failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeWarTrade must emit exactly two realized war-trade intent bridges, found {body.count('logSASGameRecordAIWarTradeIntent(')}")
	if body.count("logSASGameRecordAIDiploContactIntent(") != 2:
		failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeWarTrade must retain exactly two generic CONTACT_JOIN_WAR package bridges")
	if body.count("SAS_AI_WAR_TRADE_TECH_GOLD") != 1 or body.count("SAS_AI_WAR_TRADE_CITY_COUNTERPROPOSE") != 1:
		failures.append(f"{PLAYER_AI_SOURCE}: both realized AI war-trade payment paths must remain bridged exactly once")
	if body.count("gGameRecordLogLevel >= 2") < 2:
		failures.append(f"{PLAYER_AI_SOURCE}: realized AI war-trade intent bridges must remain level-2 pre-gated")
	for token, expected in (("tradeValJointWar(", 1), ("AI_declareWarTradeVal(", 2), ("AI_counterPropose(", 1), ("SyncRandSuccess(", 1), ("SyncRandNum(1000)", 1), ("SyncRandNum(100)", 2), ("kGame.implementDeal(", 2)):
		actual = body.count(token)
		if actual != expected:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeWarTrade live call count for {token} changed; expected {expected}, found {actual}")
	if "iBestTeamPrice = kOurTeam.AI_declareWarTradeVal" not in body:
		failures.append(f"{PLAYER_AI_SOURCE}: KI#372 legacy joint-war target must retain its cached non-sentinel hire price")
	if "if (AI_counterPropose(eHireling, hirelingGives, weGive, true, true))" not in body:
		failures.append(f"{PLAYER_AI_SOURCE}: KI#674 city-for-war implementation must remain conditional on successful AI_counterPropose")
	if "-100, -1, 0, iWeReceive, iHirelingReceives" not in body:
		failures.append(f"{PLAYER_AI_SOURCE}: ordinary war-trade row must retain the documented no-city sentinel and live final balance values")
	return failures


def check_ai_city_trade_intent_provenance(repo_root: Path) -> list[str]:
	failures = []
	for relative_path in (REVISION_HEADER, REVISION_SOURCE, PLAYER_AI_SOURCE):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing AI city-trade intent provenance file: {relative_path}")
	if failures:
		return failures

	header_text = (repo_root / REVISION_HEADER).read_text(encoding="utf-8", errors="replace")
	expected_formations = [
		"SAS_AI_CITY_TRADE_FREE_LIBERATION", "SAS_AI_CITY_TRADE_FREE_CITY_TO_HUMAN",
		"SAS_AI_CITY_TRADE_OUR_SIDE_COUNTERPROPOSE", "SAS_AI_CITY_TRADE_TARGET_SIDE_COUNTERPROPOSE",
		"SAS_AI_CITY_TRADE_SAME_TEAM_OVERRIDE",
	]
	m = re.search(r"enum\s+SASGameRecordAICityTradeFormation\s*\{(?P<body>.*?)\};", header_text, flags=re.DOTALL)
	if m is None:
		failures.append(f"{REVISION_HEADER}: missing SASGameRecordAICityTradeFormation")
	else:
		tokens = re.findall(r"^\s*(SAS_AI_CITY_TRADE_[A-Z0-9_]+)\s*,?\s*$", m.group("body"), flags=re.MULTILINE)
		if tokens != expected_formations:
			failures.append(f"{REVISION_HEADER}: AI city-trade formation vocabulary changed; expected {expected_formations}, found {tokens}")

	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_AI_CITY_TRADE_INTENT", "delivery=%s", "attitudeValue=%d", "formation=%s", "candidateRank=%d",
		"candidates=%d", "initialValueGap=%d", "inverseGapFallback=%d", "ourCityId=%d", "theirCityId=%d",
		"liberation=%d", "evacuating=%d", "sameTeam=%d", "negotiable=%d", "aiGives=%s", "aiReceives=%s",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: missing AI city-trade intent diagnostic token {required}")
	for token in expected_formations:
		if record_text.count(f"case {token}:") != 1:
			failures.append(f"{REVISION_SOURCE}: AI city-trade formation {token} must have exactly one stringifier case")

	player_text = (repo_root / PLAYER_AI_SOURCE).read_text(encoding="utf-8", errors="replace")
	m = re.search(r"bool\s+CvPlayerAI::AI_proposeCityTrade\([^)]*\)\s*\{(?P<body>.*?)^\}\s*// advc: End of functions cut from AI_doDiplo", player_text, flags=re.DOTALL | re.MULTILINE)
	if m is None:
		return failures + [f"{PLAYER_AI_SOURCE}: could not locate AI_proposeCityTrade"]
	body = m.group("body")
	if body.count("logSASGameRecordAICityTradeIntent(") != 1:
		failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeCityTrade must emit exactly one realized city-trade intent bridge")
	if not re.search(r"if\s*\(gGameRecordLogLevel\s*>=\s*2\)\s*\{[^{}]*logSASGameRecordAICityTradeIntent", body, flags=re.DOTALL):
		failures.append(f"{PLAYER_AI_SOURCE}: realized AI city-trade intent must remain level-2 pre-gated")
	if body.count("AI_intendsToCede(") != 3:
		failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeCityTrade cede-evaluation count changed; expected 3 live calls, found {body.count('AI_intendsToCede(')}")
	if body.count("AI_counterPropose(") != 2:
		failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeCityTrade counterproposal count changed; expected 2 live calls, found {body.count('AI_counterPropose(')}")
	if re.search(r"SyncRand|getSorenRand|MapRand", body):
		failures.append(f"{PLAYER_AI_SOURCE}: AI city-trade recorder path must not add RNG to AI_proposeCityTrade")
	log_pos = body.find("logSASGameRecordAICityTradeIntent(")
	for action in ("gDLL->beginDiplomacy(", "kGame.implementDeal("):
		pos = body.find(action)
		if pos < 0:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeCityTrade missing expected realized action {action}")
		elif log_pos >= 0 and log_pos > pos:
			failures.append(f"{PLAYER_AI_SOURCE}: AI city-trade intent must emit before {action}")
	for token in expected_formations:
		if token not in body:
			failures.append(f"{PLAYER_AI_SOURCE}: AI_proposeCityTrade no longer assigns realized formation {token}")
	return failures


def check_broad_settlement_vassal_battle_contracts(repo_root: Path) -> list[str]:
	"""Keep revision-126 broad settlement, vassal and exact-battle facts semantically unambiguous."""
	failures = []
	for relative_path in (REVISION_SOURCE, CITY_SITE_EVALUATOR_HEADER):
		if not (repo_root / relative_path).is_file():
			failures.append(f"missing SASGameRecord settlement/vassal contract file: {relative_path}")
	if failures:
		return failures
	record_text = (repo_root / REVISION_SOURCE).read_text(encoding="utf-8", errors="replace")
	evaluator_text = (repo_root / CITY_SITE_EVALUATOR_HEADER).read_text(encoding="utf-8", errors="replace")
	for required in (
		"GAME_RECORD_CITY_SITE_QUALITY", "knownBest1Sum=%d", "knownBest6Sum=%d", "knownBest14Sum=%d",
		"omniscientBest1Sum=%d", "omniscientBest6Sum=%d", "omniscientBest14Sum=%d",
		"knownPositivePlots=%d", "omniscientPositivePlots=%d", "knownWaterBonuses=%d", "omniscientWaterBonuses=%d", "nearestOwnCityDistance=%d",
		"knownRescoredFoundValue=%d", "omniscientRescoredFoundValue=%d",
		"kKnownEvaluator.setDebug(true)", "kOmniscientEvaluator.setDebug(true)", "setDiagnosticOmniscience(true)",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: broad chosen-site quality contract missing {required}")
	if "evaluateWithPlotValueDistribution" not in evaluator_text or "1/2/3/6/10/14" not in evaluator_text:
		failures.append(f"{CITY_SITE_EVALUATOR_HEADER}: chosen-site distribution bridge changed")

	for required in (
		"GAME_RECORD_VASSAL_RULES", "vassalLandFloor=10", "relativeSizeMasterLand=OWN_ONLY",
		"masterLossMasterLand=INCLUDING_VASSAL_CONTRIBUTION",
		"GAME_RECORD_VASSAL_STATUS", "vassalRuleLand=%d", "originalVassalLand=%d",
		"masterOwnLand=%d", "masterLossRuleLand=%d", "originalMasterLossRuleLand=%d",
		"revoltByRelativeSize=%d", "revoltByLosses=%d", "canVassalRevolt=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: broad vassal-status contract missing {required}")
	if record_text.count("logSASGameRecordVassalStatus(") < 3:
		failures.append(f"{REVISION_SOURCE}: vassal status must remain defined plus emitted at relationship start and periodic team snapshots")

	for required in (
		"GAME_RECORD_BATTLE_NONLETHAL", "GAME_RECORD_BATTLE turn=%d",
		"attackerFromX=%d", "attackerFromY=%d", "riverCrossingAttack=%d", "riverAttackPenalty=%d",
		"amphibiousAttack=%d", "amphibiousAttackPenalty=%d",
	):
		if required not in record_text:
			failures.append(f"{REVISION_SOURCE}: exact battle tactical-context contract missing {required}")
	return failures


EXPECTED_GAME_RECORD_DEFAULTS = {
	"SAS_GAME_RECORD_LOG_LEVEL": 0,
	# These configure enabled record logging but do not enable it themselves.
	"SAS_GAME_RECORD_INTERVAL_TURNS_UNSCALED_GAMESPEED": 10,
	"SAS_GAME_RECORD_LOG_USE_TIMESTAMPED_FILENAME": 1,
	"SAS_GAME_RECORD_PERFORMANCE_METRICS_ENABLE": 1,
	"SAS_GAME_RECORD_SYSTEM_CONTEXT_LEVEL": 2,
}


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that SASGameRecord report logging is disabled by default unless explicitly listed as non-enabling configuration.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	defines = read_global_define_ints(args.repo_root)
	failures = require_int_values(defines, EXPECTED_GAME_RECORD_DEFAULTS)
	failures.extend(check_revision(args.repo_root))
	failures.extend(check_context_consumer_contracts(args.repo_root))
	failures.extend(check_city_delta_contracts(args.repo_root))
	failures.extend(check_broad_settlement_vassal_battle_contracts(args.repo_root))
	failures.extend(check_ai_strategy_diagnostics(args.repo_root))
	failures.extend(check_area_ai_diagnostics(args.repo_root))
	failures.extend(check_ai_target_city_provenance(args.repo_root))
	failures.extend(check_ai_attitude_breakdown(args.repo_root))
	failures.extend(check_strategic_trade_market(args.repo_root))
	failures.extend(check_uwai_war_plan_decisions(args.repo_root))
	failures.extend(check_ai_diplo_vote_provenance(args.repo_root))
	failures.extend(check_ai_diplo_contact_provenance(args.repo_root))
	failures.extend(check_ai_help_tribute_request_provenance(args.repo_root))
	failures.extend(check_ai_give_help_provenance(args.repo_root))
	failures.extend(check_ai_tech_trade_provenance(args.repo_root))
	failures.extend(check_ai_deal_cancellation_provenance(args.repo_root))
	failures.extend(check_deal_invalidation_provenance(args.repo_root))
	failures.extend(check_ai_city_trade_intent_provenance(args.repo_root))
	failures.extend(check_ai_embargo_request_provenance(args.repo_root))
	failures.extend(check_ai_joint_war_request_provenance(args.repo_root))
	failures.extend(check_ai_vassalage_provenance(args.repo_root))
	failures.extend(check_ai_colony_split_provenance(args.repo_root))
	failures.extend(check_ai_spaceship_launch_provenance(args.repo_root))
	failures.extend(check_ai_religion_spread_target_provenance(args.repo_root))
	failures.extend(check_ai_map_trade_provenance(args.repo_root))
	failures.extend(check_unit_completion_sources(args.repo_root))
	failures.extend(check_ai_draft_provenance(args.repo_root))
	failures.extend(check_ai_hurry_provenance(args.repo_root))
	failures.extend(check_ai_vassal_resource_tribute_provenance(args.repo_root))
	failures.extend(check_ai_war_trade_intent_provenance(args.repo_root))
	failures.extend(check_ai_conquer_city_provenance(args.repo_root))
	failures.extend(check_ai_great_person_provenance(args.repo_root))
	failures.extend(check_ai_great_general_provenance(args.repo_root))
	if failures:
		print("FAIL SASGameRecord report/revision checks")
		for failure in failures:
			print(f"  - {failure}")
		return 1
	print(f"PASS SASGameRecord report/revision checks: logging defaults={len(EXPECTED_GAME_RECORD_DEFAULTS)}, revision history/current marker/context-consumer-contracts/city-delta/settlement-vassal-battle/AI-strategy/AreaAI/AI-target-city/AI-attitude/strategic-trade/UWAI-war-plan/AI-vote/AI-contact/AI-help-tribute/AI-give-help/AI-tech-trade/AI-deal-cancel/deal-invalidation/AI-city-trade/AI-embargo/AI-joint-war/AI-vassalage/AI-colony-split/AI-spaceship-launch/AI-religion-spread-target/AI-map-trade/unit-completion-sources/AI-vassal-resource-tribute/AI-draft/AI-hurry/AI-war-trade/AI-conquer-city/AI-Great-Person/AI-Great-General diagnostics synchronized")
	return 0


if __name__ == "__main__":
	sys.exit(main())
