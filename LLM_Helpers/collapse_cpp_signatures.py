#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Conservative C++ source cleanup helper.
#
# Collapses only multiline function declarations/definitions whose parameter list can safely fit on one physical line.
# This is intentionally signature-only; do not add ordinary call rewriting here. Use a separate helper for calls if needed.
# This is not a formatter. Review the diff before committing generated changes.

from __future__ import annotations

from pathlib import Path
import argparse
from datetime import datetime, timezone
from collections import Counter
import difflib
import re
import subprocess
import sys


DEFAULT_SCAN_RELATIVE_PATHS = (Path("CvGameCoreDLL"),)
DEFAULT_TRACE_CREDIT = "no specified reviewer"
CPP_SUFFIXES = {".c", ".cpp", ".h", ".hpp", ".inl"}
CONTROL_START_WORDS = {"if", "else", "for", "while", "switch", "catch", "return", "sizeof", "IFLOG"}


class Rewrite:
	def __init__(self, start: int, end: int, lines: list[str], kind: str):
		self.start = start
		self.end = end
		self.lines = lines
		self.kind = kind


class IgnoredCandidate:
	def __init__(self, relative_path: Path, start: int, end: int, reason: str, lines: list[str]):
		self.relative_path = relative_path
		self.start = start
		self.end = end
		self.reason = reason
		self.lines = lines

	def format(self) -> str:
		header = f"{self.relative_path.as_posix()}:{self.start + 1}-{self.end + 1}: {self.reason}"
		body = "".join(self.lines)
		if body and not body.endswith("\n"):
			body += "\n"
		return f"{header}\n{body}"


class BlockCommentSpan:
	def __init__(self, start: int, end: int, start_column: int, end_column: int, payload: str):
		self.start = start
		self.end = end
		self.start_column = start_column
		self.end_column = end_column
		self.payload = payload


class Stats:
	def __init__(self):
		self.files_scanned = 0
		self.files_changed = 0
		self.signatures = 0
		self.ignored = 0


# -----------------------------------------------------------------------------
# File/path helpers


def get_default_repo_root() -> Path:
	return Path(__file__).resolve().parents[1]


def git_tracked_files(repo_root: Path) -> list[Path] | None:
	try:
		result = subprocess.run(
			["git", "ls-files", "-z"],
			cwd=repo_root,
			check=True,
			stdout=subprocess.PIPE,
			stderr=subprocess.PIPE,
		)
	except (OSError, subprocess.CalledProcessError):
		return None

	paths: list[Path] = []
	for raw_path in result.stdout.split(b"\0"):
		if raw_path:
			paths.append(repo_root / raw_path.decode("utf-8", errors="replace"))
	return paths


def is_cpp_path(path: Path) -> bool:
	return path.suffix.lower() in CPP_SUFFIXES


def is_under(relative_path: Path, relative_dir: Path) -> bool:
	return relative_path == relative_dir or relative_dir in relative_path.parents


def iter_candidate_files(repo_root: Path, input_paths: list[Path]) -> list[Path]:
	if input_paths:
		paths: list[Path] = []
		for input_path in input_paths:
			path = input_path if input_path.is_absolute() else repo_root / input_path
			if path.is_file():
				paths.append(path)
			elif path.is_dir():
				paths.extend(child for child in path.rglob("*") if child.is_file())
			else:
				print(f"WARN missing path: {input_path}", file=sys.stderr)
		return sorted(set(path for path in paths if is_cpp_path(path)))

	tracked_paths = git_tracked_files(repo_root)
	if tracked_paths is not None:
		return sorted(
			path
			for path in tracked_paths
			if path.is_file()
			and is_cpp_path(path)
			and any(is_under(path.relative_to(repo_root), scan_path) for scan_path in DEFAULT_SCAN_RELATIVE_PATHS)
		)

	paths: list[Path] = []
	for scan_path in DEFAULT_SCAN_RELATIVE_PATHS:
		path = repo_root / scan_path
		if not path.exists():
			continue
		if path.is_file():
			if is_cpp_path(path):
				paths.append(path)
			continue
		paths.extend(child for child in path.rglob("*") if child.is_file() and is_cpp_path(child))
	return sorted(set(paths))


# -----------------------------------------------------------------------------
# Text helpers


def decode_bytes(raw: bytes) -> tuple[str, str]:
	if raw.startswith(b"\xef\xbb\xbf"):
		return "utf-8-sig", raw.decode("utf-8-sig")
	try:
		return "utf-8", raw.decode("utf-8")
	except UnicodeDecodeError:
		return "latin-1", raw.decode("latin-1")


def split_line_ending(line: str) -> tuple[str, str]:
	if line.endswith("\r\n"):
		return line[:-2], "\r\n"
	if line.endswith("\n"):
		return line[:-1], "\n"
	if line.endswith("\r"):
		return line[:-1], "\r"
	return line, ""


def dominant_eol(lines: list[str]) -> str:
	counts = {"\r\n": 0, "\n": 0, "\r": 0}
	for line in lines:
		_, eol = split_line_ending(line)
		if eol:
			counts[eol] += 1
	return max(counts, key=counts.get) if any(counts.values()) else "\n"


def leading_ws(text: str) -> str:
	return text[: len(text) - len(text.lstrip(" \t"))]


def lone_line_comment_index(text: str) -> int:
	in_string = None
	escaped = False
	for i, ch in enumerate(text):
		if in_string is not None:
			if escaped:
				escaped = False
			elif ch == "\\":
				escaped = True
			elif ch == in_string:
				in_string = None
			continue
		if ch in ('"', "'"):
			in_string = ch
			continue
		if ch == "/" and i + 1 < len(text) and text[i + 1] == "/":
			return i
	return -1


def contains_lone_line_comment(text: str) -> bool:
	return lone_line_comment_index(text) >= 0


def strip_trailing_line_comment(text: str) -> str:
	comment_index = lone_line_comment_index(text)
	return text[:comment_index] if comment_index >= 0 else text


def strip_tail_comments_for_validation(text: str) -> str:
	text = strip_trailing_line_comment(text)
	# Safe only for tail validation; the original comment remains in the collapsed line.
	# Inline block comments inside the parameter list are still handled by comment safety checks.
	return re.sub(r"/\*.*?\*/", " ", text)


def has_only_trailing_signature_line_comment(text: str) -> bool:
	comment_index = lone_line_comment_index(text)
	if comment_index < 0:
		return False
	# Safe only when // is after the closing parenthesis of the signature tail,
	# e.g. `) const // advc` or `) const; // advc`. Comments inside the
	# parameter list remain unsafe because collapsing would comment out code.
	return text.rfind(")", 0, comment_index) >= 0


def exposed_to_python_line_comment_payload(text: str) -> str | None:
	comment_index = lone_line_comment_index(text)
	if comment_index < 0:
		return None
	payload = text[comment_index + 2 :].strip()
	return payload if "Exposed to Python" in payload else None


def is_whole_line_comment(text: str) -> bool:
	stripped = text.strip()
	return stripped.startswith("//") or (stripped.startswith("/*") and stripped.endswith("*/"))


def has_block_comment_boundary(text: str) -> bool:
	# Inline one-line block comments are safe to preserve in collapsed signatures.
	return ("/*" in text or "*/" in text) and not ("/*" in text and "*/" in text and text.index("/*") < text.rindex("*/"))


def paren_delta(text: str, start_column: int = 0) -> int:
	in_string = None
	escaped = False
	in_block_comment = False
	delta = 0
	i = start_column
	while i < len(text):
		ch = text[i]
		next_ch = text[i + 1] if i + 1 < len(text) else ""
		if in_block_comment:
			if ch == "*" and next_ch == "/":
				in_block_comment = False
				i += 2
				continue
			i += 1
			continue
		if in_string is not None:
			if escaped:
				escaped = False
			elif ch == "\\":
				escaped = True
			elif ch == in_string:
				in_string = None
			i += 1
			continue
		if ch == "/" and next_ch == "/":
			break
		if ch == "/" and next_ch == "*":
			in_block_comment = True
			i += 2
			continue
		if ch in ('"', "'"):
			in_string = ch
			i += 1
			continue
		if ch == "(":
			delta += 1
		elif ch == ")":
			delta -= 1
		i += 1
	return delta


def find_matching_close_line(bodies: list[str], start_index: int, open_column: int, max_span_lines: int) -> int | None:
	depth = 0
	for index in range(start_index, min(len(bodies), start_index + max_span_lines)):
		column = open_column if index == start_index else 0
		depth += paren_delta(bodies[index], column)
		if depth == 0:
			return index
		if depth < 0:
			return None
	return None


def collapse_whitespace(text: str) -> str:
	text = " ".join(text.strip().split())
	text = text.replace("( ", "(").replace(" )", ")")
	text = text.replace(" ,", ",")
	text = normalize_top_level_commas(text)
	text = text.replace(" ;", ";").replace(" {", " {")
	text = re.sub(r"\s+", " ", text).strip()
	text = text.replace(",  ", ", ")
	return text


def normalize_top_level_commas(text: str) -> str:
	parts: list[str] = []
	angle_depth = 0
	index = 0
	while index < len(text):
		ch = text[index]
		if ch == "<":
			angle_depth += 1
		elif ch == ">" and angle_depth > 0:
			angle_depth -= 1
		if ch == "," and angle_depth == 0:
			parts.append(", ")
			index += 1
			while index < len(text) and text[index] == " ":
				index += 1
			continue
		parts.append(ch)
		index += 1
	return "".join(parts)


def stripped_for_length(text: str) -> str:
	return text.expandtabs(4)


def brace_delta(text: str) -> int:
	in_string = None
	escaped = False
	in_block_comment = False
	delta = 0
	i = 0
	while i < len(text):
		ch = text[i]
		next_ch = text[i + 1] if i + 1 < len(text) else ""
		if in_block_comment:
			if ch == "*" and next_ch == "/":
				in_block_comment = False
				i += 2
				continue
			i += 1
			continue
		if in_string is not None:
			if escaped:
				escaped = False
			elif ch == "\\":
				escaped = True
			elif ch == in_string:
				in_string = None
			i += 1
			continue
		if ch == "/" and next_ch == "/":
			break
		if ch == "/" and next_ch == "*":
			in_block_comment = True
			i += 2
			continue
		if ch in ('"', "'"):
			in_string = ch
			i += 1
			continue
		if ch == "{":
			delta += 1
		elif ch == "}":
			delta -= 1
		i += 1
	return delta


def brace_depths_before(bodies: list[str]) -> list[int]:
	depths: list[int] = []
	depth = 0
	for body in bodies:
		depths.append(depth)
		depth = max(0, depth + brace_delta(body))
	return depths


def classify_open_brace_context(text_before_open: str, previous_nonblank: str) -> str:
	context = text_before_open.strip()
	if re.search(r"\b(class|struct)\b", context) or re.match(r"(class|struct)\b", previous_nonblank):
		return "class"
	if ")" in context or ")" in previous_nonblank:
		return "function"
	return "other"


def function_scope_states_before(bodies: list[str]) -> list[bool]:
	states: list[bool] = []
	stack: list[str] = []
	previous_nonblank = ""
	for body in bodies:
		states.append("function" in stack)
		in_string = None
		escaped = False
		in_block_comment = False
		i = 0
		while i < len(body):
			ch = body[i]
			next_ch = body[i + 1] if i + 1 < len(body) else ""
			if in_block_comment:
				if ch == "*" and next_ch == "/":
					in_block_comment = False
					i += 2
					continue
				i += 1
				continue
			if in_string is not None:
				if escaped:
					escaped = False
				elif ch == "\\":
					escaped = True
				elif ch == in_string:
					in_string = None
				i += 1
				continue
			if ch == "/" and next_ch == "/":
				break
			if ch == "/" and next_ch == "*":
				in_block_comment = True
				i += 2
				continue
			if ch in ('"', "'"):
				in_string = ch
				i += 1
				continue
			if ch == "{":
				stack.append(classify_open_brace_context(body[:i], previous_nonblank))
			elif ch == "}" and stack:
				stack.pop()
			i += 1
		if body.strip():
			previous_nonblank = body.strip()
	return states


def block_comment_states_before(bodies: list[str]) -> list[bool]:
	states: list[bool] = []
	in_block_comment = False
	for body in bodies:
		states.append(in_block_comment)
		i = 0
		while i < len(body):
			if in_block_comment:
				close_index = body.find("*/", i)
				if close_index < 0:
					break
				in_block_comment = False
				i = close_index + 2
				continue
			open_index = body.find("/*", i)
			line_index = body.find("//", i)
			if open_index < 0 or (line_index >= 0 and line_index < open_index):
				break
			close_index = body.find("*/", open_index + 2)
			if close_index < 0:
				in_block_comment = True
				break
			i = close_index + 2
	return states


def next_nonblank_body(bodies: list[str], start: int) -> str:
	for index in range(start, len(bodies)):
		stripped = bodies[index].strip()
		if stripped:
			return stripped
	return ""


def next_nonblank_code_body(bodies: list[str], start: int) -> str:
	for index in range(start, len(bodies)):
		stripped = bodies[index].strip()
		if not stripped or is_whole_line_comment(bodies[index]):
			continue
		return stripped
	return ""


def tail_allows_signature_declaration(tail: str) -> bool:
	tail = strip_tail_comments_for_validation(tail).rstrip()
	return bool(re.match(r"^\s*(?:(?:const|volatile|override|final|throw\s*\([^)]*\)|noexcept(?:\s*\([^)]*\))?|=\s*0)\s*)*;\s*$", tail))


def tail_allows_signature_definition(tail: str, following: str) -> bool:
	tail = strip_tail_comments_for_validation(tail).rstrip()
	if re.match(r"^\s*(?:(?:const|volatile|override|final|throw\s*\([^)]*\)|noexcept(?:\s*\([^)]*\))?)\s*)*(?:\{|:)\s*$", tail):
		return True
	if re.match(r"^\s*(?:(?:const|volatile|override|final|throw\s*\([^)]*\)|noexcept(?:\s*\([^)]*\))?)\s*)*$", tail):
		return following.startswith("{") or following.startswith(":")
	return False


# -----------------------------------------------------------------------------
# C++ rewrite detection


def prefix_before_first_open(text: str) -> tuple[str, int] | None:
	open_column = text.find("(")
	if open_column < 0:
		return None
	return text[:open_column], open_column


def first_word(text: str) -> str:
	match = re.match(r"\s*([A-Za-z_][A-Za-z0-9_]*)", text)
	return match.group(1) if match else ""


def has_top_level_comma(text: str) -> bool:
	angle_depth = 0
	for ch in text:
		if ch == "<":
			angle_depth += 1
		elif ch == ">" and angle_depth > 0:
			angle_depth -= 1
		elif ch == "," and angle_depth == 0:
			return True
	return False


def final_prefix_token(text: str) -> str:
	parts = text.rstrip().split()
	return parts[-1] if parts else ""


def looks_like_signature_start(text: str) -> bool:
	stripped = text.strip()
	if not stripped or stripped.startswith(("//", "/*", "*", "#")):
		return False
	if stripped.startswith(":"):
		return False
	if any(stripped.startswith(keyword + " ") or stripped.startswith(keyword + "(") for keyword in CONTROL_START_WORDS):
		return False
	prefix_info = prefix_before_first_open(stripped)
	if prefix_info is None:
		return False
	prefix, _open_column = prefix_info
	prefix = prefix.rstrip()
	if not prefix or "=" in prefix or has_top_level_comma(prefix) or "." in prefix or "->" in prefix:
		return False
	if prefix.endswith(("+", "-", "/", "%", "?", ":", "||", "&&")):
		return False
	if "::" in final_prefix_token(prefix) and leading_ws(text):
		# Most member definitions are flush-left in this codebase; indented :: forms
		# are usually calls such as std::max/std::partial_sort inside a function.
		return False
	if not ("::" in prefix or re.search(r"\s", prefix.rstrip())):
		# Avoid treating ordinary calls such as foo(\n...) as signatures.
		# Class constructors/destructors without :: are intentionally skipped.
		return False
	if re.search(r"~?[A-Za-z_][A-Za-z0-9_]*<[^()]*>$", prefix):
		return "::" in prefix
	return bool(re.search(r"(?:~?[A-Za-z_][A-Za-z0-9_]*|operator\s*[^\s(]+)$", prefix))


def is_unindented_root_cpp_declaration_candidate(text: str, depth: int, suffix: str) -> bool:
	return suffix in {".c", ".cpp"} and depth == 0 and leading_ws(text) == ""


def tail_after_close(bodies: list[str], close_index: int) -> str:
	# Ignore trailing comments when locating the signature-closing parenthesis.
	# Otherwise comments such as `// (exposed to Python)` can make rfind(")")
	# pick a comment parenthesis instead of the real signature close.
	text = strip_tail_comments_for_validation(bodies[close_index])
	close_column = text.rfind(")")
	return text[close_column + 1 :] if close_column >= 0 else ""


def range_has_unsafe_comments(bodies: list[str], start: int, end: int, hoist_comments: bool, trace_hoisted_comments: bool, tail_exposed_to_python_comments: bool, trace_inline_comments: bool) -> bool:
	for index in range(start, end + 1):
		line = bodies[index]
		if is_whole_line_comment(line):
			if not hoist_comments:
				return True
			continue
		if contains_lone_line_comment(line):
			if index == end and has_only_trailing_signature_line_comment(line):
				continue
			if tail_exposed_to_python_comments and exposed_to_python_line_comment_payload(line) is not None:
				continue
			comment_index = lone_line_comment_index(line)
			payload = line[comment_index + 2 :].strip()
			if trace_inline_comments and not is_existing_custom_comment_payload(payload):
				continue
			return True
		if has_block_comment_boundary(line):
			if trace_inline_comments and multiline_block_comment_spans(bodies, start, end) is not None:
				continue
			return True
	return False


def sanitized_xml_comment_text(text: str) -> str:
	return text.replace("--", "- -").replace("\r", " ").replace("\n", " ")


def stripped_comment_payload(text: str) -> str:
	stripped = text.strip()
	if stripped.startswith("//"):
		return stripped[2:].strip()
	return stripped


def is_existing_custom_comment_payload(text: str) -> bool:
	return text.strip().startswith("<!-- custom:")


def normalize_block_comment_payload(lines: list[str]) -> str:
	cleaned: list[str] = []
	for line in lines:
		stripped = line.strip()
		if stripped.startswith("*"):
			stripped = stripped[1:].strip()
		if stripped:
			cleaned.append(stripped)
	return collapse_whitespace(" ".join(cleaned))


def multiline_block_comment_spans(bodies: list[str], start: int, end: int) -> list[BlockCommentSpan] | None:
	spans: list[BlockCommentSpan] = []
	active_start: tuple[int, int] | None = None
	active_lines: list[str] = []
	for index in range(start, end + 1):
		text = bodies[index]
		i = 0
		in_string = None
		escaped = False
		while i < len(text):
			ch = text[i]
			next_ch = text[i + 1] if i + 1 < len(text) else ""
			if active_start is not None:
				close_index = text.find("*/", i)
				if close_index < 0:
					active_lines.append(text[i:])
					break
				active_lines.append(text[i:close_index])
				payload = normalize_block_comment_payload(active_lines)
				if not payload or is_existing_custom_comment_payload(payload):
					return None
				start_index, start_column = active_start
				if start_index != index:
					spans.append(BlockCommentSpan(start_index, index, start_column, close_index, payload))
				active_start = None
				active_lines = []
				i = close_index + 2
				continue
			if in_string is not None:
				if escaped:
					escaped = False
				elif ch == "\\":
					escaped = True
				elif ch == in_string:
					in_string = None
				i += 1
				continue
			if ch in ('"', "'"):
				in_string = ch
				i += 1
				continue
			if ch == "/" and next_ch == "/":
				break
			if ch == "/" and next_ch == "*":
				close_index = text.find("*/", i + 2)
				if close_index >= 0:
					i = close_index + 2
					continue
				active_start = (index, i)
				active_lines = [text[i + 2 :]]
				break
			i += 1
	if active_start is not None:
		return None
	return spans


def split_parameter_segments(text: str) -> list[str]:
	segments: list[str] = []
	start = 0
	paren_depth = 0
	bracket_depth = 0
	brace_depth = 0
	angle_depth = 0
	in_string = None
	escaped = False
	for index, ch in enumerate(text):
		if in_string is not None:
			if escaped:
				escaped = False
			elif ch == "\\":
				escaped = True
			elif ch == in_string:
				in_string = None
			continue
		if ch in ('"', "'"):
			in_string = ch
			continue
		if ch == "(":
			paren_depth += 1
		elif ch == ")" and paren_depth > 0:
			paren_depth -= 1
		elif ch == "[":
			bracket_depth += 1
		elif ch == "]" and bracket_depth > 0:
			bracket_depth -= 1
		elif ch == "{":
			brace_depth += 1
		elif ch == "}" and brace_depth > 0:
			brace_depth -= 1
		elif ch == "<":
			angle_depth += 1
		elif ch == ">" and angle_depth > 0:
			angle_depth -= 1
		elif ch == "," and paren_depth == 0 and bracket_depth == 0 and brace_depth == 0 and angle_depth == 0:
			segments.append(text[start:index])
			start = index + 1
	segments.append(text[start:])
	return [segment for segment in segments if segment.strip()]


def param_name_from_segment(segment: str) -> str | None:
	segment = strip_tail_comments_for_validation(segment)
	segment = segment.split("=", 1)[0].strip()
	names = re.findall(r"[A-Za-z_][A-Za-z0-9_]*", segment)
	for name in reversed(names):
		if name not in {"const", "volatile", "unsigned", "signed", "long", "short", "int", "bool", "float", "double", "char", "wchar_t", "void", "LPCTSTR", "LPCSTR", "LPCWSTR"}:
			return name
	return None


def previous_parameter_name(bodies: list[str], start: int, comment_index: int) -> str | None:
	pieces: list[str] = []
	for index in range(start, comment_index):
		if not is_whole_line_comment(bodies[index]):
			pieces.append(strip_trailing_line_comment(bodies[index]).strip())
	text = " ".join(pieces)
	open_index = text.find("(")
	if open_index >= 0:
		text = text[open_index + 1 :]
	segments = split_parameter_segments(text)
	return param_name_from_segment(segments[-1]) if segments else None


def next_parameter_name(bodies: list[str], comment_index: int, end: int) -> str | None:
	pieces: list[str] = []
	for index in range(comment_index + 1, end + 1):
		if not is_whole_line_comment(bodies[index]):
			pieces.append(strip_trailing_line_comment(bodies[index]).strip())
	text = " ".join(pieces)
	close_index = text.rfind(")")
	if close_index >= 0:
		text = text[:close_index]
	segments = split_parameter_segments(text)
	return param_name_from_segment(segments[0]) if segments else None


def parameter_name_before_inline_comment(bodies: list[str], start: int, comment_index: int, inline_prefix: str) -> str | None:
	pieces: list[str] = []
	for index in range(start, comment_index):
		if not is_whole_line_comment(bodies[index]):
			pieces.append(strip_trailing_line_comment(bodies[index]).strip())
	pieces.append(inline_prefix.strip())
	text = " ".join(pieces)
	open_index = text.find("(")
	if open_index >= 0:
		text = text[open_index + 1 :]
	segments = split_parameter_segments(text)
	return param_name_from_segment(segments[-1]) if segments else None


def hoisted_comment_lines_with_trace(indent: str, payload: str, position: str, trace_credit: str) -> list[str]:
	credit = sanitized_xml_comment_text(trace_credit)
	trace = f"{indent}// <!-- custom: hoisted from multiline signature {position} by collapse_cpp_signatures.py. ({credit}) -->"
	if not payload:
		return [trace]
	# <!-- custom: Keep preserved inherited/custom comment text physically separate from the script-authored provenance marker. Appending both to one `//` line made an inherited sentence look jointly authored by the tracing pass. (ChatGPT-5.6-Sol) -->
	return [f"{indent}// {payload}", trace]


def hoisted_inline_trace_comment(bodies: list[str], start: int, end: int, comment_index: int, inline_prefix: str, payload: str, indent: str, trace_credit: str) -> list[str]:
	before = parameter_name_before_inline_comment(bodies, start, comment_index, inline_prefix)
	after = next_parameter_name(bodies, comment_index, end)
	if before is not None and after is not None:
		position = f"between `{before}` and `{after}`"
	elif before is not None:
		position = f"after `{before}`"
	elif after is not None:
		position = f"before `{after}`"
	else:
		position = "inside multiline signature"
	return hoisted_comment_lines_with_trace(indent, payload, position, trace_credit)


def hoisted_block_trace_comment(bodies: list[str], start: int, end: int, span: BlockCommentSpan, indent: str, trace_credit: str) -> list[str]:
	before = parameter_name_before_inline_comment(bodies, start, span.start, bodies[span.start][:span.start_column])
	after = next_parameter_name(bodies, span.end, end)
	if before is not None and after is not None:
		position = f"between `{before}` and `{after}`"
	elif before is not None:
		position = f"after `{before}`"
	elif after is not None:
		position = f"before `{after}`"
	else:
		position = "inside multiline signature"
	return hoisted_comment_lines_with_trace(indent, span.payload, position, trace_credit)


def hoisted_trace_comment(bodies: list[str], start: int, end: int, comment_index: int, indent: str, trace_credit: str) -> list[str]:
	before = previous_parameter_name(bodies, start, comment_index)
	after = next_parameter_name(bodies, comment_index, end)
	if before is not None and after is not None:
		position = f"between `{before}` and `{after}`"
	elif after is not None:
		position = f"before `{after}`"
	elif before is not None:
		position = f"after `{before}`"
	else:
		position = "inside multiline signature"
	payload = stripped_comment_payload(bodies[comment_index])
	return hoisted_comment_lines_with_trace(indent, payload, position, trace_credit)


def append_tail_line_comment(text: str, payload: str) -> str:
	comment_index = lone_line_comment_index(text)
	if comment_index >= 0:
		return text[:comment_index].rstrip() + " // " + text[comment_index + 2 :].strip() + "; " + payload
	return text.rstrip() + " // " + payload


def collapsed_range_lines(bodies: list[str], eols: list[str], start: int, end: int, indent: str, max_line_len: int, hoist_comments: bool, trace_hoisted_comments: bool, trace_credit: str | None, tail_exposed_to_python_comments: bool, trace_inline_comments: bool) -> list[str] | None:
	comment_lines: list[str] = []
	code_pieces: list[str] = []
	tail_comment_payloads: list[str] = []
	bodies_for_rewrite = list(bodies)
	block_comments_by_start: dict[int, list[BlockCommentSpan]] = {}
	if trace_inline_comments:
		block_spans = multiline_block_comment_spans(bodies, start, end)
		if block_spans is None:
			return None
		for span in block_spans:
			block_comments_by_start.setdefault(span.start, []).append(span)
		for span in reversed(block_spans):
			if span.start == span.end:
				continue
			prefix = bodies_for_rewrite[span.start][:span.start_column]
			suffix = bodies_for_rewrite[span.end][span.end_column + 2 :]
			bodies_for_rewrite[span.start] = prefix + suffix
			for blank_index in range(span.start + 1, span.end + 1):
				bodies_for_rewrite[blank_index] = ""
	for index in range(start, end + 1):
		for span in block_comments_by_start.get(index, []):
			comment_lines.extend(hoisted_block_trace_comment(bodies, start, end, span, indent, trace_credit or "uncredited"))
		body = bodies_for_rewrite[index]
		stripped = body.strip()
		if is_whole_line_comment(body):
			if hoist_comments:
				if trace_hoisted_comments:
					comment_lines.extend(hoisted_trace_comment(bodies_for_rewrite, start, end, index, indent, trace_credit or "uncredited"))
				else:
					comment_lines.append(indent + stripped)
			continue
		payload = exposed_to_python_line_comment_payload(body) if tail_exposed_to_python_comments else None
		if payload is not None:
			body = strip_trailing_line_comment(body)
			tail_comment_payloads.append(payload)
		elif trace_inline_comments and contains_lone_line_comment(body) and not (index == end and has_only_trailing_signature_line_comment(body)):
			comment_index = lone_line_comment_index(body)
			comment_payload = body[comment_index + 2 :].strip()
			body = body[:comment_index]
			comment_lines.extend(hoisted_inline_trace_comment(bodies_for_rewrite, start, end, index, body, comment_payload, indent, trace_credit or "uncredited"))
		code_pieces.append(body.strip())

	collapsed = collapse_whitespace(" ".join(code_pieces))
	for payload in tail_comment_payloads:
		collapsed = append_tail_line_comment(collapsed, payload)
	if len(stripped_for_length(collapsed)) > max_line_len:
		return None
	new_lines = [line + eols[start] for line in comment_lines]
	new_lines.append(indent + collapsed + eols[end])
	return new_lines


def try_signature_rewrite(
	bodies: list[str],
	eols: list[str],
	brace_depths: list[int],
	function_scope_states: list[bool],
	block_comment_states: list[bool],
	index: int,
	suffix: str,
	max_line_len: int,
	max_span_lines: int,
	hoist_comments: bool,
	relative_path: Path,
	ignored: list[IgnoredCandidate] | None,
	include_nonsignature_ignored: bool,
	trace_hoisted_comments: bool,
	trace_credit: str | None,
	tail_exposed_to_python_comments: bool,
	trace_inline_comments: bool,
) -> Rewrite | None:
	body = bodies[index]
	if block_comment_states[index]:
		return None
	prefix_info = prefix_before_first_open(body)
	if prefix_info is None:
		return None
	_, open_column = prefix_info
	if paren_delta(body, open_column) <= 0:
		return None

	def reject(reason: str, end_index: int | None = None) -> None:
		if ignored is None:
			return
		if end_index is None:
			end_index = min(len(bodies) - 1, index + max_span_lines - 1)
		ignored.append(IgnoredCandidate(relative_path, index, end_index, reason, [bodies[i] + eols[i] for i in range(index, end_index + 1)]))

	is_signature_start = looks_like_signature_start(body)
	close_index = find_matching_close_line(bodies, index, open_column, max_span_lines)
	if close_index is None:
		if is_signature_start or include_nonsignature_ignored:
			reject(f"no matching ')' within {max_span_lines} line(s)")
		return None
	if close_index == index:
		return None

	if not is_signature_start:
		if include_nonsignature_ignored:
			reject("not a signature-like start", close_index)
		return None

	tail = tail_after_close(bodies, close_index)
	following = next_nonblank_code_body(bodies, close_index + 1)
	depth = brace_depths[index]
	inside_function_scope = function_scope_states[index]
	suffix = suffix.lower()
	is_header = suffix in {".h", ".hpp", ".inl"}
	is_definition = tail_allows_signature_definition(tail, following)
	is_declaration = tail_allows_signature_declaration(tail)
	is_root_cpp_declaration = is_unindented_root_cpp_declaration_candidate(body, depth, suffix)

	# <!-- custom: Default mode must not rewrite ordinary local calls/constructors such as logBBAI(...), scaled r(...), or CvWString szMsg(...). Function definitions are normally top-level, namespace-level, or inline inside a class; declarations ending in ';' are safe enough in headers/class declarations and unindented root-scope .cpp declarations. Trailing end-of-signature // comments are allowed, but comments inside the parameter list are still skipped unless trace-hoisted. (GPT-5.5) -->
	if is_definition:
		if depth > 2:
			reject(f"definition-like candidate is inside nested scope (brace depth {depth})", close_index)
			return None
	elif is_declaration:
		if not is_header and not is_root_cpp_declaration:
			if depth > 0:
				if include_nonsignature_ignored:
					reject(f"declaration-like candidate is inside nested scope (brace depth {depth})", close_index)
				return None
			reject("declaration-like candidate is outside a header-like file", close_index)
			return None
		if depth > 2 or (depth > 1 and inside_function_scope):
			reject(f"declaration-like candidate is inside nested scope (brace depth {depth})", close_index)
			return None
	else:
		reject("tail is not a safe signature declaration/definition", close_index)
		return None

	if range_has_unsafe_comments(bodies, index, close_index, hoist_comments, trace_hoisted_comments, tail_exposed_to_python_comments and is_header and is_declaration, trace_inline_comments):
		reject("contains comments that are unsafe to collapse", close_index)
		return None
	indent = leading_ws(body)
	new_lines = collapsed_range_lines(bodies, eols, index, close_index, indent, max_line_len, hoist_comments, trace_hoisted_comments, trace_credit, tail_exposed_to_python_comments and is_header and is_declaration, trace_inline_comments)
	if new_lines is None:
		reject(f"collapsed line would exceed --max-line-len {max_line_len}", close_index)
		return None
	return Rewrite(index, close_index, new_lines, "signature")


def rewrite_lines(lines: list[str], suffix: str, max_line_len: int, max_span_lines: int, hoist_comments: bool, relative_path: Path, ignored: list[IgnoredCandidate] | None, include_nonsignature_ignored: bool, trace_hoisted_comments: bool, trace_credit: str | None, tail_exposed_to_python_comments: bool, trace_inline_comments: bool) -> tuple[list[str], Stats]:
	bodies: list[str] = []
	eols: list[str] = []
	for line in lines:
		body, eol = split_line_ending(line)
		bodies.append(body)
		eols.append(eol)

	brace_depths = brace_depths_before(bodies)
	function_scope_states = function_scope_states_before(bodies)
	block_comment_states = block_comment_states_before(bodies)
	stats = Stats()
	new_lines: list[str] = []
	index = 0
	while index < len(lines):
		rewrite = try_signature_rewrite(bodies, eols, brace_depths, function_scope_states, block_comment_states, index, suffix, max_line_len, max_span_lines, hoist_comments, relative_path, ignored, include_nonsignature_ignored, trace_hoisted_comments, trace_credit, tail_exposed_to_python_comments, trace_inline_comments)
		if rewrite is None:
			new_lines.append(lines[index])
			index += 1
			continue
		new_lines.extend(rewrite.lines)
		stats.signatures += 1
		index = rewrite.end + 1
	return new_lines, stats


# -----------------------------------------------------------------------------
# CLI


def default_diff_file_path(repo_root: Path, timestamp: str) -> Path:
	return repo_root / "LLM_Helpers" / "outputs" / f"collapse_cpp_signatures_{timestamp}.diff.txt"


def default_ignored_file_path(repo_root: Path, timestamp: str) -> Path:
	return repo_root / "LLM_Helpers" / "outputs" / f"collapse_cpp_signatures_{timestamp}_ignored.txt"


def resolve_output_file_path(repo_root: Path, value: str | None, default_path: Path) -> Path | None:
	if value is None:
		return None
	if value == "auto":
		return default_path
	path = Path(value)
	return path if path.is_absolute() else repo_root / path


def display_path(repo_root: Path, path: Path) -> str:
	try:
		return path.relative_to(repo_root).as_posix()
	except ValueError:
		return str(path)


def diff_text(relative_path: Path, before: str, after: str) -> str:
	return "".join(
		difflib.unified_diff(
			before.splitlines(keepends=True),
			after.splitlines(keepends=True),
			fromfile=f"{relative_path.as_posix()} (before)",
			tofile=f"{relative_path.as_posix()} (after)",
		)
	)


def write_ignored_report(path: Path, ignored: list[IgnoredCandidate]) -> None:
	path.parent.mkdir(parents=True, exist_ok=True)
	reason_counts = Counter(candidate.reason for candidate in ignored)
	with path.open("w", encoding="utf-8", newline="") as out:
		out.write("Ignored multiline C/C++ signature candidates\n")
		out.write("Generated by LLM_Helpers/collapse_cpp_signatures.py\n")
		out.write("These candidates were intentionally not rewritten by the signature-only helper.\n")
		out.write("Use this report for review, not as an error list. Add --include-nonsignature-ignored to also list ordinary calls/local statements.\n")
		out.write(f"Total ignored candidates: {len(ignored)}\n")
		out.write("\nIgnored reason summary:\n")
		for reason, count in reason_counts.most_common():
			out.write(f"- {count}: {reason}\n")
		out.write("\n")
		for candidate in ignored:
			out.write(candidate.format())
			out.write("\n")


def process_file(repo_root: Path, path: Path, args: argparse.Namespace, stats: Stats, diff_file, ignored: list[IgnoredCandidate] | None) -> bool:
	raw = path.read_bytes()
	if b"\0" in raw:
		return False
	encoding, text = decode_bytes(raw)
	lines = text.splitlines(keepends=True)
	if not lines:
		return False
	relative_path = path.relative_to(repo_root)
	ignored_before = len(ignored) if ignored is not None else 0
	new_lines, file_stats = rewrite_lines(lines, path.suffix, args.max_line_len, args.max_span_lines, args.hoist_comments, relative_path, ignored, args.include_nonsignature_ignored, args.trace_hoisted_comments, args.trace_credit, args.tail_exposed_to_python_comments, args.trace_inline_comments)
	if ignored is not None:
		stats.ignored += len(ignored) - ignored_before
	new_text = "".join(new_lines)
	stats.files_scanned += 1
	if new_text == text:
		return False

	stats.files_changed += 1
	stats.signatures += file_stats.signatures
	if args.diff or diff_file is not None:
		diff = diff_text(relative_path, text, new_text)
		if args.diff:
			print(diff)
		if diff_file is not None:
			diff_file.write(diff)
	if args.in_place:
		path.write_bytes(new_text.encode(encoding))
	return True


def main() -> int:
	parser = argparse.ArgumentParser(description="Conservative C++ signature collapse helper.")
	parser.add_argument("paths", nargs="*", type=Path, help="optional files/folders to scan; defaults to CvGameCoreDLL tracked C++ files")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to this helper's parent repo")
	parser.add_argument("--in-place", action="store_true", help="write changes; otherwise only report")
	parser.add_argument("--diff", action="store_true", help="print unified diffs for files that would change")
	parser.add_argument("--diff-file", nargs="?", const="auto", default=None, help="write unified diffs to a file; without a path, writes a timestamped file under LLM_Helpers/outputs/")
	parser.add_argument("--ignored-file", nargs="?", const="auto", default=None, help="write skipped signature-like multiline candidates to a file; without a path, writes a timestamped ignored-candidates file under LLM_Helpers/outputs/")
	parser.add_argument("--include-nonsignature-ignored", action="store_true", help="with --ignored-file, also report ordinary multiline calls/local statements that were skipped as non-signatures; very verbose")
	parser.add_argument("--hoist-comments", action="store_true", help="hoist whole-line comments from inside a collapsed signature above the signature")
	parser.add_argument("--trace-hoisted-comments", action="store_true", help="when hoisting whole-line comments, add a separate custom trace line that records their original parameter position")
	parser.add_argument("--trace-inline-comments", action="store_true", help="hoist inline // comments from inside a collapsed signature and add a separate custom trace line for their original parameter position")
	parser.add_argument("--trace-credit", default=DEFAULT_TRACE_CREDIT, help=f"credit suffix for trace comments; default: \"{DEFAULT_TRACE_CREDIT}\"")
	parser.add_argument("--tail-exposed-to-python-comments", action="store_true", help="for header declarations only, move inline // Exposed to Python comments inside the parameter list to the final signature tail comment")
	parser.add_argument("--max-line-len", type=int, default=600, help="skip collapses that would exceed this physical line length")
	parser.add_argument("--max-span-lines", type=int, default=16, help="skip candidates spanning more than this many physical lines")
	args = parser.parse_args()
	if args.trace_hoisted_comments:
		args.hoist_comments = True

	repo_root = args.repo_root.resolve()
	timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
	diff_file_path = resolve_output_file_path(repo_root, args.diff_file, default_diff_file_path(repo_root, timestamp))
	ignored_file_path = resolve_output_file_path(repo_root, args.ignored_file, default_ignored_file_path(repo_root, timestamp))
	ignored_candidates: list[IgnoredCandidate] | None = [] if ignored_file_path is not None else None
	diff_file = None
	if diff_file_path is not None:
		diff_file_path.parent.mkdir(parents=True, exist_ok=True)
		diff_file = diff_file_path.open("w", encoding="utf-8", newline="")

	stats = Stats()
	changed_paths: list[Path] = []
	try:
		for path in iter_candidate_files(repo_root, args.paths):
			if process_file(repo_root, path, args, stats, diff_file, ignored_candidates):
				changed_paths.append(path.relative_to(repo_root))
	finally:
		if diff_file is not None:
			diff_file.close()

	if ignored_file_path is not None and ignored_candidates is not None:
		if ignored_candidates:
			write_ignored_report(ignored_file_path, ignored_candidates)
		elif ignored_file_path.exists():
			ignored_file_path.unlink()

	print(f"Scanned {stats.files_scanned} C/C++ file(s).")
	if not changed_paths:
		print("No safe C++ signature collapses found.")
		if diff_file_path is not None and diff_file_path.exists() and diff_file_path.stat().st_size == 0:
			diff_file_path.unlink()
			print("No diff file written.")
		if ignored_file_path is not None:
			if ignored_candidates:
				print(f"Ignored-candidates file written: {display_path(repo_root, ignored_file_path)}")
			else:
				print("No ignored-candidates file written.")
		return 0

	action = "Updated" if args.in_place else "Would update"
	print(f"{action} {len(changed_paths)} file(s): {stats.signatures} signature collapse(s).")
	for relative_path in changed_paths:
		print(f"  - {relative_path.as_posix()}")
	if diff_file_path is not None:
		print(f"Diff file written: {display_path(repo_root, diff_file_path)}")
	if ignored_file_path is not None:
		if ignored_candidates:
			print(f"Ignored {len(ignored_candidates)} candidate(s). Ignored-candidates file written: {display_path(repo_root, ignored_file_path)}")
		else:
			print("No ignored-candidates file written.")
	if not args.in_place:
		print("Run again with --in-place after reviewing the target list/diff.")
	return 0 if args.in_place else 1


if __name__ == "__main__":
	sys.exit(main())
