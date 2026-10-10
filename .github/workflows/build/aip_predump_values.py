#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Build-check helper: compare LeaderHeadInfo AIP predump values against a lightweight Python mirror of CvLeaderHeadInfo XML defaults plus UWAI::applyPersonalityWeight.
# This is intentionally a separate script from build/aip.py so the always-on AIP release-safety check can stay lightweight while this deeper effective-value mirror can validate committed predump values.
# Direct scalar/simple-array values are checked here, and derived contact/memory aggregate values reuse the same pre-normalization AIP helper logic as the in-game Sevopedia AIP cache.
# UnitAIWeightModifiers and ImprovementWeightModifiers are not checked because they are not currently displayed/predumped. (ChatGPT-5.5) -->

from __future__ import annotations

import argparse
import ast
import datetime as _datetime
import math
import re
import sys
from copy import deepcopy
from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple
from xml.etree import ElementTree as ET

PREDUMP_RELATIVE_PATH = Path("Assets/Python/Contrib/Sevopedia/SevoPediaLeaderCachePredumped.py")
LEADER_XML_RELATIVE_PATH = Path("Assets/XML/Civilizations/CIV4LeaderHeadInfos.xml")
UWAI_DEFINE_NAME = "UWAI_PERSONALITY_PERCENT"
DEFAULTS_LEADER_TYPE = "LEADER_DEFAULTS"
AIP_EXCLUDED_LEADER_TYPES: Tuple[str, ...] = ()

# <!-- custom: Shared AIP enum/type lists, direct field specs, XML array specs, and aggregate key-family metadata are imported from ai_utils_shared_with_civ4.py in configure_shared_aip_constants().
# Keeping them shared avoids drifting copies of getter fields, CONTACT_*, MEMORY_*, attitude indexes, and iAggregated* metadata. (ChatGPT-5.5) -->

ATTITUDE_TO_INDEX: Dict[str, int] = {}
FLAVOR_TYPES: Tuple[str, ...] = ()
NO_WAR_ATTITUDE_TYPES: Tuple[str, ...] = ()
CONTACT_TYPES: Tuple[str, ...] = ()
MEMORY_TYPES: Tuple[str, ...] = ()
DIRECT_INT_FIELDS: Tuple[Tuple[str, str, int], ...] = ()
ATTITUDE_THRESHOLD_FIELDS: Tuple[Tuple[str, str, int], ...] = ()
EXTRA_UWAI_ONLY_INT_FIELDS: Tuple[Tuple[str, str, int], ...] = ()
ALL_INT_FIELDS: Tuple[Tuple[str, str, int], ...] = ()
DISPLAY_ARRAY_FIELD_SPECS: Tuple[Tuple[str, str, str, Tuple[str, ...], str], ...] = ()
HIDDEN_ARRAY_FIELD_SPECS: Tuple[Tuple[str, str, str, Tuple[str, ...], str], ...] = ()
ARRAY_FIELD_SPECS: Tuple[Tuple[str, str, str, Tuple[str, ...], str], ...] = ()

RAW_INT_RE = re.compile(r"\((-?\d+)\)")


@dataclass
class LeaderRecord:
	index: int
	type: str
	values: Dict[str, int]


@dataclass
class ComparisonResult:
	leader_index: int
	leader_type: str
	getter_key: str
	expected: Any
	actual: Any
	status: str


def find_repo_root(start: Path) -> Path:
	current = start.resolve()
	for candidate in (current,) + tuple(current.parents):
		if (candidate / "Assets").is_dir() and (candidate / "CvGameCoreDLL").is_dir():
			return candidate
	raise RuntimeError("Could not locate repo root containing Assets/ and CvGameCoreDLL/.")


def strip_xml_namespaces(root: ET.Element) -> None:
	for elem in root.iter():
		if "}" in elem.tag:
			elem.tag = elem.tag.split("}", 1)[1]


def child_text(elem: ET.Element, tag: str) -> Optional[str]:
	child = elem.find(tag)
	if child is None or child.text is None:
		return None
	text = child.text.strip()
	return text if text != "" else None


def parse_int(text: str, path: Path, leader_type: str, tag: str) -> int:
	try:
		return int(text)
	except ValueError as exc:
		raise ValueError(f"{path}: leader {leader_type}: tag {tag} expected int, got {text!r}") from exc


def get_pascal_case_suffix(xml_type: str) -> str:
	# Mirrors the AIP helper convention: FLAVOR_MILITARY -> Military,
	# ATTITUDE_FURIOUS -> Furious.
	parts = [part for part in xml_type.split("_") if part]
	if len(parts) > 1:
		parts = parts[1:]
	return "".join(part[:1] + part[1:].lower() for part in parts)


def array_value_key(key_prefix: str, xml_type: str) -> str:
	return "%s%s" % (key_prefix, get_pascal_case_suffix(xml_type))


def displayed_value_keys(shared_helpers: Any) -> Tuple[str, ...]:
	array_keys: List[str] = []
	for _root_tag, _key_tag, _value_tag, type_names, key_prefix in DISPLAY_ARRAY_FIELD_SPECS:
		for type_name in type_names:
			array_keys.append(array_value_key(key_prefix, type_name))
	# <!-- custom: displayed aggregate keys are shared with the in-game AIP helper layer, so the workflow checker does not duplicate which synthetic contact/memory fields are predumped. (ChatGPT-5.5) -->
	aggregate_keys = shared_helpers.get_aip_displayed_aggregate_value_keys()
	return (
		tuple(getter for getter, _tag, _default in DIRECT_INT_FIELDS)
		+ tuple(getter for getter, _tag, _default in ATTITUDE_THRESHOLD_FIELDS)
		+ tuple(array_keys)
		+ tuple(aggregate_keys)
	)


def uwai_primitive_value_keys() -> Tuple[str, ...]:
	array_keys: List[str] = []
	for _root_tag, _key_tag, _value_tag, type_names, key_prefix in ARRAY_FIELD_SPECS:
		for type_name in type_names:
			array_keys.append(array_value_key(key_prefix, type_name))
	return (
		tuple(getter for getter, _tag, _default in ALL_INT_FIELDS)
		+ tuple(getter for getter, _tag, _default in ATTITUDE_THRESHOLD_FIELDS)
		+ tuple(array_keys)
	)


def constructor_defaults() -> Dict[str, int]:
	values: Dict[str, int] = {}
	for getter, _tag, _explicit_default in ALL_INT_FIELDS:
		values[getter] = 0
	for getter, _tag, constructor_default in ATTITUDE_THRESHOLD_FIELDS:
		values[getter] = constructor_default
	for _root_tag, _key_tag, _value_tag, type_names, key_prefix in ARRAY_FIELD_SPECS:
		for type_name in type_names:
			values[array_value_key(key_prefix, type_name)] = 0
	return values


def read_array_fields(values: Dict[str, int], leader_elem: ET.Element, xml_path: Path, leader_type: str) -> None:
	for root_tag, key_tag, value_tag, type_names, key_prefix in ARRAY_FIELD_SPECS:
		known_types = set(type_names)
		root = leader_elem.find(root_tag)
		if root is None:
			continue
		for item in list(root):
			type_name = child_text(item, key_tag)
			value_text = child_text(item, value_tag)
			if type_name is None or value_text is None:
				continue
			if type_name not in known_types:
				raise ValueError(f"{xml_path}: leader {leader_type}: tag {root_tag}/{key_tag} unknown type {type_name!r}")
			values[array_value_key(key_prefix, type_name)] = parse_int(value_text, xml_path, leader_type, value_tag)


def read_leader_record(base_values: Dict[str, int], leader_elem: ET.Element, xml_path: Path) -> Tuple[str, Dict[str, int]]:
	leader_type = child_text(leader_elem, "Type")
	if leader_type is None:
		raise ValueError(f"{xml_path}: LeaderHeadInfo without <Type>")
	values = deepcopy(base_values)

	for getter, tag, explicit_default in ALL_INT_FIELDS:
		text = child_text(leader_elem, tag)
		if text is not None:
			values[getter] = parse_int(text, xml_path, leader_type, tag)
		else:
			# Mirrors CvLeaderHeadInfo::GetChildXmlValByName: if the copied value is
			# non-zero, it becomes the missing-tag default; otherwise use the explicit
			# default argument from CvLeaderHeadInfo::read.
			values[getter] = values[getter] if values[getter] != 0 else explicit_default

	for getter, tag, _constructor_default in ATTITUDE_THRESHOLD_FIELDS:
		text = child_text(leader_elem, tag)
		if text is None:
			continue
		if text not in ATTITUDE_TO_INDEX:
			raise ValueError(f"{xml_path}: leader {leader_type}: tag {tag} unknown attitude value {text!r}")
		values[getter] = ATTITUDE_TO_INDEX[text]

	read_array_fields(values, leader_elem, xml_path, leader_type)

	return leader_type, values


def read_leaders_from_xml(repo_root: Path) -> List[LeaderRecord]:
	xml_path = repo_root / LEADER_XML_RELATIVE_PATH
	root = ET.parse(str(xml_path)).getroot()
	strip_xml_namespaces(root)
	infos = root.findall("./LeaderHeadInfos/LeaderHeadInfo")
	if not infos:
		raise ValueError(f"{xml_path}: no LeaderHeadInfo entries found")

	leaders: List[LeaderRecord] = []
	defaults_template: Optional[Dict[str, int]] = None
	for info in infos:
		base = constructor_defaults() if defaults_template is None else deepcopy(defaults_template)
		leader_type, values = read_leader_record(base, info, xml_path)
		if leader_type == DEFAULTS_LEADER_TYPE:
			defaults_template = values
			continue
		leaders.append(LeaderRecord(index=len(leaders), type=leader_type, values=values))
	return leaders


def read_global_define_int(repo_root: Path, define_name: str) -> int:
	for path in sorted((repo_root / "Assets" / "XML").rglob("*.xml")):
		try:
			root = ET.parse(str(path)).getroot()
		except ET.ParseError:
			continue
		strip_xml_namespaces(root)
		for define in root.findall(".//Define"):
			name = child_text(define, "DefineName")
			if name != define_name:
				continue
			text = child_text(define, "iDefineIntVal")
			if text is None:
				raise ValueError(f"{path}: {define_name} missing iDefineIntVal")
			return parse_int(text, path, define_name, "iDefineIntVal")
	raise ValueError(f"Could not find {define_name} under Assets/XML")


def median_fraction(values: List[int]) -> Fraction:
	ordered = sorted(values)
	mid = len(ordered) // 2
	if len(ordered) % 2:
		return Fraction(ordered[mid], 1)
	return Fraction(ordered[mid - 1] + ordered[mid], 2)


def round_cpp_scaled(value: Fraction) -> int:
	# Mirrors ScaledNum::round for signed values: add +/- 0.5, then truncate
	# toward zero. For exact Fractions, floor/ceil express that behavior cleanly.
	if value >= 0:
		return math.floor(value + Fraction(1, 2))
	return math.ceil(value - Fraction(1, 2))


def apply_uwai_personality_weight(leaders: List[LeaderRecord], weight: int) -> None:
	if weight == 100:
		return
	personality_leaders = [leader for leader in leaders if leader.type not in AIP_EXCLUDED_LEADER_TYPES]
	if not personality_leaders:
		return
	for getter_key in uwai_primitive_value_keys():
		median = median_fraction([leader.values[getter_key] for leader in personality_leaders])
		for leader in personality_leaders:
			old_value = leader.values[getter_key]
			new_value = (median * (100 - weight) + old_value * weight) / 100
			leader.values[getter_key] = round_cpp_scaled(new_value)


def import_aip_shared_helpers(repo_root: Path) -> Any:
	sevopedia_path = repo_root / "Assets" / "Python" / "Contrib" / "Sevopedia"
	sevopedia_path_text = str(sevopedia_path)
	if sevopedia_path_text not in sys.path:
		sys.path.insert(0, sevopedia_path_text)
	import ai_utils_shared_with_civ4  # pylint: disable=import-error,import-outside-toplevel
	return ai_utils_shared_with_civ4


def configure_shared_aip_constants(shared_helpers: Any) -> None:
	# <!-- custom: Keep enum/type lists, direct getter metadata, Civ4 attitude-index mapping, XML array specs, and aggregate key-family metadata shared with the runtime AIP helpers. (ChatGPT-5.5) -->
	global AIP_EXCLUDED_LEADER_TYPES, ATTITUDE_TO_INDEX, FLAVOR_TYPES, NO_WAR_ATTITUDE_TYPES, CONTACT_TYPES, MEMORY_TYPES
	global DIRECT_INT_FIELDS, ATTITUDE_THRESHOLD_FIELDS, EXTRA_UWAI_ONLY_INT_FIELDS, ALL_INT_FIELDS
	global DISPLAY_ARRAY_FIELD_SPECS, HIDDEN_ARRAY_FIELD_SPECS, ARRAY_FIELD_SPECS
	AIP_EXCLUDED_LEADER_TYPES = tuple(shared_helpers.get_aip_excluded_leader_types())
	ATTITUDE_TO_INDEX = shared_helpers.get_aip_attitude_type_to_index()
	FLAVOR_TYPES = tuple(shared_helpers.get_aip_flavor_types_assessed())
	NO_WAR_ATTITUDE_TYPES = tuple(shared_helpers.get_aip_no_war_attitude_types_assessed())
	CONTACT_TYPES = tuple(shared_helpers.get_aip_contact_types_assessed())
	MEMORY_TYPES = tuple(shared_helpers.get_aip_memory_types_assessed())
	DIRECT_INT_FIELDS = tuple((getter, xml_tag, xml_default) for getter, xml_tag, xml_default, _label, _invert in shared_helpers.get_aip_direct_int_field_specs())
	ATTITUDE_THRESHOLD_FIELDS = tuple((getter, xml_tag, xml_default) for getter, xml_tag, xml_default, _label, _invert in shared_helpers.get_aip_attitude_threshold_field_specs())
	EXTRA_UWAI_ONLY_INT_FIELDS = tuple(shared_helpers.get_aip_extra_uwai_only_int_field_specs())
	ALL_INT_FIELDS = DIRECT_INT_FIELDS + EXTRA_UWAI_ONLY_INT_FIELDS
	DISPLAY_ARRAY_FIELD_SPECS = tuple(shared_helpers.get_aip_display_array_field_specs())
	HIDDEN_ARRAY_FIELD_SPECS = tuple(shared_helpers.get_aip_hidden_array_field_specs())
	ARRAY_FIELD_SPECS = tuple(shared_helpers.get_aip_array_field_specs())


class XmlAipValueProvider:
	# <!-- custom: Workflow-side provider for the shared pure AIP cache builder.
	# Values come from the XML mirror after LEADER_DEFAULTS, CvLeaderHeadInfo missing-tag defaults, and optional UWAI weighting have already been applied. (ChatGPT-5.5) -->
	def __init__(self, leaders: List[LeaderRecord]):
		self.leaders_by_index = {leader.index: leader for leader in leaders}

	def get_value(self, iLeader: int, cache_key: str) -> int:
		return self.leaders_by_index[iLeader].values[cache_key]

	def get_flavor_value(self, iLeader: int, iFlavor: int) -> int:
		return self.get_value(iLeader, array_value_key("iFlavor", FLAVOR_TYPES[iFlavor]))

	def get_no_war_attitude_prob(self, iLeader: int, iAttitude: int) -> int:
		return self.get_value(iLeader, array_value_key("iNoWarAttitudeProb", NO_WAR_ATTITUDE_TYPES[iAttitude]))

	def get_contact_rand(self, iLeader: int, iContact: int) -> int:
		return self.get_value(iLeader, array_value_key("iContactRand", CONTACT_TYPES[iContact]))

	def get_contact_delay(self, iLeader: int, iContact: int) -> int:
		return self.get_value(iLeader, array_value_key("iContactDelay", CONTACT_TYPES[iContact]))

	def get_memory_type(self, iMemoryIndex: int) -> str:
		return MEMORY_TYPES[iMemoryIndex]

	def get_memory_attitude_percent(self, iLeader: int, iMemoryIndex: int) -> int:
		return self.get_value(iLeader, array_value_key("iMemoryAttitudePercent", MEMORY_TYPES[iMemoryIndex]))

	def get_memory_decay_rand(self, iLeader: int, iMemoryIndex: int) -> int:
		return self.get_value(iLeader, array_value_key("iMemoryDecay", MEMORY_TYPES[iMemoryIndex]))


def get_non_excluded_leaders(leaders: List[LeaderRecord]) -> List[LeaderRecord]:
	return [leader for leader in leaders if leader.type not in AIP_EXCLUDED_LEADER_TYPES]


def build_expected_predump_cache(leaders: List[LeaderRecord], shared_helpers: Any, is_show_raw_xml_field_names_instead: bool) -> Dict[int, Dict[str, Tuple[Any, ...]]]:
	# <!-- custom: Expected predump creation now delegates to the same provider-based cache builder used by runtime AIP.
	# The workflow only supplies XML/UWAI values; labels, normalization, scale strings, aggregate selection, and tuple shape are shared. (ChatGPT-5.5) -->
	non_excluded_leader_indexes = tuple(leader.index for leader in get_non_excluded_leaders(leaders))
	return shared_helpers.compute_leaders_info_aip_cache_from_provider(non_excluded_leader_indexes, XmlAipValueProvider(leaders), is_show_raw_xml_field_names_instead, False, False)


def read_predump_cache(repo_root: Path) -> Dict[int, Dict[str, Tuple[Any, ...]]]:
	path = repo_root / PREDUMP_RELATIVE_PATH
	module_ast = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
	for node in module_ast.body:
		if isinstance(node, ast.Assign):
			for target in node.targets:
				if isinstance(target, ast.Name) and target.id == "LEADERS_INFO_CACHED":
					value = ast.literal_eval(node.value)
					if not isinstance(value, dict):
						raise ValueError(f"{path}: LEADERS_INFO_CACHED is not a dict")
					return value
	raise ValueError(f"{path}: could not find LEADERS_INFO_CACHED assignment")


def format_leader_index_type_pairs(leaders: List[LeaderRecord], shared_helpers: Any) -> str:
	return shared_helpers.format_aip_leader_index_type_pairs([(leader.index, leader.type) for leader in leaders])


def build_predump_module_text(leaders: List[LeaderRecord], expected_cache: Dict[int, Dict[str, Tuple[Any, ...]]], is_emoji_enabled: bool, is_raw_xml_names: bool, shared_helpers: Any) -> str:
	# <!-- custom: Deterministic outside-Civ4 writer for SevoPediaLeaderCachePredumped.py.
	# Tuple creation stays shared with runtime AIP; shared helper formatting also keeps the generated module header/data literal aligned with the in-game PythonDbg.log fallback.
	# The generated file intentionally has no timestamp, so a no-op --write run does not dirty the working tree. (ChatGPT-5.5) -->
	source_header_lines = (
		"# Generated by: .github/workflows/build/aip_predump_values.py --write",
	)
	leader_count_line = "#   - Number of leaders parsed from XML, excluding %s: %d" % (DEFAULTS_LEADER_TYPE, len(leaders))
	leader_index_type_line = format_leader_index_type_pairs(leaders, shared_helpers)
	lines = shared_helpers.build_aip_predump_module_lines(expected_cache, leader_count_line, AIP_EXCLUDED_LEADER_TYPES, leader_index_type_line, is_emoji_enabled, is_raw_xml_names, source_header_lines, True)
	return "\n".join(lines) + "\n"


def detect_existing_newline(path: Path) -> str:
	# <!-- custom: Preserve the checked-out predump file's newline style when --write rewrites it.
	# GitHub/Linux checkouts may present a file as LF even when a Windows working tree has CRLF, and hardcoding either style can make a no-op refresh look like every line changed. (ChatGPT-5.5) -->
	try:
		data = path.read_bytes()
	except FileNotFoundError:
		return "\n"
	crlf_count = data.count(b"\r\n")
	lf_count = data.count(b"\n")
	bare_lf_count = lf_count - crlf_count
	if crlf_count and not bare_lf_count:
		return "\r\n"
	if bare_lf_count and not crlf_count:
		return "\n"
	# Mixed or newline-less files should be rare here. Preserve the majority style rather than forcing a full-file diff; line_endings.py still catches mixed EOL.
	return "\r\n" if crlf_count > bare_lf_count else "\n"


def write_predump_cache(repo_root: Path, leaders: List[LeaderRecord], expected_cache: Dict[int, Dict[str, Tuple[Any, ...]]], is_emoji_enabled: bool, is_raw_xml_names: bool, shared_helpers: Any) -> Path:
	path = repo_root / PREDUMP_RELATIVE_PATH
	text = build_predump_module_text(leaders, expected_cache, is_emoji_enabled, is_raw_xml_names, shared_helpers)
	path.write_text(text, encoding="utf-8", newline=detect_existing_newline(path))
	return path


def compare_values(leaders: List[LeaderRecord], predump: Dict[int, Dict[str, Tuple[Any, ...]]], expected_cache: Dict[int, Dict[str, Tuple[Any, ...]]]) -> List[ComparisonResult]:
	results: List[ComparisonResult] = []
	for leader in get_non_excluded_leaders(leaders):
		expected_leader_cache = expected_cache[leader.index]
		actual_leader_cache = predump.get(leader.index)
		if actual_leader_cache is None:
			for getter_key, expected_tuple in sorted(expected_leader_cache.items()):
				results.append(ComparisonResult(leader.index, leader.type, getter_key, expected_tuple, None, "missing-leader"))
			continue

		for getter_key, expected_tuple in sorted(expected_leader_cache.items()):
			actual_tuple = actual_leader_cache.get(getter_key)
			if actual_tuple is None:
				results.append(ComparisonResult(leader.index, leader.type, getter_key, expected_tuple, None, "missing-field"))
				continue
			status = "match" if actual_tuple == expected_tuple else "mismatch"
			results.append(ComparisonResult(leader.index, leader.type, getter_key, expected_tuple, actual_tuple, status))
	return results


def print_failure_summary(results: List[ComparisonResult], max_rows: int = 20) -> None:
	failures = [result for result in results if result.status != "match"]
	if not failures:
		return
	print("")
	print("First failing AIP predump tuple entries:")
	for result in failures[:max_rows]:
		print("  %d %s %s status=%s" % (result.leader_index, result.leader_type, result.getter_key, result.status))
		print("    expected=%r" % (result.expected,))
		print("    actual  =%r" % (result.actual,))
	if len(failures) > max_rows:
		print("  ... %d more failing entries not shown" % (len(failures) - max_rows))

def normalize_markdown_lines(lines: List[str]) -> List[str]:
	normalized: List[str] = []
	previous_blank = False
	for line in lines:
		is_blank = line.strip() == ""
		if is_blank and previous_blank:
			continue
		normalized.append(line)
		previous_blank = is_blank
	while normalized and normalized[-1].strip() == "":
		normalized.pop()
	return normalized


def write_markdown_report(path: Path, repo_root: Path, leaders: List[LeaderRecord], weight: int, results: List[ComparisonResult], max_mismatches: int) -> None:
	compared = len(results)
	matches = sum(1 for result in results if result.status == "match")
	mismatches = [result for result in results if result.status == "mismatch"]
	missing = [result for result in results if result.status != "match" and result.status != "mismatch"]

	lines: List[str] = []
	lines.append("# Leader AIP predump effective-value check")
	lines.append("")
	lines.append("Current full-tuple comparison between `CIV4LeaderHeadInfos.xml`, shared AIP tuple helpers, and `SevoPediaLeaderCachePredumped.py`.")
	lines.append("")
	lines.append("This helper mirrors the narrow DLL path needed for effective AIP values checked by this script:")
	lines.append("")
	lines.append("- `LEADER_DEFAULTS` copy behavior from `CvXMLLoadUtility::SetGlobalClassInfo`.")
	lines.append("- `CvLeaderHeadInfo::GetChildXmlValByName` missing-tag behavior.")
	lines.append("- `UWAI::applyPersonalityWeight` for scalar primitive fields and checked primitive array/list fields.")
	lines.append("- Shared AIP contact/memory aggregate helpers for derived pre-normalization fields.")
	lines.append("- Shared AIP tuple formatting for labels, normalized values, and scale strings.")
	lines.append("")
	lines.append("Unit AI modifiers and improvement modifiers are intentionally not checked yet because they are not currently displayed/predumped by the AIP panel.")
	lines.append("")
	lines.append("## Summary")
	lines.append("")
	lines.append(f"- Repo root: `{repo_root}`")
	lines.append(f"- Leaders parsed, excluding `{DEFAULTS_LEADER_TYPE}`: {len(leaders)}")
	lines.append(f"- UWAI personality percent: {weight}")
	lines.append(f"- Effective-value entries compared: {compared}")
	lines.append(f"- Matches: {matches}")
	lines.append(f"- Mismatches: {len(mismatches)}")
	lines.append(f"- Missing/unparsed entries: {len(missing)}")
	lines.append("")
	if mismatches:
		lines.append("## Mismatches")
		lines.append("")
		lines.append("| Leader | Getter | Expected tuple | Predump tuple |")
		lines.append("|---|---|---|---|")
		for result in mismatches[:max_mismatches]:
			lines.append(f"| {result.leader_index}: `{result.leader_type}` | `{result.getter_key}` | `{result.expected!r}` | `{result.actual!r}` |")
		if len(mismatches) > max_mismatches:
			lines.append("")
			lines.append(f"_Only first {max_mismatches} mismatches shown._")
		lines.append("")
	if missing:
		lines.append("## Missing or unparsed entries")
		lines.append("")
		lines.append("| Leader | Getter | Expected tuple | Status |")
		lines.append("|---|---|---|---|")
		for result in missing[:max_mismatches]:
			lines.append(f"| {result.leader_index}: `{result.leader_type}` | `{result.getter_key}` | `{result.expected!r}` | {result.status} |")
		if len(missing) > max_mismatches:
			lines.append("")
			lines.append(f"_Only first {max_mismatches} missing/unparsed entries shown._")
		lines.append("")
	if not mismatches and not missing:
		lines.append("## Result")
		lines.append("")
		lines.append("All checked AIP predump tuples match the committed predump.")
		lines.append("")
	path.parent.mkdir(parents=True, exist_ok=True)
	path.write_text("\n".join(normalize_markdown_lines(lines)) + "\n", encoding="utf-8", newline="\n")


def default_output_path(repo_root: Path) -> Path:
	timestamp = _datetime.datetime.now().strftime("%Y%m%dT%H%M%S")
	return repo_root / "LLM_Helpers" / "outputs" / f"leader_aip_predump_values_{timestamp}.md"


def main() -> int:
	parser = argparse.ArgumentParser(description="Compare LeaderHeadInfo AIP predump values against XML+UWAI-emulated effective values.")
	parser.add_argument("--repo-root", type=Path, default=None, help="repository root; defaults to the current tree containing Assets/ and CvGameCoreDLL/")
	parser.add_argument("--output", type=Path, default=None, help="Markdown report path; defaults to LLM_Helpers/outputs/leader_aip_predump_values_<timestamp>.md")
	parser.add_argument("--max-mismatches", type=int, default=120, help="maximum mismatch rows to include in each report section")
	parser.add_argument("--write", action="store_true", help="rewrite SevoPediaLeaderCachePredumped.py from the reconstructed XML+UWAI AIP cache before comparing")
	parser.add_argument("--no-uwai", action="store_true", help="debug mode: do not apply UWAI_PERSONALITY_PERCENT before comparing")
	parser.add_argument("--allow-mismatch", action="store_true", help="debug mode: return exit code 0 even when mismatches or missing/unparsed entries are found")
	args = parser.parse_args()
	if args.write and args.no_uwai:
		parser.error("--write cannot be combined with --no-uwai because --no-uwai is a mismatch/debug mode")

	repo_root = find_repo_root(args.repo_root or Path.cwd())
	shared_helpers = import_aip_shared_helpers(repo_root)
	configure_shared_aip_constants(shared_helpers)
	# <!-- custom: Exercise the explicit cross-runtime half-tie contract before validating the predump values. See KI#322. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	shared_helpers.test_expected_rounding()
	leaders = read_leaders_from_xml(repo_root)
	weight = read_global_define_int(repo_root, UWAI_DEFINE_NAME)
	if args.no_uwai:
		weight = 100
	else:
		apply_uwai_personality_weight(leaders, weight)
	is_show_raw_xml_field_names_instead = read_global_define_int(repo_root, "SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_PANEL_SHOW_RAW_XML_FIELD_NAMES_INSTEAD") > 0
	is_emoji_enabled = read_global_define_int(repo_root, "SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_PANEL_SHOW_EMOJI") > 0
	expected_cache = build_expected_predump_cache(leaders, shared_helpers, is_show_raw_xml_field_names_instead)
	if args.write:
		written_path = write_predump_cache(repo_root, leaders, expected_cache, is_emoji_enabled, is_show_raw_xml_field_names_instead, shared_helpers)
		print(f"Wrote refreshed AIP predump cache: {written_path}")
	predump = read_predump_cache(repo_root)
	results = compare_values(leaders, predump, expected_cache)
	mismatch_count = sum(1 for result in results if result.status == "mismatch")
	missing_count = sum(1 for result in results if result.status != "match" and result.status != "mismatch")
	output_path = args.output or default_output_path(repo_root)
	write_markdown_report(output_path, repo_root, leaders, weight, results, args.max_mismatches)

	print_failure_summary(results)
	print("Leader AIP predump effective-value check")
	print(f"  compared: {len(results)}")
	print(f"  mismatches: {mismatch_count}")
	print(f"  missing/unparsed: {missing_count}")
	print(f"  report: {output_path}")
	if not args.allow_mismatch and (mismatch_count or missing_count):
		return 1
	return 0


if __name__ == "__main__":
	sys.exit(main())
